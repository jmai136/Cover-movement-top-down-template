// Fill out your copyright notice in the Description page of Project Settings.

#include "CoverPathSplineComponent.h"
#include "DrawDebugHelpers.h"

UCoverPathSplineComponent::UCoverPathSplineComponent()
{
    SetClosedLoop(true, true);
    bDrawDebug = true;
}

void UCoverPathSplineComponent::OnCreateSplineAroundCover(UStaticMesh* StaticMesh, const TArray<FVector>& MeshVerteces)
{
    // Don't add the mesh verteces, actually calculate some math.
    ClearSplinePoints();

    // TArray<FVector> SplinePoints = GetPointsCreateCurves(GetMidpoints(MeshVerteces));
    TArray<FVector> SplinePoints = GetMidpoints(MeshVerteces);
    SetSplinePoints(SplinePoints, ESplineCoordinateSpace::Local);

    /*
    if (GetOwner() && GEngine) {
        GEngine->AddOnScreenDebugMessage(2, 20.f, FColor::Turquoise, FString::Printf(TEXT("Parent: %s, Parent location: %s"),
            *GetOwner()->GetName(), *GetOwner()->GetActorLocation().ToCompactString()));

        DrawDebugPoint(GetWorld(), GetComponentLocation(), 3.5f, FColor::Red, true);
        DrawDebugPoint(GetWorld(), GetOwner()->GetActorLocation(), 3.5f, FColor::White, true);
    }
    */

    // Modify in GetPointsCreateCurves, depending on the angle between each of the points, determine whether it'll be a curve or not
    if (SplinePoints.Num() <= 12)
        for (const FVector& Point : SplinePoints)
            SetSplinePointType(SplinePoints.Find(Point), ESplinePointType::Linear);

    // UKismetProcedualMeshLibrary::GetSectionFromStaticMesh
}

TArray<FVector> UCoverPathSplineComponent::GetMidpoints(TArray<FVector> CornerPoints)
{
    // TSet<FVector> AutomaticRemoveDuplicatePoints;
    TArray<FVector> UniquePoints;

    for (const FVector& CurrentVector : CornerPoints)
    {
        bool bFoundDuplicate = false;
        for (const FVector& UniqueVector : UniquePoints)
        {
            if (FMath::IsNearlyEqual(CurrentVector.X, UniqueVector.X, KINDA_SMALL_NUMBER) && FMath::IsNearlyEqual(CurrentVector.Y, UniqueVector.Y, KINDA_SMALL_NUMBER))
            {
                bFoundDuplicate = true;
                break;
            }
        }

        if (!bFoundDuplicate)
        {
            // Find the average height of all vectors at the same X and Y location
            float TotalHeight = 0.f;
            int32 NumMatchingPoints = 0;
            for (const FVector& MatchingVector : CornerPoints)
            {
                if (FMath::IsNearlyEqual(CurrentVector.X, MatchingVector.X, KINDA_SMALL_NUMBER) && FMath::IsNearlyEqual(CurrentVector.Y, MatchingVector.Y, KINDA_SMALL_NUMBER))
                {
                    TotalHeight += MatchingVector.Z;
                    NumMatchingPoints++;
                }
            }

            float HalfwayHeight = TotalHeight / (NumMatchingPoints)+0.5f * GetOwner()->GetTransform().GetScale3D().Z;

            // Add the new spline point at the halfway height
            FVector HalfwayVector = CurrentVector;
            HalfwayVector.Z = HalfwayHeight;
            UniquePoints.AddUnique(HalfwayVector);
        }
    }

    /*
    for (const FVector& UniqueVector : UniquePoints)
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("Unique vector: %s"), *UniqueVector.ToCompactString()));
            DrawDebugPoint(GetWorld(), UniqueVector, 3.5f, FColor::Green, true);
        }
    */

    return UniquePoints;
}

TArray<FVector> UCoverPathSplineComponent::GetPointsCreateCurves(TArray<FVector> Midpoints)
{
    TArray<FVector> SplinePoints;
    if (Midpoints.Num() < 2)
        return SplinePoints;

    SplinePoints.Add(Midpoints[0]);

    for (const FVector& CurrentVector : Midpoints)
    {
        int CurrentVectorIndex;
        Midpoints.Find(CurrentVector, CurrentVectorIndex);

        const FVector& PrevVector = (CurrentVectorIndex - 1 > 0) ? (Midpoints[CurrentVectorIndex - 1] - CurrentVector).GetSafeNormal() : CurrentVector;
        const FVector& NextVector = (CurrentVectorIndex + 1 < Midpoints.Num()) ? (Midpoints[CurrentVectorIndex + 1] - CurrentVector).GetSafeNormal() : Midpoints.Last();

        double AngleBetweenTwoPoints = FVector::DotProduct(PrevVector, NextVector);

        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-0, 20.f, FColor::Green, FString::Printf(TEXT("Angle between points: %f, Previous vector: %s, Next vector: %s"), AngleBetweenTwoPoints, *PrevVector.ToCompactString(), *NextVector.ToCompactString()));

        // Dot products will only ever range from -1 to 0 to 1
        // -1: Two vectors pointing in opposite directions AKA 180°
        // 0: Two vectors perpendicular to each other AKA 90°
        // 1: Two vectors pointing in the same direction AKA 0°
        if ((float)AngleBetweenTwoPoints < CurveThresholdNormal)
            SplinePoints.AddUnique(PrevVector + (CurrentVector - PrevVector) * 0.5f);
    }

    return SplinePoints;
}