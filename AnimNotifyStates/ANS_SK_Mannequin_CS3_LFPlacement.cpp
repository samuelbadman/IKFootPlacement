// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_SK_Mannequin_CS3_LFPlacement.h"
#include "Animation/AnimInstances/SK_Mannequin_CS3_AnimInstance.h"

void UANS_SK_Mannequin_CS3_LFPlacement::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (USK_Mannequin_CS3_AnimInstance* SK_Mannequin_CS3_AnimInstance = Cast<USK_Mannequin_CS3_AnimInstance>(MeshComp->GetAnimInstance()))
	{
		SK_Mannequin_CS3_AnimInstance->ANS_LFPlacement_Tick();
	}
}

void UANS_SK_Mannequin_CS3_LFPlacement::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (USK_Mannequin_CS3_AnimInstance* SK_Mannequin_CS3_AnimInstance = Cast<USK_Mannequin_CS3_AnimInstance>(MeshComp->GetAnimInstance()))
	{
		SK_Mannequin_CS3_AnimInstance->ANS_LFPlacement_End();
	}
}
