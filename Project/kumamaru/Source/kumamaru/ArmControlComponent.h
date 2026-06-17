// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArmControlComponent.generated.h"

class USkeletalMeshComponent;
class UArmAnimInstance;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KUMAMARU_API UArmControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UArmControlComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Name of the upper arm bone (shoulder). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Bones")
	FName UpperArmBoneName = TEXT("upperarm_l");

	/** Name of the lower arm bone (elbow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Bones")
	FName LowerArmBoneName = TEXT("lowerarm_l");

	/** Name of the hand bone (end of the FK chain; follows passively). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Bones")
	FName HandBoneName = TEXT("hand_l");

	/** Length of the upper arm bone (L1), in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Bones", meta = (ClampMin = "0.1"))
	float UpperArmLength = 30.0f;

	/** Length of the lower arm bone (L2), in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Bones", meta = (ClampMin = "0.1"))
	float LowerArmLength = 30.0f;

	/** Speed at which the hand target moves across the XY plane, in centimeters per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Input", meta = (ClampMin = "0.0"))
	float HandMoveSpeed = 50.0f;

	/** Interp speed for smoothing theta_upper / theta_lower toward their solved targets (FInterpTo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Smoothing", meta = (ClampMin = "0.0"))
	float SmoothSpeed = 10.0f;

	/** Lerp factor used to smooth the elbow position toward its solved target each tick (0 = frozen, 1 = no smoothing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Smoothing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothFactor = 0.15f;

	/** Fraction of (L1 + L2) that D is clamped to before the law-of-cosines solve, keeping acos arguments away from the +/-1 singularity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arm IK|Reach", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ReachRatio = 0.95f;

protected:
	/** Skeletal mesh fetched from the owning actor. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	/** Fixed shoulder (upperarm) world location, captured once on BeginPlay. */
	FVector ShoulderWorldLocation = FVector::ZeroVector;

	/** Current world-space target location for the hand, confined to the shoulder's XY plane. */
	FVector HandTargetLocation = FVector::ZeroVector;

	/** Solved world-space rotation for the upper arm bone (yaw-only, within the XY plane). */
	FRotator UpperArmRotation = FRotator::ZeroRotator;

	/** Solved world-space rotation for the lower arm bone (yaw-only, within the XY plane). */
	FRotator LowerArmRotation = FRotator::ZeroRotator;

	/** Smoothed theta_upper from the previous tick (radians), used as the FInterpTo base for angle smoothing. */
	float PrevThetaUpper = 0.0f;

	/** Smoothed theta_lower from the previous tick (radians), used as the FInterpTo base for angle smoothing. */
	float PrevThetaLower = 0.0f;

	/** Smoothed elbow world XY position from the previous tick, used as the Lerp base for elbow-position smoothing. */
	FVector2D PrevElbowPos = FVector2D::ZeroVector;

private:
	/** Reads arrow key / WASD state and moves HandTargetLocation across the shoulder's XY plane. */
	void HandleDirectionalInput(float DeltaTime);

	/** Keeps the hand target on the shoulder's Z plane and clamps its distance to L1 + L2. */
	void ClampHandTargetToReach();

	/**
	 * Solves the un-smoothed law-of-cosines target elbow position from the current HandTargetLocation,
	 * clamping D to (L1 + L2) * ReachRatio to keep the acos arguments away from +/-1. Also outputs
	 * the corresponding theta_upper.
	 */
	FVector2D ComputeTargetElbowPosition(float& OutThetaUpper) const;

	/** Solves the analytic 2D two-bone IK (law of cosines), smooths the result, and fills UpperArmRotation / LowerArmRotation. */
	void SolveArmRotations(float DeltaTime);

	/** Pushes UpperArmRotation / LowerArmRotation into the skeletal mesh's AnimInstance. */
	void PushRotationsToAnimInstance() const;	
};
