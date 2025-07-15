// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Project1AnimInstanceBase.h"
#include "FunctionLibraries/CharacterAnimationLibrary.h"
#include "SK_Mannequin_CS3_AnimInstance.generated.h"

class ASK_Mannequin_CS3_Character;
class UCharacterMovementComponent;
class UCapsuleComponent;

enum class ECharacterMovementState : uint8;

/**
 *
 */
UCLASS()
class USK_Mannequin_CS3_AnimInstance : public UProject1AnimInstanceBase
{
	GENERATED_BODY()

private:
	// Foot placement system constants
	static constexpr int32 FootIndex_L = 0;
	static constexpr int32 FootIndex_R = 1;

	// Foot placement system properties
	UPROPERTY(EditAnywhere, Category = "IK Foot Placement")
	float IKFootPlacementInterpSpeed;

	UPROPERTY(EditAnywhere, Category = "IK Foot Placement")
	FPelvisFeetData IKFootPlacementPelvisFeetData;

	// Computed animation data exposed to blueprint animation system
	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	bool bShouldIdle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	bool bShouldWalk;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	bool bShouldRun;

	// Computed foot placement system data exposed to blueprint animation system
	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	FVector FootIkEffectorLocation_L;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	FVector FootIkPoleLocation_L;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	FRotator FootIkWorldRotation_L;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	FVector FootIkEffectorLocation_R;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	FVector FootIkPoleLocation_R;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	FRotator FootIkWorldRotation_R;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	FVector PelvisBoneAdditiveWorldTranslation;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	float IkAlpha;

	// World reference
	TObjectPtr<UWorld> World;

	// Pointer to the owning character of this anim instance
	TObjectPtr<ASK_Mannequin_CS3_Character> Character;

	// Owning character's movement component reference
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	// Owning character's skeletal mesh component reference
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	// Owning character's capsule component reference
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	// Data gathered each animation update
	ECharacterMovementState CharacterMovementState;
	FVector CurrentCharacterAcceleration;
	FVector CharacterCapsuleCenterWorldLocation;
	float CharacterCapsuleHalfHeight;

public:
	USK_Mannequin_CS3_AnimInstance();

	void ANS_LFPlacement_Tick();
	void ANS_LFPlacement_End();

	void ANS_RFPlacement_Tick();
	void ANS_RFPlacement_End();

private:
	void NativeInitializeAnimation() override;
	void NativeUpdateAnimation(float DeltaSeconds) override;
	void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	void CopyFootPlacementDataToOutput();
};
