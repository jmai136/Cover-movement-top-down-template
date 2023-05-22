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

	UPROPERTY(EditDefaultsOnly, Category = "Cover debug")
		bool bAutomaticallyCreateCoverPathSplineOnBeginPlay = true;

	UPROPERTY(EditDefaultsOnly, Category = "Cover debug")
		bool bToggleDebugCoverPathSpline = false;

	UPROPERTY(EditDefaultsOnly)
		float Tension = 0.5f;

	UPROPERTY(EditDefaultsOnly)
		float CurveThresholdNormal = 0.5f;

	UFUNCTION(BlueprintCallable)
		void CreateSplineAroundCover(UStaticMeshComponent* StaticMeshComponent);

private:
	virtual void BeginPlay() override;


	int Orientation(FVector P, FVector Q, FVector R);

	TArray<FVector> JarvisMarchAlgorithm(TArray<FVector> Verteces);

	TArray<FVector> SetMidpoints(TArray<FVector> CornerPoints);

	TArray<FVector> GetPointsCreateCurves(TArray<FVector> Midpoints);
};