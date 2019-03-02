// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/PostProcessComponent.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	if (ensure(DestinationMarker != nullptr))
	{
		DestinationMarker->SetupAttachment(GetRootComponent());
	}

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	if (ensure(PostProcessComponent != nullptr))
	{
		PostProcessComponent->SetupAttachment(GetRootComponent());
	}


	// VRRoot gets adjusted to player's position in the VR play space
	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	if (ensure(VRRoot != nullptr))
	{
		VRRoot->SetupAttachment(GetRootComponent());

		Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
		if (ensure(Camera != nullptr))
		{
			Camera->SetupAttachment(VRRoot);
		}

		LeftMotionControllerComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftMotionControllerComponent"));
		if (ensure(LeftMotionControllerComponent != nullptr))
		{
			LeftMotionControllerComponent->SetupAttachment(VRRoot);
			LeftMotionControllerComponent->SetTrackingSource(EControllerHand::Left);
			//LeftMotionControllerComponent->SetShowDeviceModel(true);
		}

		RightMotionControllerComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightMotionControllerComponent"));
		if (ensure(RightMotionControllerComponent != nullptr))
		{
			RightMotionControllerComponent->SetupAttachment(VRRoot);
			RightMotionControllerComponent->SetTrackingSource(EControllerHand::Right);
			//RightMotionControllerComponent->SetShowDeviceModel(true);
		}

	}
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (ensure(DestinationMarker != nullptr))
	{
		DestinationMarker->SetVisibility(false);
	}

	SetupBlinkerPostprocessingEffect();

}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// adjust our position based on how much we walked in our space
	MovePawnToVRCamera();

	// update teleportation marker
	MoveDestinationMarkerByLineTrace();

	// as we move faster, make the radius of blinker smaller to reduce motionsickness
	UpdateBlinkerRadius();
	// center the blinker in direction of motion to reduce motionsickness
	UpdateBlinkerCenter();

}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::OnMoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::OnMoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), EInputEvent::IE_Pressed, this, &AVRCharacter::OnTeleport);
}

void AVRCharacter::OnMoveForward(float throttle)
{
	if (!ensure(Camera != nullptr)) { return; }

	//UE_LOG(LogTemp, Warning, TEXT("AVRCharacter::OnMoveForward() called"));

	auto ForwardVector = Camera->GetForwardVector();
	AddMovementInput(throttle * ForwardVector);
}

void AVRCharacter::OnMoveRight(float throttle)
{
	if (!ensure(Camera != nullptr)) { return; }

	//UE_LOG(LogTemp, Warning, TEXT("AVRCharacter::OnMoveRight() called"));

	auto RightVector = Camera->GetRightVector();
	AddMovementInput(throttle * RightVector);
}

// Teleport button was pressed
void AVRCharacter::OnTeleport()
{

	if (!ensure(DestinationMarker != nullptr))
	{
		return;
	}

	// ignore teleport request if the destination marker is not visible (e.g., did not find a valid destination)
	if (!DestinationMarker->IsVisible())
	{
		return;
	}

	// also ignore teleport request if the last teleport is still in progress
	if (GetWorldTimerManager().IsTimerActive(TeleportFadeTimerHandle))
	{
		return;
	}

	// and ignore teleport request if we are not attached to a playercontroller
	// or our playercontroller has no camera manager (for fading)
	auto PlayerController = Cast<APlayerController>(GetController());
	if ( (PlayerController == nullptr) || (PlayerController->PlayerCameraManager == nullptr) )
	{
		return;
	}

	BeginTeleport();
}

// called every tick
void AVRCharacter::MovePawnToVRCamera()
{
	if (!ensure(Camera != nullptr))
	{
		return;
	}

	if (!ensure(VRRoot != nullptr))
	{
		return;
	}

	auto CameraLocation = Camera->GetComponentLocation();

	// how far camera has moved away from actor this frame
	auto Delta = CameraLocation - GetActorLocation();
	// clear the vertical direction
	Delta.Z = 0.f;

	// move ourselves to camera's location
	AddActorWorldOffset(Delta);

	// move VRRoot in opposite direction to keep the camera stationary
	VRRoot->AddWorldOffset(-Delta);
}
