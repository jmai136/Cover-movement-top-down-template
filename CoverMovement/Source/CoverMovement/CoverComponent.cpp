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

float UCoverComponent::GetDistanceBetweenShapeComponentLocationByVelocityWithDiameter(const FVector2D CoverCharacterShapeComponentLocation, FVector& OutCoverCharacterShapeComponentLocation, const FVector2D CoverCharacterVelocityNormalised, const float CoverCharacterShapeComponentDiameter, const float ShapeComponentHalfHeight, FVector& EndTraceLocation)
{
	OutCoverCharacterShapeComponentLocation = FVector(CoverCharacterShapeComponentLocation.X, CoverCharacterShapeComponentLocation.Y, ShapeComponentHalfHeight);
	FVector2D EndTrace2D = CoverCharacterShapeComponentLocation + (CoverCharacterVelocityNormalised * CoverCharacterShapeComponentDiameter);

	EndTraceLocation = FVector(EndTrace2D.X, EndTrace2D.Y, ShapeComponentHalfHeight);

	if (GEngine && GetWorld() && bToggleDebugCover)
	{
		DrawDebugDirectionalArrow(GetWorld(), OutCoverCharacterShapeComponentLocation, EndTraceLocation, 10.f, FColor::FromHex("FF7BD0FF"), false, .025f, 0U, 5.f);
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("FF7BD0FF"), FString::Printf(TEXT("A: %f"), FVector::Dist(OutCoverCharacterShapeComponentLocation, EndTraceLocation)));
	}

	return FVector::Dist(OutCoverCharacterShapeComponentLocation, EndTraceLocation);
}

float UCoverComponent::GetDistanceFromClosestPointOnSplineToEndTraceLocation(const FVector EndTraceLocation, const float CoverCharacterShapeComponentDiameter, const float ShapeComponentHalfHeight, FVector& OutClosestSplinePointToEndTraceLocation)
{
	OutClosestSplinePointToEndTraceLocation = OtherCoverPathSplineComponent->FindLocationClosestToWorldLocation(EndTraceLocation, ESplineCoordinateSpace::World);
	OutClosestSplinePointToEndTraceLocation = FVector(OutClosestSplinePointToEndTraceLocation.X, OutClosestSplinePointToEndTraceLocation.Y, ShapeComponentHalfHeight);
	FVector EndTraceFromClosestSplinePoint = OutClosestSplinePointToEndTraceLocation + (FVector(EndTraceLocation - OutClosestSplinePointToEndTraceLocation).GetSafeNormal() * CoverCharacterShapeComponentDiameter);
	EndTraceFromClosestSplinePoint = FVector(EndTraceFromClosestSplinePoint.X, EndTraceFromClosestSplinePoint.Y, ShapeComponentHalfHeight);

	if (GEngine && GetWorld() && bToggleDebugCover)
	{
		DrawDebugDirectionalArrow(GetWorld(), OutClosestSplinePointToEndTraceLocation, EndTraceFromClosestSplinePoint, 10.f, FColor::FromHex("9DBC00FF"), false, .025f, 0U, 5.f);
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("9DBC00FF"), FString::Printf(TEXT("B: %f"), FVector::Dist(EndTraceFromClosestSplinePoint, OutClosestSplinePointToEndTraceLocation)));
	}

	return FVector::Dist(EndTraceFromClosestSplinePoint, OutClosestSplinePointToEndTraceLocation);
}

float UCoverComponent::GetDistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation(const FVector OutClosestSplinePointToEndTraceLocation, const FVector OutCoverCharacterShapeComponentLocation)
{
	if (GEngine && GetWorld() && bToggleDebugCover)
		DrawDebugDirectionalArrow(GetWorld(), OutCoverCharacterShapeComponentLocation, OutClosestSplinePointToEndTraceLocation, 10.f, FColor::FromHex("0849BCFF"), false, .025f, 0U, 5.f);

	return FVector::Dist(OutCoverCharacterShapeComponentLocation, OutClosestSplinePointToEndTraceLocation);
}

// When bringing everything to C++, have these two methods be overloaded.
bool UCoverComponent::ConstrainMovementToSplineKeyboardInput(const FVector DirectionBasedOnCameraRotation, const float ActionValue, FVector2D CoverCharacterShapeComponentLocation, FVector2D CoverCharacterVelocityNormalised, const float ShapeComponentDiameter, const float ShapeComponentHalfHeight, FVector& WorldDirection, float& ScaleValue)
{
	FVector OutCoverCharacterShapeComponentLocation;
	FVector EndTraceLocation;
	FVector OutClosestSplinePointToEndTraceLocation;

	float DistanceBetweenShapeComponentLocationByActorForwardVectorWithDiameter = GetDistanceBetweenShapeComponentLocationByVelocityWithDiameter(CoverCharacterShapeComponentLocation, OutCoverCharacterShapeComponentLocation, CoverCharacterVelocityNormalised, ShapeComponentDiameter, ShapeComponentHalfHeight, EndTraceLocation);
	float DistanceFromClosestPointOnSplineToEndTraceLocation = GetDistanceFromClosestPointOnSplineToEndTraceLocation(EndTraceLocation, ShapeComponentDiameter, ShapeComponentHalfHeight, OutClosestSplinePointToEndTraceLocation);
	float DistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation = GetDistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation(OutClosestSplinePointToEndTraceLocation, OutCoverCharacterShapeComponentLocation);

	float Hypotenuse = UKismetMathLibrary::Hypotenuse(DistanceBetweenShapeComponentLocationByActorForwardVectorWithDiameter, DistanceFromClosestPointOnSplineToEndTraceLocation);

	FVector DirectionFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation = (OutClosestSplinePointToEndTraceLocation - OutCoverCharacterShapeComponentLocation).GetSafeNormal();

	bool bIsCoverCharacterInRangeOfSpline = (DistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation <= Hypotenuse);

	if (bIsCoverCharacterInRangeOfSpline)
	{
		WorldDirection = DirectionBasedOnCameraRotation;
		ScaleValue = ActionValue;
	}
	else
	{
		WorldDirection = DirectionFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation;
		ScaleValue = DistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation - Hypotenuse;
	}

	if (GEngine && bToggleDebugCover)
	{
		UWorld* World = GetWorld();

		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("0849BCFF"), FString::Printf(TEXT("C: %f, C (current value): %f, direction from C to A: %s"), Hypotenuse, DistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation, *DirectionFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation.ToCompactString()));

		FString ComparisonOperator =
			(DistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation < Hypotenuse) ? " < " :
			(DistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation == Hypotenuse) ? " == " :
			(DistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation > Hypotenuse) ? " > " : "";

		FVector CoverCharacterVelocity = FVector(CoverCharacterVelocityNormalised.X, CoverCharacterVelocityNormalised.Y, 0);

		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("#312FBCFF"),
			FString::Printf(TEXT("Distance from spline point closest to end location (normalised velocity) to current location: %f %s Hypotenuse: %f\nWorld direction: %s, Scale value: %f\nCover character velocity normalised: %s"), DistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation, *ComparisonOperator, Hypotenuse, *WorldDirection.ToCompactString(), ScaleValue, *CoverCharacterVelocity.ToCompactString()));
	}

	return  bIsCoverCharacterInRangeOfSpline;
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