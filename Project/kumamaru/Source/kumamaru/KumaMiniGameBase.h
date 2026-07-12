// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KumaInputReceiver.h"
#include "KumaMiniGameBase.generated.h"

class UKumaStoryFlowSubsystem;
class UKumaInputRouterComponent;

UENUM(BlueprintType)
enum class EKumaMiniGameState : uint8
{
	Idle,
	Running,
	Succeeded,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaMiniGameStartedSignature, FName, StepId, FName, MiniGameId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKumaMiniGameFinishedSignature, FName, StepId, FName, MiniGameId, bool, bSucceeded);

UCLASS(Abstract, Blueprintable)
class KUMAMARU_API AKumaMiniGameBase : public AActor, public IKumaInputReceiver
{
	GENERATED_BODY()

public:
	AKumaMiniGameBase();

	UFUNCTION(BlueprintCallable, Category = "Kuma MiniGame")
	virtual bool StartMiniGame(FName StepId, FName RequestedMiniGameId);

	UFUNCTION(BlueprintCallable, Category = "Kuma MiniGame")
	virtual bool FinishMiniGame(bool bSucceeded);

	UFUNCTION(BlueprintCallable, Category = "Kuma MiniGame")
	bool SucceedMiniGame();

	UFUNCTION(BlueprintCallable, Category = "Kuma MiniGame")
	bool FailMiniGame();

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame")
	bool IsMiniGameRunning() const { return MiniGameState == EKumaMiniGameState::Running; }

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame")
	FName GetMiniGameId() const { return MiniGameId; }

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame")
	FName GetActiveStepId() const { return ActiveStepId; }

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame")
	FName GetActiveMiniGameId() const { return ActiveMiniGameId; }

	UFUNCTION(BlueprintPure, Category = "Kuma MiniGame")
	EKumaMiniGameState GetMiniGameState() const { return MiniGameState; }

	UPROPERTY(BlueprintAssignable, Category = "Kuma MiniGame|Events")
	FKumaMiniGameStartedSignature OnMiniGameStarted;

	UPROPERTY(BlueprintAssignable, Category = "Kuma MiniGame|Events")
	FKumaMiniGameFinishedSignature OnMiniGameFinished;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void HandleMiniGameStarted();
	virtual void HandleMiniGameFinished(bool bSucceeded);

	UFUNCTION(BlueprintImplementableEvent, Category = "Kuma MiniGame|Events")
	void ReceiveMiniGameStarted(FName StepId, FName StartedMiniGameId);

	UFUNCTION(BlueprintImplementableEvent, Category = "Kuma MiniGame|Events")
	void ReceiveMiniGameFinished(FName StepId, FName FinishedMiniGameId, bool bSucceeded);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma MiniGame")
	FName MiniGameId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma MiniGame")
	bool bAutoBindToStoryFlow = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma MiniGame|Input")
	bool bClaimInputWhileRunning = true;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame")
	FName ActiveStepId;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame")
	FName ActiveMiniGameId;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma MiniGame")
	EKumaMiniGameState MiniGameState = EKumaMiniGameState::Idle;

	UKumaStoryFlowSubsystem* GetStoryFlowSubsystem() const;
	UKumaInputRouterComponent* FindInputRouter() const;
	void ClaimInputRouter();
	void ReleaseInputRouter();

private:
	UFUNCTION()
	void HandleStoryMiniGameRequested(FName StepId, FName RequestedMiniGameId);

	bool DoesMiniGameIdMatch(FName RequestedMiniGameId) const;

	TWeakObjectPtr<UKumaInputRouterComponent> ClaimedInputRouter;
};
