// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "KumaStoryTypes.generated.h"

USTRUCT(BlueprintType)
struct FKumaStoryStepRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story")
	FName StepId;

	// Expected values: Dialogue, Sequence, MiniGame.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story")
	FName StepType = FName(TEXT("Dialogue"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story|Dialogue")
	FName SpeakerId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story|Dialogue", meta = (MultiLine = "true"))
	FText DialogueText;

	// SequenceId for Sequence steps, MiniGameId for MiniGame steps.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story")
	FName TargetId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story|Dialogue", meta = (ClampMin = "0.0"))
	float TypingSpeed = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story")
	TArray<FName> Tags;
};

USTRUCT(BlueprintType)
struct FKumaStoryBranchRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story|Branch")
	FName FromStepId;

	// Examples: Completed, Finished, Success, Failure, Timeout, Perfect.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story|Branch")
	FName ResultId = FName(TEXT("Completed"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story|Branch")
	FName ToStepId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kuma Story|Branch")
	FString Notes;
};

USTRUCT(BlueprintType)
struct FKumaStorySaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Story|Save")
	bool bHasStoryStep = false;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Story|Save")
	FName CurrentStepId;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Story|Save")
	FName LastResultId;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Story|Save")
	TArray<FName> CompletedStepIds;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKumaStoryStepStarted, FName, StepId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaStoryStepCompleted, FName, StepId, FName, ResultId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaStoryFlowFinished, FName, LastStepId, FName, ResultId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaStorySequenceRequested, FName, StepId, FName, SequenceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaStoryMiniGameRequested, FName, StepId, FName, MiniGameId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKumaStoryDialogueClosed, FName, NextStepId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKumaStoryDialogueLineStarted, FName, StepId, FName, SpeakerId, const FText&, FullText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKumaStoryDialogueTextUpdated, const FText&, PartialText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKumaStoryDialogueLineCompleted, FName, StepId);
