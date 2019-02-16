// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	if (ensure(VRRoot != nullptr))
	{
		VRRoot->SetupAttachment(GetRootComponent());
	}

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	if (ensure(Camera != nullptr))
	{
		Camera->SetupAttachment(VRRoot);
	}
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ensure((Camera != nullptr) && (VRRoot != nullptr)))
	{
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
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Forward", this, &AVRCharacter::OnMoveForward);
	PlayerInputComponent->BindAxis("Right", this, &AVRCharacter::OnMoveRight);
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
