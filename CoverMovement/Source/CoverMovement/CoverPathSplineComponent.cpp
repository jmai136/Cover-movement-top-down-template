// Fill out your copyright notice in the Description page of Project Settings.

#include "CoverPathSplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"

UCoverPathSplineComponent::UCoverPathSplineComponent()
{
    SetClosedLoop(true, true);
    bDrawDebug = true;
}

void UCoverPathSplineComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner()->FindComponentByClass(UStaticMeshComponent::StaticClass()) && bAutomaticallyCreateCoverPathSplineOnBeginPlay)
        CreateSplineAroundCover(Cast<UStaticMeshComponent>(GetOwner()->FindComponentByClass(UStaticMeshComponent::StaticClass())));
}

void UCoverPathSplineComponent::CreateSplineAroundCover(UStaticMeshComponent* StaticMeshComponent)
{
    UStaticMesh* SM = StaticMeshComponent->GetStaticMesh();
    float HalfwayHeight = GetOwner()->GetTransform().GetScale3D().Z;

    // Don't add the mesh verteces, actually calculate some math.
    ClearSplinePoints();

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;

    UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(SM, 0, 0, Vertices, Triangles, Normals, UVs, Tangents);

    // TArray<FVector> SplinePoints = GetPointsCreateCurves(SetMidpoints(MeshVerteces));
    TArray<FVector> SplinePoints = SetMidpoints(Vertices, HalfwayHeight);
    SetSplinePoints(SplinePoints, ESplineCoordinateSpace::Local);

    if (GetOwner() && GEngine && bToggleDebugCoverPathSpline) {
        GEngine->AddOnScreenDebugMessage(2, 20.f, FColor::Turquoise, FString::Printf(TEXT("Parent: %s, Parent location: %s"),
            *GetOwner()->GetName(), *GetOwner()->GetActorLocation().ToCompactString()));

        DrawDebugPoint(GetWorld(), GetComponentLocation(), 3.5f, FColor::Red, true);
        DrawDebugPoint(GetWorld(), GetOwner()->GetActorLocation(), 3.5f, FColor::White, true);
    }

    // Modify in GetPointsCreateCurves, depending on the angle between each of the points, determine whether it'll be a curve or not
    if (SplinePoints.Num() <= 12)
        for (const FVector& Point : SplinePoints)
            SetSplinePointType(SplinePoints.Find(Point), ESplinePointType::Linear);
}

TArray<FVector> UCoverPathSplineComponent::SetMidpoints(TArray<FVector> CornerPoints, const float HalfwayHeight)
{
    TArray<FVector> UniquePoints;

    float MinZOnMesh = MAX_FLT;

    // Prevents the issue of the mesh having to be symmetrical.
    // The important part is to grab the base, since covers will always have a base and not necessarily be dependent on the top.
    for (const FVector& CurrentVector : CornerPoints)
        if (CurrentVector.Z < MinZOnMesh)
            MinZOnMesh = CurrentVector.Z;

    for (const FVector& CurrentVector : CornerPoints)
    {
        if (FMath::IsNearlyEqual(CurrentVector.Z, MinZOnMesh, KINDA_SMALL_NUMBER))
        {
            bool bFoundDuplicate = false;
            for (const FVector& UniqueVector : UniquePoints)
                if (FMath::IsNearlyEqual(CurrentVector.X, UniqueVector.X, KINDA_SMALL_NUMBER) && FMath::IsNearlyEqual(CurrentVector.Y, UniqueVector.Y, KINDA_SMALL_NUMBER))
                {
                    bFoundDuplicate = true;
                    break;
                }

            if (!bFoundDuplicate)
            {
                FVector HalfwayVector = CurrentVector;
                HalfwayVector.Z = HalfwayHeight;
                UniquePoints.AddUnique(HalfwayVector);
            }
        }
    }

    if (GEngine && bToggleDebugCoverPathSpline)
        for (const FVector& UniqueVector : UniquePoints)
        {
            GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("Unique vector: %s"), *UniqueVector.ToCompactString()));
            DrawDebugPoint(GetWorld(), UniqueVector, 3.5f, FColor::Green, true);
        }

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