// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KumaStoryTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KumaStoryFlowSubsystem.generated.h"

class UDataTable;
class UKumaDialogueSubsystem;

UCLASS()
class KUMAMARU_API UKumaStoryFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UKumaStoryFlowSubsystem();

	UFUNCTION(BlueprintCallable, Category = "Kuma Story")
	bool StartDefaultStory();

	UFUNCTION(BlueprintCallable, Category = "Kuma Story")
	bool StartStory(FName StartStepId);

	UFUNCTION(BlueprintCallable, Category = "Kuma Story")
	void ResetStory();

	UFUNCTION(BlueprintCallable, Category = "Kuma Story|Dialogue")
	bool AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "Kuma Story|Sequence")
	bool NotifySequenceFinished(FName SequenceId, FName ResultId);

	UFUNCTION(BlueprintCallable, Category = "Kuma Story|MiniGame")
	bool NotifyMiniGameFinished(FName MiniGameId, FName ResultId);

	UFUNCTION(BlueprintCallable, Category = "Kuma Story|MiniGame")
	bool NotifyMiniGameResult(FName MiniGameId, bool bSucceeded);

	UFUNCTION(BlueprintCallable, Category = "Kuma Story|Data")
	bool SetStoryTables(UDataTable* InStoryStepTable, UDataTable* InStoryBranchTable);

	UFUNCTION(BlueprintCallable, Category = "Kuma Story|Data")
	bool LoadStoryTablesFromPaths(const FString& StoryStepTablePath, const FString& StoryBranchTablePath);

	UFUNCTION(BlueprintCallable, Category = "Kuma Story|Save")
	bool ResumeRestoredStory();

	UFUNCTION(BlueprintPure, Category = "Kuma Story")
	bool IsStoryActive() const;

	UFUNCTION(BlueprintPure, Category = "Kuma Story|Dialogue")
	bool IsDialogueTyping() const;

	UFUNCTION(BlueprintPure, Category = "Kuma Story")
	FName GetCurrentStepId() const { return CurrentStepId; }

	UFUNCTION(BlueprintPure, Category = "Kuma Story")
	FName GetCurrentStepType() const { return CurrentStepType; }

	UFUNCTION(BlueprintPure, Category = "Kuma Story")
	FName GetCurrentTargetId() const { return CurrentTargetId; }

	UFUNCTION(BlueprintPure, Category = "Kuma Story|Save")
	FName GetRestoredStoryStepId() const { return RestoredStoryStepId; }

	const FKumaStoryStepRow* FindStoryStep(FName StepId) const;
	const FKumaStoryBranchRow* FindStoryBranch(FName FromStepId, FName ResultId) const;

	void CaptureStorySaveData(FKumaStorySaveData& OutData) const;
	void RestoreStorySaveData(const FKumaStorySaveData& InData);

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStoryStepStarted OnStoryStepStarted;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStoryStepCompleted OnStoryStepCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStoryFlowFinished OnStoryFlowFinished;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStorySequenceRequested OnSequenceRequested;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStoryMiniGameRequested OnMiniGameRequested;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStoryDialogueClosed OnDialogueClosed;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStoryDialogueLineStarted OnDialogueLineStarted;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStoryDialogueTextUpdated OnDialogueTextUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Story|Events")
	FKumaStoryDialogueLineCompleted OnDialogueLineCompleted;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool ExecuteStep(FName StepId);
	bool ExecuteDialogueStep(FName StepId, const FKumaStoryStepRow& StepRow);
	bool ExecuteSequenceStep(FName StepId, const FKumaStoryStepRow& StepRow);
	bool ExecuteMiniGameStep(FName StepId, const FKumaStoryStepRow& StepRow);
	bool CompleteCurrentStep(FName ResultId);
	bool AdvanceFromResult(FName FromStepId, FName ResultId);
	void CloseDialogueForNonDialogueStep(FName NextStepId, const FKumaStoryStepRow& StepRow);
	bool ValidateStoryTables() const;
	bool IsCurrentStepType(const TCHAR* TypeName) const;
	bool IsStepType(FName StepType, const TCHAR* TypeName) const;
	bool IsStepTypeAny(FName StepType, const TCHAR* FirstTypeName, const TCHAR* SecondTypeName) const;
	FName ResolveStepId(FName RowName, const FKumaStoryStepRow& StepRow) const;
	UKumaDialogueSubsystem* GetDialogueSubsystem() const;
	void BindDialogueSubsystem();
	void UnbindDialogueSubsystem();

	UFUNCTION()
	void HandleDialogueLineStarted(FName LineId, FName SpeakerId, const FText& FullText);

	UFUNCTION()
	void HandleDialogueTextUpdated(const FText& PartialText);

	UFUNCTION()
	void HandleDialogueLineCompleted(FName LineId);

	UFUNCTION()
	void HandleDialogueClosed(FName LastLineId);

	UFUNCTION()
	void HandleDialogueSequenceFinished(FName OwnerId, FName LastLineId);

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Story")
	FName DefaultStartStepId = FName(TEXT("CH01_SEQ_01"));

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Story|Data")
	FString DefaultStoryStepTablePath = TEXT("/Game/Test/Data/T_DataTables/DT_KumaStoryStep.DT_KumaStoryStep");

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Story|Data")
	FString DefaultStoryBranchTablePath = TEXT("/Game/Test/Data/T_DataTables/DT_KumaStoryBranch.DT_KumaStoryBranch");

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> StoryStepTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> StoryBranchTable;

	FName CurrentStepId;
	FName CurrentStepType;
	FName CurrentTargetId;
	FName LastResultId;
	FName RestoredStoryStepId;
	FName WaitingStoryDialogueStepId;
	bool bWaitingForStoryDialogue = false;
	TArray<FName> CompletedStepIds;
};
