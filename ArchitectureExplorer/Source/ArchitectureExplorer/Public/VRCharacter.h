// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

class UCameraComponent;
struct FTimerHandle;

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

	// player input handlers
	void OnMoveForward(float throttle);
	void OnMoveRight(float throttle);
	void OnTeleport();

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

private:
	// TELEPORT FUNCTIONALITY

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxTeleportDistance = 1000.0f; // centimeters

	UPROPERTY(EditAnywhere, Category = "Movement")
	float TeleportFadeOut = 1.0f; // seconds

	UPROPERTY(EditAnywhere, Category = "Movement")
	float TeleportFadeIn = 1.0f; // seconds

	UPROPERTY(EditAnywhere, Category = "Movement")
	FVector TeleportProjectionExtent = FVector(100.0f, 100.0f, 100.0f);

	FTimerHandle TeleportFadeTimerHandle;

	// we don't teleport until after the fade-out completes
	// this variable is used to remember where we want to teleport to
	FVector TeleportLocation;

	// project the teleport location down to the navigation mesh to avoid targets that are on walls
	// returns true if navigation mesh point was found
	bool bProjectTeleportToNavigation(FVector &OutLocation, FVector InLocation);

	// do a line trace and project point to navigation mesh to find a teleport destination
	// returns true if found
	bool bFindTeleportDestination(FVector &OutLocation);

	// move the DestinationMarker to the first hit of a linetrace
	// this is called every tick
	void MoveDestinationMarkerByLineTrace();

	// functions to begin and finish teleportation
	// phasing in and out requires two separate steps
	void BeginTeleport();
	void FinishTeleport();
};
