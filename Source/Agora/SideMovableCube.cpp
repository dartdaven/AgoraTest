// Fill out your copyright notice in the Description page of Project Settings.


#include "SideMovableCube.h"

#include "Net/UnrealNetwork.h"

// Sets default values
ASideMovableCube::ASideMovableCube()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
    
    CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
    RootComponent = CubeMesh;

    bReplicates = true;
    SetReplicateMovement(true);

    bInitialLocation = true;
    MoveDistance = 500.f;
}

// Called when the game starts or when spawned
void ASideMovableCube::BeginPlay()
{
	Super::BeginPlay();
	
    InitialLocation = GetActorLocation();
    TargetLocation = InitialLocation - FVector(0, MoveDistance, 0);
}

void ASideMovableCube::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASideMovableCube, bInitialLocation);
}


void ASideMovableCube::Interact_Implementation(AActor* Interactor)
{
    if (HasAuthority())
    {
        MoveCube();
    }
    else
    {
       ServerInteractWithObject(Interactor);
    }
}

void ASideMovableCube::MoveCube()
{
    if (bInitialLocation)
    {
        SetActorLocation(TargetLocation);
        bInitialLocation = false;
    }
    else
    {
        SetActorLocation(InitialLocation);
        bInitialLocation = true;
    }
}

bool ASideMovableCube::ServerInteractWithObject_Validate(AActor* Interactor)
{
    return true;
}

void ASideMovableCube::ServerInteractWithObject_Implementation(AActor* Interactor)
{
    MoveCube();
}



