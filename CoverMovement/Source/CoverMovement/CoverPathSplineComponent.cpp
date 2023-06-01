#include "CoverPathSplineComponent.h"
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

    // Don't add the mesh verteces, actually calculate some math.
    ClearSplinePoints();

    TArray<FVector> Verteces;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;

    UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(SM, 0, 0, Verteces, Triangles, Normals, UVs, Tangents);

    // TArray<FVector> SplineVerteces = GetVertecesCreateCurves(SetMidVerteces(MeshVerteces));
    TArray<FVector> SplineVerteces = SetMidpoints(Verteces);
    SetSplinePoints(SplineVerteces, ESplineCoordinateSpace::Local);

    /*
    if (GetOwner() && GEngine && bToggleDebugCoverPathSpline) {
        GEngine->AddOnScreenDebugMessage(2, 20.f, FColor::Turquoise, FString::Printf(TEXT("Parent: %s, Parent location: %s"),
            *GetOwner()->GetName(), *GetOwner()->GetActorLocation().ToCompactString()));

        DrawDebugPoint(GetWorld(), GetComponentLocation(), 3.5f, FColor::Red, true);
        DrawDebugPoint(GetWorld(), GetOwner()->GetActorLocation(), 3.5f, FColor::White, true);
    }
    */

    // Modify in GetVertecesCreateCurves, depending on the angle between each of the Verteces, determine whether it'll be a curve or not
    if (SplineVerteces.Num() <= 12)
        for (const FVector& Point : SplineVerteces)
            SetSplinePointType(SplineVerteces.Find(Point), ESplinePointType::Linear);
}

// Inspiration from how to make outlines using inverted convex hull in Blender
// https://www.geeksforgeeks.org/convex-hull-using-jarvis-algorithm-or-wrapping/
// To find orientation of ordered triplet (p, q, r).
    // The function returns following values
    // 0 --> p, q and r are collinear
    // 1 --> Clockwise
    // 2 --> Counterclockwise
int UCoverPathSplineComponent::Orientation(FVector P, FVector Q, FVector R)
{
    int Val = (Q.Y - P.Y) * (R.X - Q.X) - (Q.X - P.X) * (R.Y - Q.Y);
    return (Val == 0) ? 0 : (Val > 0) ? 1 : 2;
}

TArray<FVector> UCoverPathSplineComponent::JarvisMarchAlgorithm(TArray<FVector> Verteces)
{
    if (Verteces.Num() < 3) return Verteces;

    int N = Verteces.Num();

    TArray<FVector> ConvexHull;

    // Find the leftmost point
    // UE follows a left hand coordinate system, so really horizontal is Y
    int Leftmost = 0;
    for (int I = 1; I < N; I++)
        if (Verteces[I].Y < Verteces[Leftmost].Y)
            Leftmost = I;

    int P = Leftmost, Q;

    do
    {
        ConvexHull.Add(Verteces[P]);

        // Search for a point 'q' such that orientation(p, q, x) is counterclockwise for all Verteces 'x'. The idea is to keep track of last visited most counterclock-wise point in q. If any point 'i' is more counterclock-wise than q, then update q.
        Q = (P + 1) % N;

        for (int I = 0; I < N; I++)
            // If i is more counterclockwise than current q, then update q
            if (Orientation(Verteces[P], Verteces[I], Verteces[Q]) == 2)
                Q = I;

        // Now q is the most counterclockwise with respect to p. Set p as q for next iteration, so that q is added to result 'hull'
        P = Q;
    } while (P != Leftmost);

    return ConvexHull;
}

TArray<FVector> UCoverPathSplineComponent::SetMidpoints(TArray<FVector> CornerVerteces)
{
    TArray<FVector> UniqueVerteces;

    float MinZOnMesh = MAX_FLT;
    float HalfwayHeight = GetOwner()->GetTransform().GetScale3D().Z;

    // Prevents the issue of the mesh having to be symmetrical.
    // The important part is to grab the base, since covers will always have a base and not necessarily be dependent on the top.
    for (const FVector& CurrentVector : CornerVerteces)
        if (CurrentVector.Z < MinZOnMesh)
            MinZOnMesh = CurrentVector.Z;

    for (const FVector& CurrentVector : CornerVerteces)
    {
        if (FMath::IsNearlyEqual(CurrentVector.Z, MinZOnMesh, KINDA_SMALL_NUMBER))
        {
            bool bFoundDuplicate = false;
            for (const FVector& UniqueVector : UniqueVerteces)
                if (FMath::IsNearlyEqual(CurrentVector.X, UniqueVector.X, KINDA_SMALL_NUMBER) && FMath::IsNearlyEqual(CurrentVector.Y, UniqueVector.Y, KINDA_SMALL_NUMBER))
                {
                    bFoundDuplicate = true;
                    break;
                }

            if (!bFoundDuplicate)
            {
                FVector HalfwayVector = CurrentVector;
                HalfwayVector.Z = HalfwayHeight;
                UniqueVerteces.AddUnique(HalfwayVector);
            }
        }
    }

    /*
    if (GEngine && bToggleDebugCoverPathSpline)
        for (const FVector& UniqueVector : UniqueVerteces)
        {
            GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("Unique vector: %s"), *UniqueVector.ToCompactString()));
            DrawDebugPoint(GetWorld(), UniqueVector, 3.5f, FColor::Green, true);
        }
    */
    return JarvisMarchAlgorithm(UniqueVerteces);
}

TArray<FVector> UCoverPathSplineComponent::GetPointsCreateCurves(TArray<FVector> MidVerteces)
{
    TArray<FVector> SplineVerteces;
    if (MidVerteces.Num() < 2)
        return SplineVerteces;

    SplineVerteces.Add(MidVerteces[0]);

    for (const FVector& CurrentVector : MidVerteces)
    {
        int CurrentVectorIndex;
        MidVerteces.Find(CurrentVector, CurrentVectorIndex);

        const FVector& PrevVector = (CurrentVectorIndex - 1 > 0) ? (MidVerteces[CurrentVectorIndex - 1] - CurrentVector).GetSafeNormal() : CurrentVector;
        const FVector& NextVector = (CurrentVectorIndex + 1 < MidVerteces.Num()) ? (MidVerteces[CurrentVectorIndex + 1] - CurrentVector).GetSafeNormal() : MidVerteces.Last();

        double AngleBetweenTwoVerteces = FVector::DotProduct(PrevVector, NextVector);

        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-0, 20.f, FColor::Green, FString::Printf(TEXT("Angle between Verteces: %f, Previous vector: %s, Next vector: %s"), AngleBetweenTwoVerteces, *PrevVector.ToCompactString(), *NextVector.ToCompactString()));

        // Dot products will only ever range from -1 to 0 to 1
        // -1: Two vectors pointing in opposite directions AKA 180°
        // 0: Two vectors perpendicular to each other AKA 90°
        // 1: Two vectors pointing in the same direction AKA 0°
        if ((float)AngleBetweenTwoVerteces < CurveThresholdNormal)
            SplineVerteces.AddUnique(PrevVector + (CurrentVector - PrevVector) * 0.5f);
    }

    return SplineVerteces;
}
