#include "VRCharacter.h"
#include "Components/PostProcessComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Curves/CurveFloat.h"

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

	PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
}

void AVRCharacter::UpdateBlinkerRadius()
{
	if (RadiusVsVelocity == nullptr)
	{
		return;
	}

	if (BlinkerMaterialInstance == nullptr)
	{
		return;
	}

	float BlinkerRadius = 0.0f;
	float MySpeed = GetVelocity().Size();

	BlinkerRadius = RadiusVsVelocity->GetFloatValue(MySpeed);

	//UE_LOG(LogTemp, Warning, TEXT("AVRCharacter::UpdateBlinkerRadius() setting Radius to %1.4f"), BlinkerRadius);
	BlinkerMaterialInstance->SetScalarParameterValue(FName(TEXT("Radius")), BlinkerRadius);
}

void AVRCharacter::UpdateBlinkerCenter()
{
	if (BlinkerMaterialInstance == nullptr)
	{
		return;
	}

	FVector2D Center = CalculateBlinkerCenter();

	BlinkerMaterialInstance->SetVectorParameterValue(FName(TEXT("Center")), FLinearColor(Center.X, Center.Y, 0.0f, 0.0f));
}

FVector2D AVRCharacter::CalculateBlinkerCenter() const
{
	const FVector2D CenterDefault = FVector2D(0.5f, 0.5f);

	if (!ensure(Camera != nullptr))
	{
		return CenterDefault;
	}

	auto PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController == nullptr)
	{
		return CenterDefault;
	}

	auto MyNormalizedVelocity = GetVelocity().GetSafeNormal();
	if (MyNormalizedVelocity.IsNearlyZero())
	{
		// the Size of GetSafeNormal() will be 1.0, except when the velocity
		// vector is near zero in which case it returns 0.0
		//
		// if we have a near-zero velocity, let's center our blinker

		return CenterDefault;
	}

	auto CameraLocation = Camera->GetComponentLocation();

	FVector PointInOurDirectionOfMovement;
	if (FVector::DotProduct(Camera->GetForwardVector(), MyNormalizedVelocity) > 0) // test is going backwards
	{
		// we go some arbitrary 1000cm in front of the camera location
		PointInOurDirectionOfMovement = CameraLocation + 1000.0f*MyNormalizedVelocity;
	}
	else
	{
		// we go some arbitrary 1000cm behind the camera location (since we are moving backwards)
		PointInOurDirectionOfMovement = CameraLocation - 1000.0f*MyNormalizedVelocity;
	}

	FVector2D OutScreenLocation;
	bool bFoundScreenLocation;
	// project the PointInOurDirectionOfMovement back onto our screen to get a 2d screen location
	bFoundScreenLocation = PlayerController->ProjectWorldLocationToScreen(PointInOurDirectionOfMovement, OutScreenLocation);
	if (!bFoundScreenLocation)
	{
		// this happens if projection is outside the screen
		// we are fine if this happens because the center will just be offscreen, which is fine

		// so no special handling code here
	}

	int32 SizeX, SizeY;
	PlayerController->GetViewportSize(SizeX, SizeY);
	auto NewCenter = OutScreenLocation / FVector2D(SizeX, SizeY); // component-wise division; turns coordinate into range 0.0 to 1.0

	return NewCenter;
}
