// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KumaDialogueSubsystem.h"
#include "KumaChapterTwoDirector.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class UKumaDialogueBoxWidget;
class UKumaChapterEndWidget;
class UKumaInputRouterComponent;
class UKumaMiniGame1Widget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKumaChapterTwoReadyForMiniGame1);

enum class EKumaMiniGame1Phase : uint8
{
	Inactive,
	WaitingForFirstPrompt,
	WaitingForFirstDirection,
	WaitingForFirstDialogue,
	WaitingForSecondPrompt,
	WaitingForSecondDirection,
	WaitingForSecondDialogue,
	WaitingForButtonPrompt,
	ButtonCountdown,
	SuccessSequence,
	SuccessDialogue,
	FailureDialogue,
	FailureFade,
	Complete
};

/** Plays Chapter 2 from its intro fade until MiniGame 1 is ready to begin. */
UCLASS()
class KUMAMARU_API AKumaChapterTwoDirector : public AActor
{
	GENERATED_BODY()

public:
	AKumaChapterTwoDirector();

	/** Called by the Chapter 2 title UI after its fade has fully completed. */
	void StartChapterTwo();

	UPROPERTY(BlueprintAssignable, Category = "Kuma Chapter 2|Events")
	FKumaChapterTwoReadyForMiniGame1 OnChapterTwoReadyForMiniGame1;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void StartOpeningNarration();
	void StartHankCall();
	void StartPostHankNarration();
	void StartMotherEnter();
	void StartMotherDialogue();
	void StartMotherMachineTreatment();
	void TryStartPostMachineDialogue();
	void StartPostMachineDialogue();
	void StartMotherExit();
	void StartHeatNarration();
	void FinishBeforeMiniGame1();
	void StartMiniGame1();
	void StartMiniGame1SecondDirectionPrompt();
	void StartMiniGame1ButtonCountdown();
	void BeginMiniGame1ButtonCountdown();
	void StartMiniGame1Success();
	void StartMiniGame1SuccessDialogue();
	void StartMiniGame1Failure();
	void RestartMiniGame1();
	void PollMiniGame1DirectionKeys();
	void DeferMiniGame1ArmInputLock();
	void LockMiniGame1ArmInput();
	void BindMiniGame1InputRouter();
	bool IsMiniGame1ButtonInteractionAvailable() const;
	void TriggerMiniGame1Shake() const;
	void ShowChapterEndMenu();

	void CreateDialogueWidget();
	void SetPlayerArmInputEnabled(bool bEnabled) const;
	ULevelSequencePlayer* CreateSequencePlayer(const TSoftObjectPtr<ULevelSequence>& SequenceAsset, bool bLooping, TObjectPtr<ALevelSequenceActor>& OutSequenceActor) const;
	void ApplyCTestBindingOverrides(ULevelSequence* SequenceAsset, ALevelSequenceActor* SequenceActor) const;
	void LockMotherInMachineFinalPose() const;
	void PlayDialogue(FName OwnerId, const TArray<FKumaDialogueLine>& Lines) const;
	FKumaDialogueLine MakeAutoLine(FName LineId, FName SpeakerId, const TCHAR* Text) const;
	UKumaDialogueSubsystem* GetDialogueSubsystem() const;

	TArray<FKumaDialogueLine> BuildOpeningDialogue() const;
	TArray<FKumaDialogueLine> BuildHankDialogue() const;
	TArray<FKumaDialogueLine> BuildPostHankDialogue() const;
	TArray<FKumaDialogueLine> BuildMotherDialogue() const;
	TArray<FKumaDialogueLine> BuildMachineDialogue() const;
	TArray<FKumaDialogueLine> BuildPostMachineDialogue() const;
	TArray<FKumaDialogueLine> BuildHeatDialogue() const;
	TArray<FKumaDialogueLine> BuildMiniGame1FirstMoveDialogue() const;
	TArray<FKumaDialogueLine> BuildMiniGame1SecondMoveDialogue() const;
	TArray<FKumaDialogueLine> BuildMiniGame1SuccessDialogue() const;
	TArray<FKumaDialogueLine> BuildMiniGame1FailureDialogue() const;

	UFUNCTION()
	void HandleDialogueSequenceFinished(FName OwnerId, FName LastLineId);

	UFUNCTION()
	void HandleMotherEnterSequenceFinished();

	UFUNCTION()
	void HandleMotherMachineSequenceFinished();

	UFUNCTION()
	void HandleMotherExitSequenceFinished();

	UFUNCTION()
	void HandleMiniGame1EndSequenceFinished();

	UFUNCTION()
	void HandleMiniGame1DirectionInput(FVector2D DirectionValue, float DeltaTime);

	UFUNCTION()
	void HandleMiniGame1InstructionFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 2|Assets")
	TSoftObjectPtr<ULevelSequence> HankSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 2|Assets")
	TSoftObjectPtr<ULevelSequence> MotherEnterSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 2|Assets")
	TSoftObjectPtr<ULevelSequence> MotherMachineSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 2|Assets")
	TSoftObjectPtr<ULevelSequence> MotherExitSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 2|Assets")
	TSoftObjectPtr<ULevelSequence> MotherMiniGame1EndSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 2|Timing", meta = (ClampMin = "0.0"))
	float TypingCharactersPerSecond = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 2|Timing", meta = (ClampMin = "0.0"))
	float AutoAdvanceDelaySeconds = 1.f;

	UPROPERTY(Transient)
	TObjectPtr<UKumaDialogueBoxWidget> DialogueWidget;

	UPROPERTY(Transient)
	TObjectPtr<UKumaChapterEndWidget> ChapterEndWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> HankSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> HankSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> MotherEnterSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> MotherEnterSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> MotherMachineSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> MotherMachineSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> MotherExitSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> MotherExitSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> MotherMiniGame1EndSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> MotherMiniGame1EndSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<UKumaMiniGame1Widget> MiniGame1Widget;

	UPROPERTY(Transient)
	TObjectPtr<UKumaInputRouterComponent> MiniGame1InputRouter;

	bool bHasStarted = false;
	bool bHasFinished = false;
	bool bMotherMachineSequenceFinished = false;
	bool bMotherMachineDialogueFinished = false;
	bool bPostMachineDialogueStarted = false;
	bool bMiniGame1DirectionKeyWasDown = false;
	EKumaMiniGame1Phase MiniGame1Phase = EKumaMiniGame1Phase::Inactive;
	float MiniGame1RemainingSeconds = 60.f;
	FTimerHandle MiniGame1RestartTimer;
};
