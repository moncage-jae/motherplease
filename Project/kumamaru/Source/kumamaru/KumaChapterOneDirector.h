// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KumaDialogueSubsystem.h"
#include "KumaChapterOneDirector.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class UKumaChapterIntroWidget;
class UKumaDialogueBoxWidget;
class UKumaMiniGame0Widget;
class UKumaChapterEndWidget;
class AActor;
class UMediaPlayer;
class UFileMediaSource;
class UMaterialInterface;
class UStaticMeshComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKumaChapterOneReadyForMiniGame0);

/** Plays the non-interactive part of Chapter 1 and stops before MiniGame 0. */
UCLASS()
class KUMAMARU_API AKumaChapterOneDirector : public AActor
{
	GENERATED_BODY()

public:
	AKumaChapterOneDirector();

	UPROPERTY(BlueprintAssignable, Category = "Kuma Chapter 1|Events")
	FKumaChapterOneReadyForMiniGame0 OnChapterOneReadyForMiniGame0;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void StartChapterOne();
	void StartRoomNarration();
	void ScheduleTVVideoAfterFinalButterfly();
	void StartTVVideoPlayback();
	void RestoreTVOriginalMaterial();
	UFUNCTION() void HandleTVMediaOpened(FString OpenedUrl);
	UFUNCTION() void HandleTVMediaOpenFailed(FString FailedUrl);
	UFUNCTION() void HandleTVVideoEnded();
	void StartHankCall();
	void StartMotherEnter();
	void StartMotherEnterDialogue();
	void StartMotherConversation();
	void StartMotherExit();
	void FinishBeforeMiniGame0();
	void TryStartMotherConversation();
	void TryFinishBeforeMiniGame0();
	void StartMiniGame0();
	void PlayButterflyMove(int32 MoveIndex);
	void BeginButterflySearch();
	void HandleButterflyFound();
	void PlayTVSequence();
	void ShowChapterEndMenu();
	void SetMiniGameLookInput(bool bEnabled) const;
	bool IsButterflyHit(AActor* HitActor) const;
	UFUNCTION() void HandleButterflySequenceFinished();
	UFUNCTION() void HandleTVSequenceFinished();

	void CreateDialogueWidget();
	void CreateChapterIntroWidget();
	void SetPlayerArmInputEnabled(bool bEnabled) const;
	ULevelSequencePlayer* CreateSequencePlayer(const TSoftObjectPtr<ULevelSequence>& SequenceAsset, bool bLooping, TObjectPtr<ALevelSequenceActor>& OutSequenceActor, bool bPauseAtEnd = false) const;
	void PlayDialogue(FName OwnerId, const TArray<FKumaDialogueLine>& Lines) const;
	FKumaDialogueLine MakeAutoLine(FName LineId, FName SpeakerId, const TCHAR* Text) const;
	UKumaDialogueSubsystem* GetDialogueSubsystem() const;

	TArray<FKumaDialogueLine> BuildRoomNarration() const;
	TArray<FKumaDialogueLine> BuildHankDialogue() const;
	TArray<FKumaDialogueLine> BuildMotherEnterDialogue() const;
	TArray<FKumaDialogueLine> BuildMotherConversation() const;
	TArray<FKumaDialogueLine> BuildMotherExitDialogue() const;

	UFUNCTION()
	void HandleDialogueSequenceFinished(FName OwnerId, FName LastLineId);

	UFUNCTION()
	void HandleMotherEnterSequenceFinished();

	UFUNCTION()
	void HandleMotherExitSequenceFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Assets")
	TSoftObjectPtr<ULevelSequence> HankSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Assets")
	TSoftObjectPtr<ULevelSequence> MotherEnterSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Assets")
	TSoftObjectPtr<ULevelSequence> MotherExitSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Assets")
	TSoftObjectPtr<UMediaPlayer> TVMediaPlayerAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Assets")
	TSoftObjectPtr<UFileMediaSource> TVMediaSourceAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Assets")
	TSoftObjectPtr<UMaterialInterface> TVVideoMaterialAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Audio")
	TSoftObjectPtr<USoundBase> BirdSound;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Timing", meta = (ClampMin = "0.0"))
	float IntroHoldSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Timing", meta = (ClampMin = "0.0"))
	float IntroFadeSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Timing", meta = (ClampMin = "0.0"))
	float TypingCharactersPerSecond = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Timing", meta = (ClampMin = "0.0"))
	float AutoAdvanceDelaySeconds = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter 1|Timing", meta = (ClampMin = "0.0"))
	float MotherEnterDialogueDelaySeconds = 1.f;

	UPROPERTY(Transient)
	TObjectPtr<UKumaDialogueBoxWidget> DialogueWidget;

	UPROPERTY(Transient)
	TObjectPtr<UKumaChapterIntroWidget> ChapterIntroWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> HankSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> HankSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> MotherEnterSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> MotherEnterSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> MotherExitSequencePlayer;
	UPROPERTY(Transient) TObjectPtr<ULevelSequencePlayer> ButterflySequencePlayer;
	UPROPERTY(Transient) TObjectPtr<ULevelSequencePlayer> TVSequencePlayer;
	UPROPERTY(Transient) TObjectPtr<ALevelSequenceActor> ButterflySequenceActor;
	UPROPERTY(Transient) TObjectPtr<ALevelSequenceActor> TVSequenceActor;
	UPROPERTY(Transient) TObjectPtr<UKumaMiniGame0Widget> MiniGame0Widget;
	UPROPERTY(Transient) TObjectPtr<UKumaChapterEndWidget> ChapterEndWidget;
	UPROPERTY(Transient) TObjectPtr<UMediaPlayer> ActiveTVMediaPlayer;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ActiveTVMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OriginalTVMaterial;
	TSoftObjectPtr<ULevelSequence> ButterflySequences[3];
	TSoftObjectPtr<ULevelSequence> TVSequence;
	int32 ButterflyMoveIndex = 0;
	bool bMiniGame0Searching = false;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> MotherExitSequenceActor;

	bool bHasStarted = false;
	bool bHasFinished = false;
	bool bMotherEnterSequenceFinished = false;
	bool bMotherEnterDialogueFinished = false;
	bool bMotherExitSequenceFinished = false;
	bool bMotherExitDialogueFinished = false;
	FTimerHandle MotherEnterDialogueTimerHandle;
	FTimerHandle TVVideoStartTimer;
};
