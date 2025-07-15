// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SkeletalMeshComponent.h"

bool FPelvisFeetData::IsValid()
{
	const int32 NumUserDefinedFeet = IKFootPlacementFootParams.Num();

	return (NumUserDefinedFeet > 0) &&
		(PosedFootBoneWorldTransforms.Num() == NumUserDefinedFeet) &&
		(FootPlacementFlags.Num() == NumUserDefinedFeet) &&
		(PosedFootBoneComponentLocations.Num() == NumUserDefinedFeet) &&
		(FootRaycastHitResults.Num() == NumUserDefinedFeet) &&
		(TargetFootIKEffectorWorldLocations.Num() == NumUserDefinedFeet) &&
		(TargetFootWorldRotations.Num() == NumUserDefinedFeet) &&
		(TargetFootIKPoleWorldLocations.Num() == NumUserDefinedFeet) &&
		(InterpolatedFootIKEffectorWorldLocations.Num() == NumUserDefinedFeet) &&
		(InterpolatedFootWorldRotations.Num() == NumUserDefinedFeet) &&
		(InterpolatedFootIKPoleWorldLocations.Num() == NumUserDefinedFeet);
}

void UCharacterAnimationLibrary::InitializePelvis(AActor* OwningCharacterActor, FPelvisFeetData& FeetData)
{
	// Get number of feet attached to the pelvis
	const int32 NumFeet = FeetData.IKFootPlacementFootParams.Num();

	// Initialize foot data arrays for number of feet
	FeetData.PosedFootBoneWorldTransforms.SetNumZeroed(NumFeet);
	FeetData.FootPlacementFlags.SetNumZeroed(NumFeet);
	FeetData.PosedFootBoneComponentLocations.SetNumZeroed(NumFeet);

	FeetData.FootRaycastHitResults.SetNumZeroed(NumFeet);

	FeetData.TargetFootIKEffectorWorldLocations.SetNumZeroed(NumFeet);
	FeetData.TargetFootWorldRotations.SetNumZeroed(NumFeet);
	FeetData.TargetFootIKPoleWorldLocations.SetNumZeroed(NumFeet);

	FeetData.InterpolatedFootIKEffectorWorldLocations.SetNumZeroed(NumFeet);
	FeetData.InterpolatedFootWorldRotations.SetNumZeroed(NumFeet);
	FeetData.InterpolatedFootIKPoleWorldLocations.SetNumZeroed(NumFeet);

	// Add owning character as an ignored actor to the foot raycast collision query parameters for each foot
	for (int32 i = 0; i < NumFeet; ++i)
	{
		FeetData.IKFootPlacementFootParams[i].FootRaycastParams.FootRaycastCollisionQueryParams.AddIgnoredActor(OwningCharacterActor);
	}
}

void UCharacterAnimationLibrary::UpdatePelvis(const USkeletalMeshComponent* const CharacterSkeletalMeshComponent,
	const FVector& CharacterCapsuleCenterWorldLocation,
	const float CharacterCapsuleHalfHeight,
	FPelvisFeetData& FeetData)
{
#if WITH_EDITOR
	if (!FeetData.IsValid())
	{
		return;
	}
#endif // WITH_EDITOR

	// Get number of feet attached to the pelvis
	const int32 NumFeet = FeetData.IKFootPlacementFootParams.Num();

	// Gather foot placement system data

	// For each foot
	for (int32 i = 0; i < NumFeet; ++i)
	{
		// Get transform of foot ik bone in world space after the skeleton has been posed. Foot ik is performed as a post process step so need the original pose location of the 
		// foot bone here to probe for terrain collision geometry
		FeetData.PosedFootBoneWorldTransforms[i] = CharacterSkeletalMeshComponent->GetBoneTransform(FeetData.IKFootPlacementFootParams[i].PosedFootSourceBoneName,
			ERelativeTransformSpace::RTS_World);

		// Get location of foot ik bone in component space after the skeleton has been posed
		FeetData.PosedFootBoneComponentLocations[i] = CharacterSkeletalMeshComponent->GetBoneLocation(FeetData.IKFootPlacementFootParams[i].PosedFootSourceBoneName,
			EBoneSpaces::ComponentSpace);
	}
}

void UCharacterAnimationLibrary::ThreadSafeUpdatePelvis(UWorld* World,
	const FVector& CharacterCapsuleCenterWorldLocation,
	const float CharacterCapsuleHalfHeight,
	FPelvisFeetData& FeetData,
	const float DeltaSeconds,
	const float IKFootPlacementInterpSpeed,
	FVector& OutPelvisBoneAdditiveWorldTranslation)
{
#if WITH_EDITOR
	if (!FeetData.IsValid())
	{
		return;
	}
#endif // WITH_EDITOR

	// Get number of feet attached to the pelvis
	const int32 NumFeet = FeetData.IKFootPlacementFootParams.Num();

	// Calculate world space location of the bottom of the character's capsule
	const FVector CapsuleBottomWorldLocation = FVector(CharacterCapsuleCenterWorldLocation.X,
		CharacterCapsuleCenterWorldLocation.Y,
		CharacterCapsuleCenterWorldLocation.Z - StaticCast<double>(CharacterCapsuleHalfHeight));

	// Allocate foot placement update data
	FVector TargetPelvisBoneAdditiveWorldTranslation = FVector::ZeroVector;

	// Calculate feet
	for (int32 i = 0; i < NumFeet; ++i)
	{
		UCharacterAnimationLibrary::ComputeFoot(World, FeetData.PosedFootBoneWorldTransforms[i].GetLocation(), FeetData.PosedFootBoneWorldTransforms[i].GetRotation(),
			FeetData.PosedFootBoneComponentLocations[i], FeetData.IKFootPlacementFootParams[i], FeetData.FootPlacementFlags[i], FeetData.FootRaycastHitResults[i],
			FeetData.TargetFootIKEffectorWorldLocations[i], FeetData.TargetFootWorldRotations[i], FeetData.TargetFootIKPoleWorldLocations[i]);
	}

	// Calculate pelvis
	UCharacterAnimationLibrary::ComputePelvis(FeetData.FootRaycastHitResults.GetData(), NumFeet, CapsuleBottomWorldLocation, TargetPelvisBoneAdditiveWorldTranslation,
		FeetData.TargetFootIKEffectorWorldLocations.GetData());

	// Interpolate foot placement values
	UCharacterAnimationLibrary::InterpolateFootPlacementValues(FeetData.TargetFootIKEffectorWorldLocations.GetData(), FeetData.TargetFootWorldRotations.GetData(),
		FeetData.TargetFootIKPoleWorldLocations.GetData(), TargetPelvisBoneAdditiveWorldTranslation, NumFeet, DeltaSeconds, IKFootPlacementInterpSpeed,
		FeetData.InterpolatedFootIKEffectorWorldLocations.GetData(), FeetData.InterpolatedFootWorldRotations.GetData(), FeetData.InterpolatedFootIKPoleWorldLocations.GetData(),
		OutPelvisBoneAdditiveWorldTranslation);
}

void UCharacterAnimationLibrary::RaycastFootForPlacement(FHitResult& OutHit,
	const TObjectPtr<UWorld> World,
	const FVector& FootBonePoseWorldLocation,
	const FIKFootPlacementParameters& FootPlacementParams)
{
	FVector WorldRaycastStart = FootBonePoseWorldLocation;
	WorldRaycastStart.Z += StaticCast<double>(FootPlacementParams.FootRaycastParams.FootRaycastHeightOffset);

	FVector WorldRaycastEnd = WorldRaycastStart;
	WorldRaycastEnd.Z -= FootPlacementParams.FootRaycastParams.FootRaycastDistance;

	World->LineTraceSingleByChannel(
		OutHit,
		WorldRaycastStart,
		WorldRaycastEnd,
		FootPlacementParams.FootRaycastParams.FootRaycastCollisionChannel,
		FootPlacementParams.FootRaycastParams.FootRaycastCollisionQueryParams);
}

FVector UCharacterAnimationLibrary::CalculateFootPlacementLocation(const FHitResult& FootPlacementRaycastResult, const float FootBoneHeight)
{
	FVector Temp = FootPlacementRaycastResult.Location;
	Temp.Z += StaticCast<double>(FootBoneHeight);
	return Temp;
}

FRotator UCharacterAnimationLibrary::CalculateFootPlacementAdditiveRotation(const FHitResult& FootPlacementRaycastResult,
	const FValueConstraint& AdditivePitchConstraint,
	const FValueConstraint& AdditiveRollConstraint)
{
	const FVector& Normal = FootPlacementRaycastResult.Normal;
	FRotator Temp = FRotator::MakeFromEuler(FVector(UKismetMathLibrary::DegAtan2(Normal.Y, Normal.Z), UKismetMathLibrary::DegAtan2(Normal.X, Normal.Z) * -1.0, 0.0));
	Temp.Pitch = AdditivePitchConstraint.ConstrainValue(Temp.Pitch);
	Temp.Roll = AdditiveRollConstraint.ConstrainValue(Temp.Roll);
	return Temp;
}

double UCharacterAnimationLibrary::CalculateAdditivePelvisBoneVerticalTranslationForFootPlacement(const FHitResult* const FootRaycastHitResultContiguousStorageStart,
	const int32 NumFeet,
	const double CapsuleBottomWorldSpaceVerticalLocation)
{
	// If no foot raycast found collision geometry, do not offset the pelvis by any amount
	bool FootFoundCollision = false;
	for (int32 i = 0; i < NumFeet; ++i)
	{
		if ((FootRaycastHitResultContiguousStorageStart + i)->bBlockingHit)
		{
			FootFoundCollision = true;
			break;
		}
	}

	if (!FootFoundCollision)
	{
		return 0.0;
	}

	// Heap allocation
	TArray<double>  FootVerticalOffsets = {};
	FootVerticalOffsets.Reserve(NumFeet);

	for (int32 i = 0; i < NumFeet; ++i)
	{
		const FHitResult& HitResult = *(FootRaycastHitResultContiguousStorageStart + i);

		FootVerticalOffsets.Emplace((HitResult.bBlockingHit) ?
			(HitResult.Location.Z - CapsuleBottomWorldSpaceVerticalLocation) : UE_DOUBLE_BIG_NUMBER);
	}

	return FMath::Min(FootVerticalOffsets);
}

void UCharacterAnimationLibrary::ComputeFoot(UWorld* const World,
	const FVector& FootBonePoseWorldSpaceLocation,
	const FQuat& FootBonePoseWorldSpaceRotation,
	const FVector& FootBonePoseComponentSpaceLocation,
	const FIKFootPlacementParameters& FootPlacementParameters,
	const bool PlaceFootFlag,
	FHitResult& OutFootRaycastHitResult,
	FVector& OutTargetFootIkEffectorWorldSpaceLocation,
	FRotator& OutTargetFootWorldSpaceRotation,
	FVector& OutTargetFootIkPoleWorldSpaceLocation)
{
	UCharacterAnimationLibrary::RaycastFootForPlacement(OutFootRaycastHitResult,
		World,
		FootBonePoseWorldSpaceLocation,
		FootPlacementParameters);

	OutTargetFootIkEffectorWorldSpaceLocation = (OutFootRaycastHitResult.bBlockingHit) ?
		UCharacterAnimationLibrary::CalculateFootPlacementLocation(OutFootRaycastHitResult, FootPlacementParameters.FootBoneHeight) :
		FootBonePoseWorldSpaceLocation;

	if (OutFootRaycastHitResult.bBlockingHit)
	{
		if (PlaceFootFlag)
		{
			OutTargetFootIkEffectorWorldSpaceLocation = UCharacterAnimationLibrary::CalculateFootPlacementLocation(OutFootRaycastHitResult, FootPlacementParameters.FootBoneHeight);

			OutTargetFootWorldSpaceRotation = UKismetMathLibrary::ComposeRotators(FootBonePoseWorldSpaceRotation.Rotator(),
				UCharacterAnimationLibrary::CalculateFootPlacementAdditiveRotation(OutFootRaycastHitResult, FootPlacementParameters.FootAdditivePitchValueConstraint,
					FootPlacementParameters.FootAdditiveRoleValueConstraint)
			);
		}
		else
		{
			// If placement for the foot is not active, need to add component space height of the posed bone to the calculated foot placement location's world up component (Z axis)
			// without a foot bone height offset
			OutTargetFootIkEffectorWorldSpaceLocation = UCharacterAnimationLibrary::CalculateFootPlacementLocation(OutFootRaycastHitResult, 0.0f);
			OutTargetFootIkEffectorWorldSpaceLocation.Z += FootBonePoseComponentSpaceLocation.Z;

			OutTargetFootWorldSpaceRotation = FootBonePoseWorldSpaceRotation.Rotator();
		}
	}
	else
	{
		OutTargetFootIkEffectorWorldSpaceLocation = FootBonePoseWorldSpaceLocation;
		OutTargetFootWorldSpaceRotation = FootBonePoseWorldSpaceRotation.Rotator();
	}

	OutTargetFootIkPoleWorldSpaceLocation = (OutTargetFootIkEffectorWorldSpaceLocation +
		FVector(0.0, 0.0, StaticCast<double>(FootPlacementParameters.LegIkPoleTargetVerticalOffset))) -
		(FRotator(0.0, 90.0, 0.0).RotateVector(FootBonePoseWorldSpaceRotation.GetUpVector()) * FootPlacementParameters.LegIkPoleTargetOffset);
}

void UCharacterAnimationLibrary::ComputePelvis(const FHitResult* const FootRaycastHitResultContiguousStorageStart,
	const int32 NumFeet,
	const FVector& CharacterCapsuleBottomWorldSpaceLocation,
	FVector& OutTargetPelvisBoneAdditiveWorldSpaceTranslation,
	FVector* const OutTargetFootIkEffectorWorldSpaceLocationContiguousStorageStart)
{
	if (NumFeet == 0)
	{
		return;
	}

	OutTargetPelvisBoneAdditiveWorldSpaceTranslation.Z = UCharacterAnimationLibrary::CalculateAdditivePelvisBoneVerticalTranslationForFootPlacement(
		FootRaycastHitResultContiguousStorageStart, NumFeet, CharacterCapsuleBottomWorldSpaceLocation.Z);

	// We have moved the pelvis by above amount. If there is no collision found from a foot raycast, the foot should be moved by the same amount
	for (int32 i = 0; i < NumFeet; ++i)
	{
		if (!((FootRaycastHitResultContiguousStorageStart + i)->bBlockingHit))
		{
			(OutTargetFootIkEffectorWorldSpaceLocationContiguousStorageStart + i)->Z += OutTargetPelvisBoneAdditiveWorldSpaceTranslation.Z;
		}
	}
}

void UCharacterAnimationLibrary::InterpolateFootPlacementValues(const FVector* const TargetFootIKEffectorWorldSpaceLocationsContiguousStorageStart,
	const FRotator* const TargetFootWorldSpaceRotationsContiguousStorageStart,
	const FVector* const TargetFootIKPoleLocationsContiguousStorageStart,
	const FVector& TargetPelvisBoneAdditiveWorldSpaceTranslation,
	const int32 NumFeet,
	const float DeltaSeconds,
	const float InterpolationSpeed,
	FVector* const OutInterpolatedFootIKEffectorWorldSpaceLocationsContiguousStorageStart,
	FRotator* const OutInterpolatedFootWorldSpaceRotationsContiguousStorageStart,
	FVector* const OutInterpolatedFootIKPoleLocationsContiguousStorageStart,
	FVector& OutInterpolatedPelvisBoneAdditiveWorldSpaceTranslation)
{
	for (int32 i = 0; i < NumFeet; ++i)
	{
		// Foot ik effector location
		*(OutInterpolatedFootIKEffectorWorldSpaceLocationsContiguousStorageStart + i) =
			FMath::VInterpTo(*(OutInterpolatedFootIKEffectorWorldSpaceLocationsContiguousStorageStart + i),
				*(TargetFootIKEffectorWorldSpaceLocationsContiguousStorageStart + i),
				DeltaSeconds,
				InterpolationSpeed);

		// Foot rotation
		*(OutInterpolatedFootWorldSpaceRotationsContiguousStorageStart + i) =
			FMath::RInterpTo(*(OutInterpolatedFootWorldSpaceRotationsContiguousStorageStart + i),
				*(TargetFootWorldSpaceRotationsContiguousStorageStart + i),
				DeltaSeconds,
				InterpolationSpeed);

		// Foot ik pole target location
		*(OutInterpolatedFootIKPoleLocationsContiguousStorageStart + i) =
			FMath::VInterpTo(*(OutInterpolatedFootIKPoleLocationsContiguousStorageStart + i),
				*(TargetFootIKPoleLocationsContiguousStorageStart + i),
				DeltaSeconds,
				InterpolationSpeed);

		// Pelvis additive translation
		OutInterpolatedPelvisBoneAdditiveWorldSpaceTranslation =
			FMath::VInterpTo(OutInterpolatedPelvisBoneAdditiveWorldSpaceTranslation,
				TargetPelvisBoneAdditiveWorldSpaceTranslation,
				DeltaSeconds,
				InterpolationSpeed);
	}
}
