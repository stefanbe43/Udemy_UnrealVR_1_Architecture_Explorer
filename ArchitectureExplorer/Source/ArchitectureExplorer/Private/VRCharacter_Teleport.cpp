#include "VRCharacter.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

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
	// if that happens, we would fail below assert and also never fade back in
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

	if (!ensure(RightMotionControllerComponent != nullptr))
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

	// old code that puts teleport location to where I look instead of where I point
	//FVector Start = Camera->GetComponentLocation();
	//FVector End = Start + MaxTeleportDistance_UNUSED * Camera->GetForwardVector().GetSafeNormal();

	// set to true to use old code
	bool bUseLinetraceInsteadOfProjectileTrace = false;

	FVector PointDirection = RightMotionControllerComponent->GetForwardVector();
	if (bUseLinetraceInsteadOfProjectileTrace)
	{
		// rotate the controller direction a bit for comfort with the old-style linetrace teleportation
		PointDirection = PointDirection.RotateAngleAxis(30, RightMotionControllerComponent->GetRightVector());
	}
	PointDirection = PointDirection.GetSafeNormal();

	// trace from our character's location to 100 meters out in direction where we look
	// adding small 5cm step away in Start to avoid self-colliding with the controller mesh on Oculus Rift
	FVector Start = RightMotionControllerComponent->GetComponentLocation() + 5.0f * PointDirection;
	FVector End = Start + MaxTeleportDistance_UNUSED * PointDirection;

	if (bUseLinetraceInsteadOfProjectileTrace)
	{
		// do the linetrace
		auto bLineTraceFoundTarget = World->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (!bLineTraceFoundTarget)
		{
			return false;
		}
	}
	else
	{
		// do projectile trace
		FPredictProjectilePathParams PredictParams(
			TeleportProjectileRadius,					// InProjectileRadius
			Start,										// InStartLocation
			TeleportProjectileSpeed * PointDirection,	// InLaunchVelocity
			TeleportSimulationTime,						// InMaxSimTime
			ECollisionChannel::ECC_Visibility,			// InTraceChannel
			this										// ActorToIgnore
		);
		PredictParams.DrawDebugType = EDrawDebugTrace::ForOneFrame;
		FPredictProjectilePathResult PredictResult;

		auto bProjectilePathFoundTarget = UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);

		if (!bProjectilePathFoundTarget)
		{
			return false;
		}

		HitResult = PredictResult.HitResult;
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
