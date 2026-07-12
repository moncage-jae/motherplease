// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KumaMiniGameBase.h"
#include "KumaMiniGame0CameraAlign.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FKumaMiniGame0AlignmentUpdatedSignature, FVector2D, TargetScreenPosition, FVector2D, ScreenCenter, float, DistanceFromCenter, float, AlignmentProgress);

UCLASS(Blueprintable)
class KUMAMARU_API AKumaMiniGame0CameraAlign : public AKumaMiniGameBase
{
	GENERATED_BODY()

public:
	AKumaMiniGame0CameraAlign();

	virtual void Tick(float DeltaTime) override;
	virtual void HandleKumaMouseDragInput_Implementation(FVector2D DragDelta, float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Kuma MiniGame0|Target")
	void SetTargetActor(AActor* NewTargetActor);

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame0|Target")
	FVector GetTargetWorldLocation() const;

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame0|State")
	float GetCurrentDistanceFromCenter() const { return CurrentDistanceFromCenter; }

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame0|State")
	float GetCurrentAlignmentProgress() const { return CurrentAlignmentProgress; }

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame0|State")
	FVector GetCurrentScreenCenterWorldLocation() const { return CurrentScreenCenterWorldLocation; }

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame0|State")
	FVector GetCurrentTargetOffsetFromCenterWorld() const { return CurrentTargetOffsetFromCenterWorld; }

	UPROPERTY(BlueprintAssignable, Category = "Kuma MiniGame0|Events")
	FKumaMiniGame0AlignmentUpdatedSignature OnAlignmentUpdated;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleMiniGameStarted() override;
	virtual void HandleMiniGameFinished(bool bSucceeded) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Target")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Target")
	FVector TargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Success", meta = (ClampMin = "0.0"))
	float SuccessWorldTolerance = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Success", meta = (ClampMin = "0.0"))
	float RequiredAlignedSeconds = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Failure")
	bool bUseFailureTimeLimit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Failure", meta = (EditCondition = "bUseFailureTimeLimit", ClampMin = "0.1"))
	float TimeLimitSeconds = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Drag", meta = (ClampMin = "0.0"))
	float DragYawSensitivity = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Drag", meta = (ClampMin = "0.0"))
	float DragPitchSensitivity = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Drag", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float MinPitch = -80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma MiniGame0|Drag", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float MaxPitch = 80.f;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame0|State")
	FVector2D CurrentTargetScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame0|State")
	FVector2D CurrentScreenCenter = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame0|State")
	FVector CurrentScreenCenterWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame0|State")
	FVector CurrentTargetOffsetFromCenterWorld = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame0|State")
	float CurrentDistanceFromCenter = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame0|State")
	float CurrentAlignmentProgress = 0.f;

private:
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	bool bDidOverrideMouseCursor = false;
	bool bSavedShowMouseCursor = false;
	float AlignedElapsedSeconds = 0.f;
	float RunningElapsedSeconds = 0.f;

	APlayerController* GetCachedPlayerController();
	void ConfigureMouseCursor();
	void RestoreMouseCursor();
	bool ApplyDragToPawnCamera(APlayerController* PlayerController, FVector2D DragDelta);
	void ApplyDragToRotation(FRotator& Rotation, FVector2D DragDelta) const;
	void UpdateAlignment(float DeltaTime);
	bool GetActiveCameraView(FVector& OutCameraLocation, FRotator& OutCameraRotation) const;
	bool ProjectTargetToScreen(FVector2D& OutTargetScreenPosition, FVector2D& OutScreenCenter) const;
};
