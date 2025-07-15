// Fill out your copyright notice in the Description page of Project Settings.


#include "SK_Mannequin_CS3_AnimInstance.h"
#include "Pawns/Characters/SK_Mannequin_CS3_Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

USK_Mannequin_CS3_AnimInstance::USK_Mannequin_CS3_AnimInstance()
	:
	IKFootPlacementInterpSpeed(22.5f),
	IKFootPlacementPelvisFeetData({}),
	bShouldIdle(true),
	bShouldWalk(false),
	bShouldRun(false),
	FootIkEffectorLocation_L(FVector::ZeroVector),
	FootIkPoleLocation_L(FVector::ZeroVector),
	FootIkWorldRotation_L(FRotator::ZeroRotator),
	FootIkEffectorLocation_R(FVector::ZeroVector),
	FootIkPoleLocation_R(FVector::ZeroVector),
	FootIkWorldRotation_R(FRotator::ZeroRotator),
	PelvisBoneAdditiveWorldTranslation(FVector::ZeroVector),
	IkAlpha(1.0f),
	World(nullptr),
	Character(nullptr),
	MovementComponent(nullptr),
	MeshComponent(nullptr),
	CapsuleComponent(nullptr),
	CharacterMovementState(ECharacterMovementState::Run),
	CurrentCharacterAcceleration(FVector::ZeroVector),
	CharacterCapsuleCenterWorldLocation(FVector::ZeroVector),
	CharacterCapsuleHalfHeight(0.0f)
{
}

void USK_Mannequin_CS3_AnimInstance::ANS_LFPlacement_Tick()
{
	// Tick used here instead of begin event as begin event is not always called when animations are blending

#if WITH_EDITOR
	if (!IKFootPlacementPelvisFeetData.FootPlacementFlags.IsValidIndex(FootIndex_L))
	{
		return;
	}
#endif // WITH_EDITOR

	IKFootPlacementPelvisFeetData.FootPlacementFlags[FootIndex_L] = true;
}

void USK_Mannequin_CS3_AnimInstance::ANS_LFPlacement_End()
{
#if WITH_EDITOR
	if (!IKFootPlacementPelvisFeetData.FootPlacementFlags.IsValidIndex(FootIndex_L))
	{
		return;
	}
#endif // WITH_EDITOR

	IKFootPlacementPelvisFeetData.FootPlacementFlags[FootIndex_L] = false;
}

void USK_Mannequin_CS3_AnimInstance::ANS_RFPlacement_Tick()
{
	// Tick used here instead of begin event as begin event is not always called when animations are blending

#if WITH_EDITOR
	if (!IKFootPlacementPelvisFeetData.FootPlacementFlags.IsValidIndex(FootIndex_R))
	{
		return;
	}
#endif // WITH_EDITOR

	IKFootPlacementPelvisFeetData.FootPlacementFlags[FootIndex_R] = true;
}

void USK_Mannequin_CS3_AnimInstance::ANS_RFPlacement_End()
{
#if WITH_EDITOR
	if (!IKFootPlacementPelvisFeetData.FootPlacementFlags.IsValidIndex(FootIndex_R))
	{
		return;
	}
#endif // WITH_EDITOR

	IKFootPlacementPelvisFeetData.FootPlacementFlags[FootIndex_R] = false;
}

void USK_Mannequin_CS3_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Get world
	World = GetWorld();

	// Get owning character
	Character = Cast<ASK_Mannequin_CS3_Character>(TryGetPawnOwner());

	// Character can be null when using animation editors
#if WITH_EDITOR
	if (!IsValid(Character))
	{
		return;
	}
#endif

	// Get owning character component references
	MovementComponent = Character->GetCharacterMovement();
	MeshComponent = Character->GetMesh();
	CapsuleComponent = Character->GetCapsuleComponent();

	// Initialize foot ik placement system
	UCharacterAnimationLibrary::InitializePelvis(Character, IKFootPlacementPelvisFeetData);
}

void USK_Mannequin_CS3_AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// If character and component references are valid. This can be null when viewing animations in the editor
#if WITH_EDITOR
	if (!IsValid(Character) ||
		!IsValid(MovementComponent) ||
		!IsValid(MeshComponent) ||
		!IsValid(CapsuleComponent))
	{
		// Disable character IK
		IkAlpha = 0.0f;

		return;
	}
#endif

	// Get character movement state
	CharacterMovementState = Character->GetMovementState();

	// Get current character acceleration
	CurrentCharacterAcceleration = Character->GetCharacterMovement()->GetCurrentAcceleration();

	// Get character capsule center world space location
	CharacterCapsuleCenterWorldLocation = CapsuleComponent->GetComponentLocation();

	// Get character capsule scaled half height
	CharacterCapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();

	// Update foot ik placement system
	UCharacterAnimationLibrary::UpdatePelvis(MeshComponent, CharacterCapsuleCenterWorldLocation, CharacterCapsuleHalfHeight, IKFootPlacementPelvisFeetData);
}

void USK_Mannequin_CS3_AnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// Update ground locomotion flags
	bShouldIdle = !(CurrentCharacterAcceleration.SquaredLength() > 0.0);

	if (bShouldIdle)
	{
		bShouldWalk = bShouldRun = false;
	}
	else
	{
		switch (CharacterMovementState)
		{
		case ECharacterMovementState::Walk:
			bShouldWalk = true;
			bShouldRun = false;
			break;

		case ECharacterMovementState::Run:
			bShouldWalk = false;
			bShouldRun = true;
			break;
		}
	}

	// Update foot ik placement system
	UCharacterAnimationLibrary::ThreadSafeUpdatePelvis(World, CharacterCapsuleCenterWorldLocation, CharacterCapsuleHalfHeight, IKFootPlacementPelvisFeetData, DeltaSeconds,
		IKFootPlacementInterpSpeed, PelvisBoneAdditiveWorldTranslation);

	// Copy interpolated data to exposed single variables as array lookup is not supported by animation fast path
	CopyFootPlacementDataToOutput();
}

void USK_Mannequin_CS3_AnimInstance::CopyFootPlacementDataToOutput()
{
#if WITH_EDITOR
	// Check if there is valid foot data for each foot when in the editor. 
	// Foot data should always be present in build so the if check is not performed in shipped code
	if (!IKFootPlacementPelvisFeetData.IsValid())
	{
		return;
	}
#endif // WITH_EDITOR

	FootIkEffectorLocation_L = IKFootPlacementPelvisFeetData.InterpolatedFootIKEffectorWorldLocations[FootIndex_L];
	FootIkWorldRotation_L = IKFootPlacementPelvisFeetData.InterpolatedFootWorldRotations[FootIndex_L];
	FootIkPoleLocation_L = IKFootPlacementPelvisFeetData.InterpolatedFootIKPoleWorldLocations[FootIndex_L];

	FootIkEffectorLocation_R = IKFootPlacementPelvisFeetData.InterpolatedFootIKEffectorWorldLocations[FootIndex_R];
	FootIkWorldRotation_R = IKFootPlacementPelvisFeetData.InterpolatedFootWorldRotations[FootIndex_R];
	FootIkPoleLocation_R = IKFootPlacementPelvisFeetData.InterpolatedFootIKPoleWorldLocations[FootIndex_R];
}
