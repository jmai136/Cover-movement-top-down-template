// Fill out your copyright notice in the Description page of Project Settings.


#include "CoverComponent.h"
#include "CoverPathSplineComponent.h"
#include "Components/ShapeComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UCoverComponent::UCoverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

uint8 UCoverComponent::GetIsInCover()
{
	return bIsInCover;
}

bool UCoverComponent::IsOverlappingCover()
{
	// If doesn't get any overlapping actors, array doesn't have any values so returns an error if you actually try to pass IsOverlappingActor
	return (Result.IsEmpty()) ? false : GetOwner()->IsOverlappingActor(Result[0]);
}

bool UCoverComponent::HasCoverSplinePath()
{
	if (Result[0]->FindComponentByClass(UCoverPathSplineComponent::StaticClass()) != nullptr)
	{
		OtherCoverPathSplineComponent = Cast<UCoverPathSplineComponent>(Result[0]->FindComponentByClass(UCoverPathSplineComponent::StaticClass()));
		ShapeComponent = Cast<UShapeComponent>(Result[0]->FindComponentByClass(UShapeComponent::StaticClass()));
		check(OtherCoverPathSplineComponent && ShapeComponent);

		return true;
	}
	else
	{
		return false;
	}
}

void UCoverComponent::TakeCover(bool& bOrientRotationToMovement)
{
	if (GetIsInCover())
	{
		SetIsInCover(false);
		bOrientRotationToMovement = true;
	}
	else
	{
		GetOwner()->GetOverlappingActors(Result, UCoverPathSplineComponent::StaticClass());

		if (IsOverlappingCover() && HasCoverSplinePath()) {
			SetIsInCover(true);
			bOrientRotationToMovement = false;
		}
	}
}

void UCoverComponent::SetIsInCover(uint8 SetIsInCover)
{
	bIsInCover = SetIsInCover;
}

// https://forums.unrealengine.com/t/get-distance-at-location-along-spline/75633
float UCoverComponent::FindDistanceAlongSplineClosestToWorldLocation(const FVector& WorldLocation)
{
	float WorldLocationInputKey = OtherCoverPathSplineComponent->FindInputKeyClosestToWorldLocation(WorldLocation),
		DistanceAlongSplineWorldLocation = OtherCoverPathSplineComponent->GetDistanceAlongSplineAtSplinePoint(FGenericPlatformMath::TruncToInt(WorldLocationInputKey)),
		DistancePlusOneAlongSplineWorldLocation = OtherCoverPathSplineComponent->GetDistanceAlongSplineAtSplinePoint(FGenericPlatformMath::TruncToInt(WorldLocationInputKey + 1.f)),
		DifferenceDistancePlusOneAndDistance = DistancePlusOneAlongSplineWorldLocation - DistanceAlongSplineWorldLocation,
		DifferenceDistanceTimesDifferenceWorldLocation = DifferenceDistancePlusOneAndDistance * (WorldLocationInputKey - FGenericPlatformMath::TruncToInt(WorldLocationInputKey));

	return DistanceAlongSplineWorldLocation + DifferenceDistanceTimesDifferenceWorldLocation;
}

// Get where the actor will be, not where the actor is because ultimately you want to determine whether the location you're going to is viable, current location the player is at doesn't tell you that.
// The source code for calculating AddMovementInput is: ControlInputVector += (WorldDirection * ScaleValue)
// Doesn't need to be exact, bForce will add more force, this is base calculation done.
FVector UCoverComponent::GetPredictedLocation()
{
	return GetOwner()->GetActorLocation() + (PredictedWorldDirection * PredictedScaleValue);
}

float UCoverComponent::UpdateDestination()
{
	PredictedLocation = GetPredictedLocation();
	FVector SplinePointLocationClosestToPredicted = OtherCoverPathSplineComponent->FindLocationClosestToWorldLocation(PredictedLocation, ESplineCoordinateSpace::World);

	if (GetWorld() && bToggleDebugCover)
	{
		UWorld* World = GetWorld();
		FHitResult HitResult;

		World->LineTraceSingleByChannel(HitResult, PredictedLocation, SplinePointLocationClosestToPredicted, ECollisionChannel::ECC_Visibility, FCollisionQueryParams::DefaultQueryParam, FCollisionResponseParams::DefaultResponseParam);

		DrawDebugLine(World, PredictedLocation, SplinePointLocationClosestToPredicted, FColor::FromHex("#FFF500F"), false, -1, 0, 2.0f);

		DrawDebugPoint(World, PredictedLocation, 10.f, FColor::FromHex("#FFD89CFF"));
		DrawDebugPoint(World, SplinePointLocationClosestToPredicted, 10.f, FColor::FromHex("#2C2677FF"));

		DrawDebugSphere(World, PredictedLocation, 20.f, 12, FColor::FromHex("#FFD89CFF"));
		DrawDebugSphere(World, SplinePointLocationClosestToPredicted, 20.f, 12, FColor::FromHex("#2C2677FF"));
	}

	return FVector::Dist(PredictedLocation, SplinePointLocationClosestToPredicted);
}

FVector UCoverComponent::GetClosestPointOnOtherCoverPathSplineComponentOwnerShapeComponent()
{
	if (GEngine && bToggleDebugCover)
		GEngine->AddOnScreenDebugMessage(0, 0, FColor::FromHex("#9DBC00"), FString::Printf(TEXT("Primitive component: %s"), *ShapeComponent->GetName()));

	FVector OutPointOnBody;
	ShapeComponent->GetClosestPointOnCollision(PredictedLocation, OutPointOnBody);
	return OutPointOnBody;
}

float UCoverComponent::GetDistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt()
{
	FVector StartLocation = GetClosestPointOnOtherCoverPathSplineComponentOwnerShapeComponent();
	ClosestPointOnOtherCoverPathSplineOfComponentOwnerShapeComponentToPredictedValue = OtherCoverPathSplineComponent->FindLocationClosestToWorldLocation(StartLocation, ESplineCoordinateSpace::World);

	if (GetWorld() && bToggleDebugCover)
	{
		UWorld* World = GetWorld();
		FHitResult HitResult;

		World->LineTraceSingleByChannel(HitResult, StartLocation, ClosestPointOnOtherCoverPathSplineOfComponentOwnerShapeComponentToPredictedValue, ECollisionChannel::ECC_Visibility, FCollisionQueryParams::DefaultQueryParam, FCollisionResponseParams::DefaultResponseParam);

		DrawDebugLine(World, StartLocation, ClosestPointOnOtherCoverPathSplineOfComponentOwnerShapeComponentToPredictedValue, FColor::FromHex("#66FFFF"), false, -1, 0, 2.0f);

		DrawDebugPoint(World, StartLocation, 10.f, FColor::FromHex("#9DBC00"));
		DrawDebugPoint(World, ClosestPointOnOtherCoverPathSplineOfComponentOwnerShapeComponentToPredictedValue, 10.f, FColor::FromHex("#D2D1CD"));

		DrawDebugSphere(World, StartLocation, 20.f, 12, FColor::FromHex("#9DBC00"));
		DrawDebugSphere(World, ClosestPointOnOtherCoverPathSplineOfComponentOwnerShapeComponentToPredictedValue, 20.f, 12, FColor::FromHex("#D2D1CD"));
	}

	return FVector::Dist(StartLocation, ClosestPointOnOtherCoverPathSplineOfComponentOwnerShapeComponentToPredictedValue);
}

// When bringing everything to C++, have these two methods be overloaded.
bool UCoverComponent::ConstrainMovementToSplineKeyboardInput(const FVector DirectionBasedOnCameraRotation, const float ActionValue, FVector& WorldDirection, float& ScaleValue)
{
	PredictedWorldDirection = WorldDirection;
	PredictedScaleValue = ActionValue;

	float DistanceBetweenPredictedLocationAndClosestPointOnSplineToItFromLineTrace = UpdateDestination();
	float DistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt = GetDistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt();

	bool bDistanceCondition = DistanceBetweenPredictedLocationAndClosestPointOnSplineToItFromLineTrace <= DistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt;

	if (bDistanceCondition)
	{
		WorldDirection = DirectionBasedOnCameraRotation;
		ScaleValue = ActionValue;
	}
	else
	{
		// By getting the rotation from the predicted location and the closest splne point to the closest point on the shape component, the player will always end up going in that direction if they are past the distance condition.
		WorldDirection = UKismetMathLibrary::FindLookAtRotation(PredictedLocation, ClosestPointOnOtherCoverPathSplineOfComponentOwnerShapeComponentToPredictedValue).Vector();
		// If the WorldDirection always return a unit vector, the ScaleValue determines the intensity of the movement.
		ScaleValue = DistanceBetweenPredictedLocationAndClosestPointOnSplineToItFromLineTrace - DistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt;
	}

	if (GEngine && bToggleDebugCover)
	{
		FString ComparisonOperator =
			(DistanceBetweenPredictedLocationAndClosestPointOnSplineToItFromLineTrace < DistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt) ? " < " :
			(DistanceBetweenPredictedLocationAndClosestPointOnSplineToItFromLineTrace == DistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt) ? " == " :
			(DistanceBetweenPredictedLocationAndClosestPointOnSplineToItFromLineTrace > DistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt) ? " > " : "";

		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("#003277FF"),
			FString::Printf(TEXT("Distance between predicted location and spline point (predicted location): %f %s Distance between collision point (predicted location) and spline point (predicted location): %f\nWorld direction: %s, Scale value: %f"), DistanceBetweenPredictedLocationAndClosestPointOnSplineToItFromLineTrace, *ComparisonOperator, DistanceBetweenCollisionPointNearestToPredictedLocationAndSplinePointClosestToIt, *WorldDirection.ToCompactString(), ScaleValue));
	}

	return bDistanceCondition;
}

void UCoverComponent::ConstrainMovementToSplineMouseInput(const FVector& ClickedDest, float& DistanceAlongSpline, FVector& LocationAtFindLocationClosestToWorldLocation, FRotator& RotationFromFindTangentClosestToWorldLocation)
{
	DistanceAlongSpline = FindDistanceAlongSplineClosestToWorldLocation(ClickedDest);
	LocationAtFindLocationClosestToWorldLocation = OtherCoverPathSplineComponent->FindLocationClosestToWorldLocation(ClickedDest, ESplineCoordinateSpace::World);
	RotationFromFindTangentClosestToWorldLocation = OtherCoverPathSplineComponent->FindTangentClosestToWorldLocation(ClickedDest, ESplineCoordinateSpace::World).Rotation();

	if (GEngine && bToggleDebugCover)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Yellow,
			FString::Printf(TEXT("Get location at location closest to world location: %s\nRotation from tangent closest to world location: %s\nDistance along spline: %f"), *LocationAtFindLocationClosestToWorldLocation.ToCompactString(), *RotationFromFindTangentClosestToWorldLocation.ToCompactString(), DistanceAlongSpline));

		DrawDebugPoint(GetOwner()->GetWorld(), LocationAtFindLocationClosestToWorldLocation, 1.0f, FColor::White, true, -1.0f, -1);
	}
}

FVector UCoverComponent::GetCoverStrafeDirectionAllAxis()
{
	return CoverStrafeDirectionAllAxis;
}

void UCoverComponent::SetCoverStrafeDirectionAllAxis(UCharacterMovementComponent* CharacterMovementComponent, FVector& CoverStrafeDirectionAllAxisValue, FIntVector& CoverStrafeDirectionSign)
{
	CoverStrafeDirectionAllAxisValue = UKismetMathLibrary::LessLess_VectorRotator(CharacterMovementComponent->Velocity, GetOwner()->GetActorRotation());
	CoverStrafeDirectionSign = FIntVector((int)FMath::Sign(CoverStrafeDirectionAllAxisValue.X), (int)FMath::Sign(CoverStrafeDirectionAllAxisValue.Y), (int)FMath::Sign(CoverStrafeDirectionAllAxisValue.Z));

	if (GEngine && bToggleDebugCover)
		GEngine->AddOnScreenDebugMessage(0, 0, FColor::Cyan, FString::Printf(TEXT("Cover strafe direction all axis: %s\nCover strafe direction sign: %s"), *CoverStrafeDirectionAllAxisValue.ToCompactString(), *CoverStrafeDirectionSign.ToString()));
}