// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaMiniGame0CameraAlign.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AKumaMiniGame0CameraAlign::AKumaMiniGame0CameraAlign()
{
	MiniGameId = FName(TEXT("MG_CH01_Tutorial"));
}

void AKumaMiniGame0CameraAlign::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsMiniGameRunning())
	{
		return;
	}

	RunningElapsedSeconds += DeltaTime;

	UpdateAlignment(DeltaTime);

	if (!IsMiniGameRunning())
	{
		return;
	}

	if (bUseFailureTimeLimit && RunningElapsedSeconds >= TimeLimitSeconds)
	{
		FailMiniGame();
	}
}

void AKumaMiniGame0CameraAlign::SetTargetActor(AActor* NewTargetActor)
{
	TargetActor = NewTargetActor;
}

FVector AKumaMiniGame0CameraAlign::GetTargetWorldLocation() const
{
	return TargetActor ? TargetActor->GetActorLocation() : TargetWorldLocation;
}

void AKumaMiniGame0CameraAlign::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreMouseCursor();

	Super::EndPlay(EndPlayReason);
}

void AKumaMiniGame0CameraAlign::HandleMiniGameStarted()
{
	Super::HandleMiniGameStarted();

	CachedPlayerController = UGameplayStatics::GetPlayerController(this, 0);
	AlignedElapsedSeconds = 0.f;
	RunningElapsedSeconds = 0.f;
	CurrentDistanceFromCenter = 0.f;
	CurrentAlignmentProgress = 0.f;
	CurrentScreenCenterWorldLocation = FVector::ZeroVector;
	CurrentTargetOffsetFromCenterWorld = FVector::ZeroVector;

	ConfigureMouseCursor();
}

void AKumaMiniGame0CameraAlign::HandleMiniGameFinished(bool bSucceeded)
{
	RestoreMouseCursor();

	Super::HandleMiniGameFinished(bSucceeded);
}

void AKumaMiniGame0CameraAlign::HandleKumaMouseDragInput_Implementation(FVector2D DragDelta, float DeltaTime)
{
	APlayerController* PlayerController = GetCachedPlayerController();
	if (!PlayerController || DragDelta.IsNearlyZero())
	{
		return;
	}

	if (!ApplyDragToPawnCamera(PlayerController, DragDelta))
	{
		FRotator ControlRotation = PlayerController->GetControlRotation().GetNormalized();
		ApplyDragToRotation(ControlRotation, DragDelta);
		PlayerController->SetControlRotation(ControlRotation);
	}
}

APlayerController* AKumaMiniGame0CameraAlign::GetCachedPlayerController()
{
	if (!CachedPlayerController.IsValid())
	{
		CachedPlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	return CachedPlayerController.Get();
}

void AKumaMiniGame0CameraAlign::ConfigureMouseCursor()
{
	if (bDidOverrideMouseCursor)
	{
		return;
	}

	APlayerController* PlayerController = GetCachedPlayerController();
	if (!PlayerController)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	PlayerController->SetInputMode(InputMode);

	bSavedShowMouseCursor = PlayerController->bShowMouseCursor;
	PlayerController->bShowMouseCursor = true;
	bDidOverrideMouseCursor = true;
}

void AKumaMiniGame0CameraAlign::RestoreMouseCursor()
{
	if (!bDidOverrideMouseCursor)
	{
		return;
	}

	if (APlayerController* PlayerController = GetCachedPlayerController())
	{
		PlayerController->bShowMouseCursor = bSavedShowMouseCursor;
	}

	bDidOverrideMouseCursor = false;
}

bool AKumaMiniGame0CameraAlign::ApplyDragToPawnCamera(APlayerController* PlayerController, FVector2D DragDelta)
{
	if (!PlayerController)
	{
		return false;
	}

	APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return false;
	}

	if (UCameraComponent* Camera = Pawn->FindComponentByClass<UCameraComponent>())
	{
		FRotator CameraRotation = Camera->GetComponentRotation().GetNormalized();
		ApplyDragToRotation(CameraRotation, DragDelta);
		Camera->SetWorldRotation(CameraRotation);
		return true;
	}

	return false;
}

void AKumaMiniGame0CameraAlign::ApplyDragToRotation(FRotator& Rotation, FVector2D DragDelta) const
{
	Rotation.Yaw -= DragDelta.X * DragYawSensitivity;
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch - DragDelta.Y * DragPitchSensitivity, MinPitch, MaxPitch);
	Rotation.Roll = 0.f;
	Rotation.Normalize();
}

void AKumaMiniGame0CameraAlign::UpdateAlignment(float DeltaTime)
{
	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetActiveCameraView(CameraLocation, CameraRotation))
	{
		AlignedElapsedSeconds = 0.f;
		CurrentAlignmentProgress = 0.f;
		return;
	}

	const FVector TargetDirection = (GetTargetWorldLocation() - CameraLocation).GetSafeNormal();
	const FVector CameraForward = CameraRotation.Vector().GetSafeNormal();
	if (TargetDirection.IsNearlyZero() || CameraForward.IsNearlyZero())
	{
		AlignedElapsedSeconds = 0.f;
		CurrentAlignmentProgress = 0.f;
		return;
	}

	const FVector TargetLocation = GetTargetWorldLocation();
	const FVector ToTarget = TargetLocation - CameraLocation;
	const float DistanceAlongCenter = FVector::DotProduct(ToTarget, CameraForward);
	if (DistanceAlongCenter < 0.f)
	{
		AlignedElapsedSeconds = 0.f;
		CurrentAlignmentProgress = 0.f;
		return;
	}

	CurrentScreenCenterWorldLocation = CameraLocation + CameraForward * DistanceAlongCenter;
	CurrentTargetOffsetFromCenterWorld = TargetLocation - CurrentScreenCenterWorldLocation;
	CurrentDistanceFromCenter = CurrentTargetOffsetFromCenterWorld.Size();

	const float Tolerance = FMath::Max(0.f, SuccessWorldTolerance);
	const bool bAlignedToTarget =
		FMath::Abs(CurrentTargetOffsetFromCenterWorld.X) <= Tolerance &&
		FMath::Abs(CurrentTargetOffsetFromCenterWorld.Y) <= Tolerance &&
		FMath::Abs(CurrentTargetOffsetFromCenterWorld.Z) <= Tolerance;

	CurrentAlignmentProgress = 1.f - FMath::Clamp(CurrentDistanceFromCenter / FMath::Max(1.f, Tolerance), 0.f, 1.f);

	FVector2D TargetScreenPosition;
	FVector2D ScreenCenter;
	if (ProjectTargetToScreen(TargetScreenPosition, ScreenCenter))
	{
		CurrentTargetScreenPosition = TargetScreenPosition;
		CurrentScreenCenter = ScreenCenter;
		OnAlignmentUpdated.Broadcast(CurrentTargetScreenPosition, CurrentScreenCenter, CurrentDistanceFromCenter, CurrentAlignmentProgress);
	}

	if (bAlignedToTarget)
	{
		AlignedElapsedSeconds += DeltaTime;
		if (AlignedElapsedSeconds >= RequiredAlignedSeconds)
		{
			SucceedMiniGame();
		}
	}
	else
	{
		AlignedElapsedSeconds = 0.f;
	}
}

bool AKumaMiniGame0CameraAlign::GetActiveCameraView(FVector& OutCameraLocation, FRotator& OutCameraRotation) const
{
	APlayerController* PlayerController = CachedPlayerController.Get();
	if (!PlayerController)
	{
		return false;
	}

	if (APawn* Pawn = PlayerController->GetPawn())
	{
		if (UCameraComponent* Camera = Pawn->FindComponentByClass<UCameraComponent>())
		{
			OutCameraLocation = Camera->GetComponentLocation();
			OutCameraRotation = Camera->GetComponentRotation();
			return true;
		}
	}

	if (PlayerController->PlayerCameraManager)
	{
		OutCameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		OutCameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		return true;
	}

	return false;
}

bool AKumaMiniGame0CameraAlign::ProjectTargetToScreen(FVector2D& OutTargetScreenPosition, FVector2D& OutScreenCenter) const
{
	APlayerController* PlayerController = CachedPlayerController.Get();
	if (!PlayerController)
	{
		return false;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return false;
	}

	OutScreenCenter = FVector2D(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
	return UGameplayStatics::ProjectWorldToScreen(PlayerController, GetTargetWorldLocation(), OutTargetScreenPosition, true);
}
