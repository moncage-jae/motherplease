// Fill out your copyright notice in the Description page of Project Settings.


#include "ArmControlComponent.h"
#include "MPAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
UArmControlComponent::UArmControlComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Default to a fully extended arm along the shoulder's -Y axis (offset magnitude == L1 + L2).
	InitialHandOffset = FVector2D(0.0f, -(UpperArmLength + LowerArmLength));

	// ...
}


// Called when the game starts
void UArmControlComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if (AActor* Owner = GetOwner())
	{
		SkeletalMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (SkeletalMesh)
	{
		// Shoulder position is fixed for the lifetime of the component: captured once here.
		ShoulderWorldLocation = SkeletalMesh->GetBoneLocation(UpperArmBoneName, EBoneSpaces::WorldSpace);

		// --- Previous behavior (restore by uncommenting if InitialHandOffset approach is reverted) ---
		// Start the hand target where the hand bone currently is so the arm doesn't snap on play.
		// HandTargetLocation = SkeletalMesh->GetBoneLocation(HandBoneName, EBoneSpaces::WorldSpace);
		// HandTargetLocation.Z = ShoulderWorldLocation.Z;

		// Seed the hand target from the designer-specified offset instead of the hand bone's pose,
		// so the starting hand position is explicit and can later be restored from a save game.
		HandTargetLocation = ShoulderWorldLocation + FVector(InitialHandOffset.X, InitialHandOffset.Y, 0.0f);

		// Seed the smoothing state with the un-smoothed solve for the starting pose so the
		// first tick doesn't interpolate from zero and snap the arm into place.
		PrevElbowPos = ComputeTargetElbowPosition(PrevThetaUpper);
		PrevThetaLower = FMath::Atan2(HandTargetLocation.Y - PrevElbowPos.Y, HandTargetLocation.X - PrevElbowPos.X);
	}
}


// Called every frame
void UArmControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (!SkeletalMesh)
	{
		return;
	}

	HandleDirectionalInput(DeltaTime);
	ClampHandTargetToReach();
	SolveArmRotations(DeltaTime);
	PushRotationsToAnimInstance();
}

void UArmControlComponent::HandleDirectionalInput(float DeltaTime)
{
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwningPawn ? Cast<APlayerController>(OwningPawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	FVector2D MoveInput = FVector2D::ZeroVector;

	// Arrow keys
	if (PlayerController->IsInputKeyDown(EKeys::Up))    { MoveInput.Y -= 1.0f; }
	if (PlayerController->IsInputKeyDown(EKeys::Down))  { MoveInput.Y += 1.0f; }
	if (PlayerController->IsInputKeyDown(EKeys::Right)) { MoveInput.X += 1.0f; }
	if (PlayerController->IsInputKeyDown(EKeys::Left))  { MoveInput.X -= 1.0f; }

	// WASD
	if (PlayerController->IsInputKeyDown(EKeys::W)) { MoveInput.Y += 1.0f; }
	if (PlayerController->IsInputKeyDown(EKeys::S)) { MoveInput.Y -= 1.0f; }
	if (PlayerController->IsInputKeyDown(EKeys::D)) { MoveInput.X += 1.0f; }
	if (PlayerController->IsInputKeyDown(EKeys::A)) { MoveInput.X -= 1.0f; }

	if (!MoveInput.IsNearlyZero())
	{
		MoveInput = MoveInput.GetSafeNormal();

		// Only X/Y are touched; Z stays locked to the shoulder's plane in ClampHandTargetToReach().
		HandTargetLocation.X += MoveInput.X * HandMoveSpeed * DeltaTime;
		HandTargetLocation.Y += MoveInput.Y * HandMoveSpeed * DeltaTime;
	}
}

void UArmControlComponent::ClampHandTargetToReach()
{
	// Keep upperarm / lowerarm / hand coplanar by holding the hand on the shoulder's Z plane.
	HandTargetLocation.Z = ShoulderWorldLocation.Z;

	const float MaxReach = UpperArmLength + LowerArmLength;
	FVector OffsetFromShoulder = HandTargetLocation - ShoulderWorldLocation;
	const float Distance = OffsetFromShoulder.Size();

	if (Distance > MaxReach && Distance > KINDA_SMALL_NUMBER)
	{
		// Preserve direction, clamp the magnitude to the arm's maximum reach (L1 + L2).
		HandTargetLocation = ShoulderWorldLocation + OffsetFromShoulder.GetSafeNormal() * MaxReach;
	}
}

FVector2D UArmControlComponent::ComputeTargetElbowPosition(float& OutThetaUpper) const
{
	const float L1 = UpperArmLength;
	const float L2 = LowerArmLength;

	const float Dx = HandTargetLocation.X - ShoulderWorldLocation.X;
	const float Dy = HandTargetLocation.Y - ShoulderWorldLocation.Y;

	// Distance is clamped away from zero so the law-of-cosines divisions stay well defined;
	// ClampHandTargetToReach() already keeps it within [0, L1 + L2].
	const float D = FMath::Max(FMath::Sqrt(Dx * Dx + Dy * Dy), KINDA_SMALL_NUMBER);

	// Auxiliary clamp: keep D away from L1 + L2 so the acos arguments below stay away from
	// +/-1, where acos's derivative blows up and a tiny change in D would otherwise produce
	// a large jump in Delta (and therefore theta_upper / theta_lower).
	const float MaxReach = (L1 + L2) * ReachRatio;
	const float ClampedD = FMath::Min(D, MaxReach);

	// alpha: elbow interior angle (falls out of the same geometry; kept for derivation completeness)
	const float CosAlpha = FMath::Clamp((L1 * L1 + L2 * L2 - ClampedD * ClampedD) / (2.0f * L1 * L2), -1.0f, 1.0f);
	const float Alpha = FMath::Acos(CosAlpha);
	(void)Alpha;

	// delta: angle between the shoulder->hand line and the upper arm
	const float CosDelta = FMath::Clamp((L1 * L1 + ClampedD * ClampedD - L2 * L2) / (2.0f * L1 * ClampedD), -1.0f, 1.0f);
	const float Delta = FMath::Acos(CosDelta);

	// beta: world-space direction angle from shoulder to the hand target
	const float Beta = FMath::Atan2(Dy, Dx);

	// theta_upper: world-space direction angle of the upper arm
	OutThetaUpper = Beta - Delta;

	return FVector2D(ShoulderWorldLocation.X + FMath::Cos(OutThetaUpper) * L1, ShoulderWorldLocation.Y + FMath::Sin(OutThetaUpper) * L1);
}

void UArmControlComponent::SolveArmRotations(float DeltaTime)
{
	float TargetThetaUpper = 0.0f;
	const FVector2D TargetElbowPos = ComputeTargetElbowPosition(TargetThetaUpper);

	// Elbow position smoothing: ease the elbow toward its target position instead of snapping
	// to it, softening the residual discontinuity left by the D clamp above.
	PrevElbowPos = FMath::Lerp(PrevElbowPos, TargetElbowPos, SmoothFactor);

	// Re-derive theta_upper / theta_lower from the smoothed elbow position.
	const float SmoothedThetaUpper = FMath::Atan2(PrevElbowPos.Y - ShoulderWorldLocation.Y, PrevElbowPos.X - ShoulderWorldLocation.X);
	const float SmoothedThetaLower = FMath::Atan2(HandTargetLocation.Y - PrevElbowPos.Y, HandTargetLocation.X - PrevElbowPos.X);

	// Angle smoothing: ease theta_upper / theta_lower toward their elbow-derived targets via
	// FInterpTo. FindDeltaAngleRadians keeps this wrap-safe across the +/-PI boundary.
	PrevThetaUpper += FMath::FInterpTo(0.0f, FMath::FindDeltaAngleRadians(PrevThetaUpper, SmoothedThetaUpper), DeltaTime, SmoothSpeed);
	PrevThetaLower += FMath::FInterpTo(0.0f, FMath::FindDeltaAngleRadians(PrevThetaLower, SmoothedThetaLower), DeltaTime, SmoothSpeed);

	// Both bones rotate purely within the XY plane (yaw about world Z); pitch/roll stay at zero
	// so the chain never twists out of plane.
	UpperArmRotation = FRotator(0.0f, FMath::RadiansToDegrees(PrevThetaUpper), 0.0f);
	LowerArmRotation = FRotator(0.0f, FMath::RadiansToDegrees(PrevThetaLower), 0.0f);
}

void UArmControlComponent::PushRotationsToAnimInstance() const
{
	if (UMPAnimInstance* ArmAnimInstance = Cast<UMPAnimInstance>(SkeletalMesh->GetAnimInstance()))
	{
		ArmAnimInstance->UpperArmRotation = UpperArmRotation;
		ArmAnimInstance->LowerArmRotation = LowerArmRotation;
	}
}
