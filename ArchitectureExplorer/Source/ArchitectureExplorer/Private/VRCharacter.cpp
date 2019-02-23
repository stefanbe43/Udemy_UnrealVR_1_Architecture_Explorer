// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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

void AVRCharacter::SetupBlinkerPostprocessingEffect()
{
	if (!ensure(PostProcessComponent != nullptr))
	{
		return;
	}

	if (BlinkerMaterialBase == nullptr)
	{
		return;
	}

	BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
	if (BlinkerMaterialInstance == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AVRCharacter::SetupBlinkerPostprocessingEffect() unable to create the blinker dynamic material"));
		return;
	}

	PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialBase);

	// set some starting value - this will be dynamically adjusted elsewhere
	BlinkerMaterialInstance->SetScalarParameterValue(FName(TEXT("Radius")), 0.4f);
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// adjust our position based on how much we walked in our space
	MovePawnToVRCamera();

	// update teleportation marker
	MoveDestinationMarkerByLineTrace();
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

bool AVRCharacter::bFindTeleportDestination(FVector &OutLocation)
{
	auto World = GetWorld();

	if (!ensure(World != nullptr))
	{
		return false;
	}

	if (!ensure(Camera != nullptr))
	{
		return false;
	}

	// don't do line traces if we have a teleport in progress
	// also stop showing the teleportation marker
	if (GetWorldTimerManager().IsTimerActive(TeleportFadeTimerHandle))
	{
		DestinationMarker->SetVisibility(false);
		return false;
	}

	// output parameter
	FHitResult HitResult;

	// trace from our character's location to 100 meters out in direction where we look
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + MaxTeleportDistance * Camera->GetForwardVector().GetSafeNormal();

	// do the linetrace
	auto bLineTraceFoundTarget = World->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
	if (!bLineTraceFoundTarget)
	{
		return false;
	}

	// output parameter
	FVector NavLocation;

	// and project to navigation mesh
	bool bNavLocationFound = false;
	bNavLocationFound = bProjectTeleportToNavigation(OutLocation, HitResult.Location);

	return bNavLocationFound;
}

void AVRCharacter::MoveDestinationMarkerByLineTrace()
{
	if (!ensure(DestinationMarker != nullptr))
	{
		return;
	}

	// output parameter
	FVector Location;

	// if successful, move DestinationMarker to where linetrace hit projected to navigation mesh and unhide it
	if (bFindTeleportDestination(Location))
	{
		//UE_LOG(LogTemp, Warning, TEXT("AVRCharacter::MoveDestinationMarkerByLineTrace() target found at %s"), *(NavLocation.ToString()));
		DestinationMarker->SetWorldLocation(Location);
		DestinationMarker->SetVisibility(true);
	}
	// otherwise hide the DestinationMarker
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("AVRCharacter::MoveDestinationMarkerByLineTrace() target not found trace=%d navloc=%d"), bLineTraceFoundTarget, bNavLocationFound);
		DestinationMarker->SetVisibility(false);
	}
}

bool AVRCharacter::bProjectTeleportToNavigation(FVector &OutLocation, FVector InLocation)
{
	auto World = GetWorld();

	if (!ensure(World != nullptr))
	{
		return false;
	}

	auto NavigationSystem = Cast<UNavigationSystemV1>(World->GetNavigationSystem());

	if (!ensure(NavigationSystem != nullptr))
	{
		return false;
	}

	FNavLocation OutNavLocation;

	auto bNavLocationFound = NavigationSystem->ProjectPointToNavigation(InLocation, OutNavLocation, TeleportProjectionExtent);

	OutLocation = OutNavLocation.Location;

	return bNavLocationFound;
}

void AVRCharacter::BeginTeleport()
{
	auto PlayerController = Cast<APlayerController>(GetController());
	auto CapsuleComponent = GetCapsuleComponent();

	if (!ensure(DestinationMarker != nullptr))
	{
		return;
	}

	if (!ensure((PlayerController != nullptr) && (PlayerController->PlayerCameraManager != nullptr)))
	{
		return;
	}

	if (!ensure(CapsuleComponent != nullptr))
	{
		return;
	}

	// record teleport location
	TeleportLocation = DestinationMarker->GetComponentLocation();
	TeleportLocation.Z += CapsuleComponent->GetScaledCapsuleHalfHeight(); // offset up by our capsule so we don't teleport into the ground

	// fade out
	PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, TeleportFadeOut, FLinearColor::Black);

	// setup timer to finish the teleportation
	GetWorldTimerManager().SetTimer(TeleportFadeTimerHandle, this, &AVRCharacter::FinishTeleport, TeleportFadeOut);

}

void AVRCharacter::FinishTeleport()
{
	auto PlayerController = Cast<APlayerController>(GetController());

	// TODO I am unsure if we can loose our playercontroller in between BeginTeleport and FinishTeleport (like if we die)
	// if that happens, we would crash below assert and also never fade back in
	if (!ensure((PlayerController != nullptr) && (PlayerController->PlayerCameraManager != nullptr)))
	{
		return;
	}

	// camera has finished fading out
	// now we can teleport to new location
	SetActorLocation(TeleportLocation);

	// fade back in
	PlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, TeleportFadeIn, FLinearColor::Black);
}
