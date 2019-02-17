// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

class UCameraComponent;

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent *Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USceneComponent *VRRoot;

	// VR marker for teleportation
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	UStaticMeshComponent *DestinationMarker;

	void OnMoveForward(float throttle);
	void OnMoveRight(float throttle);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	// move the player pawn to the position of the VR camera
	// and then readjust the VRRoot in the opposite direction to keep camera stationary
	// ---
	// when we walk around in our vr space, this will ensure the pawn location updates, too
	void MovePawnToVRCamera();

	// move the DestinationMarker to the first hit of a linetrace
	void MoveDestinationMarkerByLineTrace();

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxTeleportDistance = 1000.0f; // centimeters
};
