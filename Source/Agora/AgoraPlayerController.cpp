// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgoraPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "AgoraCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h" 
#include "InteractInterface.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AAgoraPlayerController::AAgoraPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;

	CameraMovementSpeed = 1000.f;
	MaxInteractionDistance = 250.f;
}

void AAgoraPlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	if (IsLocalPlayerController())
	{
		if (HUDWidgetClass)
		{
			HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
			if (HUDWidgetInstance)
			{
				HUDWidgetInstance->AddToViewport();
			}
		}
	}
}

void AAgoraPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &AAgoraPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &AAgoraPlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &AAgoraPlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &AAgoraPlayerController::OnSetDestinationReleased);

		// Setup camera movement
		EnhancedInputComponent->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &AAgoraPlayerController::MoveCamera);
		EnhancedInputComponent->BindAction(ResetCameraAction, ETriggerEvent::Started, this, &AAgoraPlayerController::ResetCamera);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AAgoraPlayerController::OnInputStarted()
{
	StopMovement();

	FHitResult Hit;
	bool bHitSuccessful = false;

	bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	if (Hit.GetActor() && Hit.GetActor()->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
	{
		InteractWithObject(Hit.GetActor());
	}
}

// Triggered every frame when the input is held down
void AAgoraPlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();
	
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	
	bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
	
	// Move towards mouse pointer
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void AAgoraPlayerController::OnSetDestinationReleased()
{

	FHitResult Hit;
	bool bHitSuccessful = false;

	bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	if (Hit.GetActor() && Hit.GetActor()->GetClass()->ImplementsInterface(UInteractInterface::StaticClass())) 
	{
	}
	// If it was a short press
	else if (FollowTime <= ShortPressThreshold)
	{
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}


void AAgoraPlayerController::InteractWithObject(AActor* InteractableObject)
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn && InteractableObject)
	{
		float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), InteractableObject->GetActorLocation());

		if (Distance <= MaxInteractionDistance)
		{
			if (HasAuthority())
			{
				if (InteractableObject->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
				{
					IInteractInterface::Execute_Interact(InteractableObject, GetPawn());
				}
			}
			else
			{
				ServerInteractWithObject(InteractableObject);
			}
		}
	}

}

void AAgoraPlayerController::ServerInteractWithObject_Implementation(AActor* InteractableObject)
{
	InteractWithObject(InteractableObject);
}

bool AAgoraPlayerController::ServerInteractWithObject_Validate(AActor* InteractableObject)
{
	return true;
}

void AAgoraPlayerController::MoveCamera(const FInputActionValue& Value)
{
	AAgoraCharacter* ControlledAgoraCharacter = Cast<AAgoraCharacter>(GetPawn());

	if (IsValid(ControlledAgoraCharacter))
	{
		USpringArmComponent* CameraBoom = ControlledAgoraCharacter->GetCameraBoom();

		if (!CameraBoom->IsUsingAbsoluteLocation())
		{
			CameraBoom->SetUsingAbsoluteLocation(true);

			CameraBoom->TargetOffset = ControlledAgoraCharacter->GetActorLocation();

			ControlledAgoraCharacter->UpdateComponentTransforms();
		}
		else
		{
			FVector2D MovementVector = Value.Get<FVector2D>();
			FVector DeltaLocation = FVector(MovementVector.Y * CameraMovementSpeed, MovementVector.X * CameraMovementSpeed, 0) * UGameplayStatics::GetWorldDeltaSeconds(this);
		
			CameraBoom->TargetOffset += DeltaLocation;
		}
	}
}

void AAgoraPlayerController::ResetCamera()
{
	AAgoraCharacter* ControlledAgoraCharacter = Cast<AAgoraCharacter>(GetPawn());

	if (IsValid(ControlledAgoraCharacter))
	{
		USpringArmComponent* CameraBoom = ControlledAgoraCharacter->GetCameraBoom();

		if (CameraBoom->IsUsingAbsoluteLocation())
		{
			CameraBoom->SetUsingAbsoluteLocation(false);

			CameraBoom->TargetOffset = FVector::ZeroVector;

			ControlledAgoraCharacter->UpdateComponentTransforms();
		}
	}
}
