// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "InteractInterface.h"

#include "ColorChangingCube.generated.h"

UCLASS()
class AGORA_API AColorChangingCube : public AActor, public IInteractInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AColorChangingCube();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	virtual void Interact_Implementation(AActor* Interactor) override;

private:
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerInteractWithObject(AActor* Interactor);

	UFUNCTION()
	void ChangeColor();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CubeMesh;

	UPROPERTY(ReplicatedUsing = OnRep_CubeColor)
	FLinearColor CubeColor;

	UFUNCTION()
	void OnRep_CubeColor();
};
