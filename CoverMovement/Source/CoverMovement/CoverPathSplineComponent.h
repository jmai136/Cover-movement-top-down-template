// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "CoverPathSplineComponent.generated.h"

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class UCoverPathSplineComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UCoverPathSplineComponent();

	UPROPERTY(EditDefaultsOnly)
		float Tension = 0.5f;

	UPROPERTY(EditDefaultsOnly)
		float CurveThresholdNormal = 0.5f;

	UFUNCTION(BlueprintCallable)
		void OnCreateSplineAroundCover(UStaticMesh* StaticMesh, const TArray<FVector>& MeshVerteces);

private:
	TArray<FVector> GetMidpoints(TArray<FVector> CornerPoints);

	TArray<FVector> GetPointsCreateCurves(TArray<FVector> Midpoints);
};