// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MPAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class KUMAMARU_API UMPAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** World-space rotation to apply to the upper arm bone (Transform Modify Bone, Replace Existing, World Space). */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Arm IK")
	FRotator UpperArmRotation = FRotator::ZeroRotator;

	/** World-space rotation to apply to the lower arm bone (Transform Modify Bone, Replace Existing, World Space). */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Arm IK")
	FRotator LowerArmRotation = FRotator::ZeroRotator;
	
};
