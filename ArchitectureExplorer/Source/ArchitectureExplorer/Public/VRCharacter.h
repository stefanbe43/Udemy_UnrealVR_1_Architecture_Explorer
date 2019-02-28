// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

// Forward declarations
//
// I could also include the headers, but I avoid including headers in headers
// to improve compile times. A forward declaration is enough as long as
// my header doesn't need to know about what the type implements. Since the
// code is in the cpp file, it usually doesn't need to know about the
// implementation.
class UCameraComponent;
class UPostProcessComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCurveFloat;
class UMotionControllerComponent;
struct FTimerHandle;

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent *Camera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USceneComponent *VRRoot = nullptr;

	// player input handlers
	void OnMoveForward(float throttle);
	void OnMoveRight(float throttle);
	void OnTeleport();

private:

	// move the player pawn to the position of the VR camera
	// and then readjust the VRRoot in the opposite direction to keep camera stationary
	// ---
	// when we walk around in our vr space, this will ensure the pawn location updates, too
	void MovePawnToVRCamera();

/////////////////////
// MOTION CONTROLLERS

private:

	UPROPERTY(VisibleAnywhere, Category = "Controller")
	UMotionControllerComponent *LeftMotionControllerComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Controller")
	UMotionControllerComponent *RightMotionControllerComponent = nullptr;

////////////////////////
// BLINKER FUNCTIONALITY
protected:

	UPROPERTY(VisibleAnywhere, Category = "Blinker")
	UPostProcessComponent *PostProcessComponent = nullptr;

private:

	UPROPERTY(EditAnywhere, Category = "Blinker")
	UMaterialInterface *BlinkerMaterialBase = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Blinker")
	UMaterialInstanceDynamic *BlinkerMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, Category = "Blinker")
	UCurveFloat *RadiusVsVelocity = nullptr;

	// Setup the blinker postprocessing effect
	//
	// the blinker material type comes from Blueprint set via the BlinkerMaterialBase
	// I create a dynamic material to allow for blinker parameters to be modified
	// at run-time. For example, we can adjust the radius depending on player movement.
	void SetupBlinkerPostprocessingEffect();

	// calculate a new blinker radius based on my current velocity
	// then update the blinker radius
	//
	// the faster character goes, the tighter the blinker should be
	void UpdateBlinkerRadius();

	// calculate a new blinker center based on direction character moves
	// do not update the blinker center in this function
	FVector2D CalculateBlinkerCenter() const;

	// update the blinker center (calls CalculateBlinkerCenter)
	void UpdateBlinkerCenter();

/////////////////////////
// TELEPORT FUNCTIONALITY
protected:
	// VR marker for teleportation
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	UStaticMeshComponent *DestinationMarker = nullptr;

private:

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
