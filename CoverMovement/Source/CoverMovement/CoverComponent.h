#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoverComponent.generated.h"

class UCharacterMovementComponent;
class UShapeComponent;
class UCoverPathSplineComponent;

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class UCoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoverComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover spline reference")
		UCoverPathSplineComponent* OtherCoverPathSplineComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover debug")
		bool bToggleDebugCover = false;

	UFUNCTION(BlueprintCallable, Category = "Cover")
		void TakeCover(bool& bOrientRotationToMovement);

	UFUNCTION(BlueprintCallable, Category = "Cover")
		void SetIsInCover(uint8 SetIsInCover);

	UFUNCTION(BlueprintPure, Category = "Cover")
		bool ConstrainMovementToSplineKeyboardInput(const FVector DirectionBasedOnCameraRotation, const float ActionValue, FVector2D CoverCharacterShapeComponentLocation, FVector2D CoverCharacterVelocityNormalised, const float ShapeComponentDiameter, const float ShapeComponentHalfHeight, FVector& WorldDirection, float& ScaleValue);

	UFUNCTION(BlueprintPure, Category = "Cover")
		void ConstrainMovementToSplineMouseInput(const FVector& ClickedDest, float& DistanceAlongSpline, FVector& LocationAtFindLocationClosestToWorldLocation, FRotator& RotationFromGetTangentAtDistanceAlongSpline);

	UFUNCTION(BlueprintPure, Category = "Cover")
		void SetCoverStrafeDirectionAllAxis(UCharacterMovementComponent* CharacterMovementComponent, FVector& CoverStrafeDirectionAllAxisValue, FIntVector& CoverStrafeDirectionSign);

private:
	UShapeComponent* ShapeComponent;

	TArray<AActor*> Result;

	UPROPERTY(BlueprintGetter = GetIsInCover, BlueprintSetter = SetIsInCover)
		uint8 bIsInCover;

	UFUNCTION(BlueprintPure, Category = "Cover")
		uint8 GetIsInCover();

	UFUNCTION(BlueprintPure, Category = "Cover")
		bool IsOverlappingCover();

	UFUNCTION(BlueprintPure, Category = "Cover")
		bool HasCoverSplinePath();

	UFUNCTION(BlueprintPure, Category = "Cover path spline")
		float FindDistanceAlongSplineClosestToWorldLocation(const FVector& WorldLocation);

	float GetDistanceBetweenShapeComponentLocationByVelocityWithDiameter(const FVector2D CoverCharacterShapeComponentLocation, FVector& OutCoverCharacterShapeComponentLocation, const FVector2D CoverCharacterVelocityNormalised, const float CoverCharacterShapeComponentDiameter, const float ShapeComponentHalfHeight, FVector& EndTraceLocation);

	float GetDistanceFromClosestPointOnSplineToEndTraceLocation(const FVector EndTraceLocation, const float CoverCharacterShapeComponentDiameter, const float ShapeComponentHalfHeight, FVector& OutClosestSplinePointToEndTraceLocation);

	float GetDistanceFromOutClosestSplinePointToEndTraceLocationToCurrentCoverCharacterLocation(const FVector OutClosestSplinePointToEndTraceLocation, const FVector OutCoverCharacterShapeComponentLocation);

	UPROPERTY(BlueprintGetter = GetCoverStrafeDirectionAllAxis)
		FVector CoverStrafeDirectionAllAxis;

	UFUNCTION(BlueprintPure, Category = "Cover")
		FVector GetCoverStrafeDirectionAllAxis();
};