// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CharacterAnimationLibrary.generated.h"

USTRUCT(BlueprintType)
struct FValueConstraint
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere)
	float Max = 360.0f;

	UPROPERTY(EditAnywhere)
	float Min = -360.0f;

	template<typename T>
	constexpr T ConstrainValue(const T Value) const { return FMath::Clamp(Value, StaticCast<T>(Min), StaticCast<T>(Max)); }
};

USTRUCT(BlueprintType)
struct FFootRaycastParameters
{
	GENERATED_BODY()

	// The vertical distance in Unreal units that are added to the posed foot bone's vertical location used as the start location of the foot probe used to search for world
	// collision geometry
	UPROPERTY(EditDefaultsOnly)
	float FootRaycastHeightOffset = 0.0f;

	// The distance probed vertically down from the foot probe start location to search for world collision geometry
	UPROPERTY(EditDefaultsOnly)
	float FootRaycastDistance = 150.0f;

	// Collision channel tested for world collision geometry
	// TODO: Consider replacing this with a foot placement speciifc collision channel? Will require all terrain to be marked/unmarked
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> FootRaycastCollisionChannel = ECC_Visibility;

	// Not exposed to blueprint, setup from code. Can be used to add ignored actors to the raycast
	FCollisionQueryParams FootRaycastCollisionQueryParams = {};
};

USTRUCT(BlueprintType)
struct FIKFootPlacementParameters
{
	GENERATED_BODY()

	// The name of the foot bone in the character's skeleton that will be used to gather pose transform data for the foot. The bone name supplied here should not be that of a bone that
	// is manipulated by the IK foot placement system
	UPROPERTY(EditDefaultsOnly)
	FName PosedFootSourceBoneName = NAME_None;

	// The distance from the ground of each foot bone in rest pose
	UPROPERTY(EditDefaultsOnly)
	float FootBoneHeight = 0.0f;

	// The distance the leg IK joint pole target is offset in front of the character's leg
	UPROPERTY(EditDefaultsOnly)
	float LegIkPoleTargetOffset = 200.0f;

	// The distance the leg IK joint pole target is offset in the world up axis
	UPROPERTY(EditDefaultsOnly)
	float LegIkPoleTargetVerticalOffset = -100.0f;

	// Foot raycast parameters
	UPROPERTY(EditDefaultsOnly)
	FFootRaycastParameters FootRaycastParams = {};

	// The max/min amount of pitch that can be added to the posed foot bone rotation when aligning the foot with a collision surface
	UPROPERTY(EditDefaultsOnly)
	FValueConstraint FootAdditivePitchValueConstraint = {};

	// The max/min amount of roll that can be added to the posed foot bone rotation when aligning the foot with a collision surface
	UPROPERTY(EditDefaultsOnly)
	FValueConstraint FootAdditiveRoleValueConstraint = {};
};

// This struct contains the data for all of the feet that are attached to a pelvis. Each pelvis the character has will need one of these structures
USTRUCT(BlueprintType)
struct FPelvisFeetData
{
	GENERATED_BODY()

	// Filled out in blueprint details panel
	UPROPERTY(EditAnywhere)
	TArray<FIKFootPlacementParameters> IKFootPlacementFootParams = {};

	// Used internally by foot placement system
	TArray<FTransform> PosedFootBoneWorldTransforms = {};
	TArray<bool> FootPlacementFlags = {};
	TArray<FVector> PosedFootBoneComponentLocations = {};

	TArray<FHitResult> FootRaycastHitResults = {};

	TArray<FVector> TargetFootIKEffectorWorldLocations = {};
	TArray<FRotator> TargetFootWorldRotations = {};
	TArray<FVector> TargetFootIKPoleWorldLocations = {};

	TArray<FVector> InterpolatedFootIKEffectorWorldLocations = {};
	TArray<FRotator> InterpolatedFootWorldRotations = {};
	TArray<FVector> InterpolatedFootIKPoleWorldLocations = {};

	bool IsValid();
};

/**
 *
 */
UCLASS()
class UCharacterAnimationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Call during animation intialize event in a character's anim instance for each pelvis the character has with the relevant FPelvisFeetData structure for the pelvis.
	static void InitializePelvis(AActor* OwningCharacterActor, FPelvisFeetData& FeetData);

	// Call during animation update event in a character's anim instance for each pelvis the character has with the relevant FPelvisFeetData structure for the pelvis.
	static void UpdatePelvis(const USkeletalMeshComponent* const CharacterSkeletalMeshComponent, const FVector& CharacterCapsuleCenterWorldLocation,
		const float CharacterCapsuleHalfHeight, FPelvisFeetData& FeetData);

	// Call during animation thread safe update event in a character's anim instance for each pelvis the character has with the relevant FPelvisFeetData structure for the pelvis
	static void ThreadSafeUpdatePelvis(UWorld* World, const FVector& CharacterCapsuleCenterWorldLocation, const float CharacterCapsuleHalfHeight, FPelvisFeetData& FeetData,
		const float DeltaSeconds, const float IKFootPlacementInterpSpeed, FVector& OutPelvisBoneAdditiveWorldTranslation);

private:
	// Performs a raycast for a foot. Returns through the input parameter the hit result of the raycast. Used as part of a character's foot IK placement system
	static void RaycastFootForPlacement(FHitResult& OutHit,
		const TObjectPtr<UWorld> World,
		const FVector& FootBonePoseWorldLocation,
		const FIKFootPlacementParameters& FootPlacementParams);

	// Returns the foot bone location to place the foot on top of the hit geometry
	static FVector CalculateFootPlacementLocation(const FHitResult& FootPlacementRaycastResult, const float FootBoneHeight);

	// Returns the additive rotation to align the foot with the hit geometry
	static FRotator CalculateFootPlacementAdditiveRotation(const FHitResult& FootPlacementRaycastResult,
		const FValueConstraint& AdditivePitchConstraint,
		const FValueConstraint& AdditiveRollConstraint);

	// Returns the additive translation to add to the pelvis bone in the vertical up axis to correct pelvis location when placing feet on world collision geometry
	static double CalculateAdditivePelvisBoneVerticalTranslationForFootPlacement(const FHitResult* const FootRaycastHitResultContiguousStorageStart,
		const int32 NumFeet,
		const double CapsuleBottomWorldSpaceVerticalLocation);

	static void ComputeFoot(UWorld* const World,
		const FVector& FootBonePoseWorldSpaceLocation,
		const FQuat& FootBonePoseWorldSpaceRotation,
		const FVector& FootBonePoseComponentSpaceLocation,
		const FIKFootPlacementParameters& FootPlacementParameters,
		const bool PlaceFootFlag,
		FHitResult& OutFootRaycastHitResult,
		FVector& OutTargetFootIkEffectorWorldSpaceLocation,
		FRotator& OutTargetFootWorldSpaceRotation,
		FVector& OutTargetFootIkPoleWorldSpaceLocation);

	static void ComputePelvis(const FHitResult* const FootRaycastHitResultContiguousStorageStart,
		const int32 NumFeet,
		const FVector& CharacterCapsuleBottomWorldSpaceLocation,
		FVector& OutTargetPelvisBoneAdditiveWorldSpaceTranslation,
		FVector* const OutTargetFootIkEffectorWorldSpaceLocationContiguousStorageStart);

	static void InterpolateFootPlacementValues(const FVector* const TargetFootIKEffectorWorldSpaceLocationsContiguousStorageStart,
		const FRotator* const TargetFootWorldSpaceRotationsContiguousStorageStart,
		const FVector* const TargetFootIKPoleLocationsContiguousStorageStart,
		const FVector& TargetPelvisBoneAdditiveWorldSpaceTranslation,
		const int32 NumFeet,
		const float DeltaSeconds,
		const float InterpolationSpeed,
		FVector* const OutInterpolatedFootIKEffectorWorldSpaceLocationsContiguousStorageStart,
		FRotator* const OutInterpolatedFootWorldSpaceRotationsContiguousStorageStart,
		FVector* const OutInterpolatedFootIKPoleLocationsContiguousStorageStart,
		FVector& OutInterpolatedPelvisBoneAdditiveWorldSpaceTranslation);
};
