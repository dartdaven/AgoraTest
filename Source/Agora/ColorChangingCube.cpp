// Fill out your copyright notice in the Description page of Project Settings.


#include "ColorChangingCube.h"

#include "Net/UnrealNetwork.h"

// Sets default values
AColorChangingCube::AColorChangingCube()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	RootComponent = CubeMesh;

	CubeColor = FLinearColor::White;
}

// Called when the game starts or when spawned
void AColorChangingCube::BeginPlay()
{
	Super::BeginPlay();
	
}

void AColorChangingCube::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AColorChangingCube, CubeColor);
}

void AColorChangingCube::Interact_Implementation(AActor* Interactor)
{
	if (HasAuthority())
	{
		ChangeColor();
	}
	else
	{
		ServerInteractWithObject(Interactor);
	}
}

void AColorChangingCube::ServerInteractWithObject_Implementation(AActor* Interactor)
{
	Interact_Implementation(Interactor);
}

bool AColorChangingCube::ServerInteractWithObject_Validate(AActor* Interactor)
{
	return true;
}

void AColorChangingCube::ChangeColor()
{
	CubeColor = FLinearColor::MakeRandomColor();
	OnRep_CubeColor();
}

void AColorChangingCube::OnRep_CubeColor()
{
	if (CubeMesh)
	{	
		FVector CubeColorVector = FVector(CubeColor.R, CubeColor.G, CubeColor.B);

		//Cube material must have parameter with exact name BaseColor
		CubeMesh->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), CubeColorVector);
	}
}