// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaStoryFlowSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "KumaDialogueTypingComponent.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogKumaStory, Log, All);

namespace KumaStoryResult
{
	static const FName Completed(TEXT("Completed"));
	static const FName Finished(TEXT("Finished"));
	static const FName Success(TEXT("Success"));
	static const FName Failure(TEXT("Failure"));
}

UKumaStoryFlowSubsystem::UKumaStoryFlowSubsystem()
{
}

void UKumaStoryFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadStoryTablesFromPaths(DefaultStoryStepTablePath, DefaultStoryBranchTablePath);

	TypingComponent = NewObject<UKumaDialogueTypingComponent>(this);
	if (TypingComponent)
	{
		TypingComponent->OnCharacterRevealed.AddUObject(this, &UKumaStoryFlowSubsystem::HandleTypingTextUpdated);
		TypingComponent->OnTypingCompleted.AddUObject(this, &UKumaStoryFlowSubsystem::HandleTypingCompleted);
	}
}

void UKumaStoryFlowSubsystem::Deinitialize()
{
	if (TypingComponent)
	{
		if (UWorld* World = GetWorld())
		{
			TypingComponent->CancelTyping(World->GetTimerManager());
		}

		TypingComponent->OnCharacterRevealed.RemoveAll(this);
		TypingComponent->OnTypingCompleted.RemoveAll(this);
	}

	ResetStory();

	Super::Deinitialize();
}

bool UKumaStoryFlowSubsystem::StartDefaultStory()
{
	ResetStory();
	return StartStory(DefaultStartStepId);
}

bool UKumaStoryFlowSubsystem::StartStory(FName StartStepId)
{
	if (StartStepId.IsNone())
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] StartStory ignored because StartStepId is None."));
		return false;
	}

	RestoredStoryStepId = NAME_None;
	return ExecuteStep(StartStepId);
}

void UKumaStoryFlowSubsystem::ResetStory()
{
	CurrentStepId = NAME_None;
	CurrentStepType = NAME_None;
	CurrentTargetId = NAME_None;
	LastResultId = NAME_None;
	RestoredStoryStepId = NAME_None;
	CompletedStepIds.Reset();
}

bool UKumaStoryFlowSubsystem::AdvanceDialogue()
{
	if (!IsCurrentStepType(TEXT("Dialogue")) && !IsCurrentStepType(TEXT("Dialog")))
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] AdvanceDialogue ignored because current step is not Dialogue. CurrentStep=%s"), *CurrentStepId.ToString());
		return false;
	}

	if (!TypingComponent)
	{
		return CompleteCurrentStep(KumaStoryResult::Completed);
	}

	if (TypingComponent->IsTyping())
	{
		if (UWorld* World = GetWorld())
		{
			TypingComponent->SkipToEnd(World->GetTimerManager());
			return true;
		}
	}

	return CompleteCurrentStep(KumaStoryResult::Completed);
}

bool UKumaStoryFlowSubsystem::NotifySequenceFinished(FName SequenceId, FName ResultId)
{
	if (!IsCurrentStepType(TEXT("Sequence")))
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] Sequence result ignored because current step is not Sequence. CurrentStep=%s"), *CurrentStepId.ToString());
		return false;
	}

	if (!CurrentTargetId.IsNone() && SequenceId != CurrentTargetId)
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] SequenceId mismatch. Expected=%s Actual=%s"), *CurrentTargetId.ToString(), *SequenceId.ToString());
		return false;
	}

	return CompleteCurrentStep(ResultId.IsNone() ? KumaStoryResult::Finished : ResultId);
}

bool UKumaStoryFlowSubsystem::NotifyMiniGameFinished(FName MiniGameId, FName ResultId)
{
	if (!IsCurrentStepType(TEXT("MiniGame")) && !IsCurrentStepType(TEXT("Minigame")))
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] MiniGame result ignored because current step is not MiniGame. CurrentStep=%s"), *CurrentStepId.ToString());
		return false;
	}

	if (ResultId.IsNone())
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] MiniGame result ignored because ResultId is None. MiniGame=%s"), *MiniGameId.ToString());
		return false;
	}

	if (!CurrentTargetId.IsNone() && MiniGameId != CurrentTargetId)
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] MiniGameId mismatch. Expected=%s Actual=%s"), *CurrentTargetId.ToString(), *MiniGameId.ToString());
		return false;
	}

	return CompleteCurrentStep(ResultId);
}

bool UKumaStoryFlowSubsystem::NotifyMiniGameResult(FName MiniGameId, bool bSucceeded)
{
	return NotifyMiniGameFinished(MiniGameId, bSucceeded ? KumaStoryResult::Success : KumaStoryResult::Failure);
}

bool UKumaStoryFlowSubsystem::SetStoryTables(UDataTable* InStoryStepTable, UDataTable* InStoryBranchTable)
{
	StoryStepTable = InStoryStepTable;
	StoryBranchTable = InStoryBranchTable;
	return ValidateStoryTables();
}

bool UKumaStoryFlowSubsystem::LoadStoryTablesFromPaths(const FString& StoryStepTablePath, const FString& StoryBranchTablePath)
{
	UDataTable* LoadedStepTable = StoryStepTablePath.IsEmpty() ? nullptr : LoadObject<UDataTable>(nullptr, *StoryStepTablePath);
	UDataTable* LoadedBranchTable = StoryBranchTablePath.IsEmpty() ? nullptr : LoadObject<UDataTable>(nullptr, *StoryBranchTablePath);

	if (!LoadedStepTable)
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] Story step table was not loaded. Path=%s"), *StoryStepTablePath);
	}

	if (!LoadedBranchTable)
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] Story branch table was not loaded. Path=%s"), *StoryBranchTablePath);
	}

	StoryStepTable = LoadedStepTable;
	StoryBranchTable = LoadedBranchTable;
	return ValidateStoryTables();
}

bool UKumaStoryFlowSubsystem::ResumeRestoredStory()
{
	if (RestoredStoryStepId.IsNone())
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] ResumeRestoredStory ignored because there is no restored story step."));
		return false;
	}

	const FName StepIdToResume = RestoredStoryStepId;
	RestoredStoryStepId = NAME_None;
	return StartStory(StepIdToResume);
}

bool UKumaStoryFlowSubsystem::IsStoryActive() const
{
	return !CurrentStepId.IsNone();
}

bool UKumaStoryFlowSubsystem::IsDialogueTyping() const
{
	return TypingComponent && TypingComponent->IsTyping();
}

const FKumaStoryStepRow* UKumaStoryFlowSubsystem::FindStoryStep(FName StepId) const
{
	if (!StoryStepTable || StepId.IsNone())
	{
		return nullptr;
	}

	if (const FKumaStoryStepRow* Row = StoryStepTable->FindRow<FKumaStoryStepRow>(StepId, TEXT("UKumaStoryFlowSubsystem::FindStoryStep")))
	{
		return Row;
	}

	for (const TPair<FName, uint8*>& Pair : StoryStepTable->GetRowMap())
	{
		const FKumaStoryStepRow* Row = reinterpret_cast<const FKumaStoryStepRow*>(Pair.Value);
		if (Row && Row->StepId == StepId)
		{
			return Row;
		}
	}

	return nullptr;
}

const FKumaStoryBranchRow* UKumaStoryFlowSubsystem::FindStoryBranch(FName FromStepId, FName ResultId) const
{
	if (!StoryBranchTable || FromStepId.IsNone() || ResultId.IsNone())
	{
		return nullptr;
	}

	for (const TPair<FName, uint8*>& Pair : StoryBranchTable->GetRowMap())
	{
		const FKumaStoryBranchRow* Row = reinterpret_cast<const FKumaStoryBranchRow*>(Pair.Value);
		if (Row && Row->FromStepId == FromStepId && Row->ResultId == ResultId)
		{
			return Row;
		}
	}

	return nullptr;
}

void UKumaStoryFlowSubsystem::CaptureStorySaveData(FKumaStorySaveData& OutData) const
{
	OutData.bHasStoryStep = !CurrentStepId.IsNone();
	OutData.CurrentStepId = CurrentStepId;
	OutData.LastResultId = LastResultId;
	OutData.CompletedStepIds = CompletedStepIds;
}

void UKumaStoryFlowSubsystem::RestoreStorySaveData(const FKumaStorySaveData& InData)
{
	CompletedStepIds = InData.CompletedStepIds;
	LastResultId = InData.LastResultId;
	RestoredStoryStepId = InData.bHasStoryStep ? InData.CurrentStepId : NAME_None;
	CurrentStepId = NAME_None;
	CurrentStepType = NAME_None;
	CurrentTargetId = NAME_None;
}

bool UKumaStoryFlowSubsystem::ExecuteStep(FName StepId)
{
	const FKumaStoryStepRow* StepRow = FindStoryStep(StepId);
	if (!StepRow)
	{
		UE_LOG(LogKumaStory, Error, TEXT("[KumaStory] Step not found: %s"), *StepId.ToString());
		return false;
	}

	const FName ResolvedStepId = ResolveStepId(StepId, *StepRow);
	CurrentStepId = ResolvedStepId;
	CurrentStepType = StepRow->StepType;
	CurrentTargetId = StepRow->TargetId;

	UE_LOG(LogKumaStory, Log, TEXT("[KumaStory] Execute Step=%s Type=%s Target=%s"), *CurrentStepId.ToString(), *CurrentStepType.ToString(), *CurrentTargetId.ToString());
	OnStoryStepStarted.Broadcast(CurrentStepId);
	CloseDialogueForNonDialogueStep(CurrentStepId, *StepRow);

	if (IsStepTypeAny(StepRow->StepType, TEXT("Dialogue"), TEXT("Dialog")))
	{
		return ExecuteDialogueStep(ResolvedStepId, *StepRow);
	}

	if (IsStepType(StepRow->StepType, TEXT("Sequence")))
	{
		return ExecuteSequenceStep(ResolvedStepId, *StepRow);
	}

	if (IsStepTypeAny(StepRow->StepType, TEXT("MiniGame"), TEXT("Minigame")))
	{
		return ExecuteMiniGameStep(ResolvedStepId, *StepRow);
	}

	UE_LOG(LogKumaStory, Error, TEXT("[KumaStory] Unknown StepType. Step=%s Type=%s"), *ResolvedStepId.ToString(), *StepRow->StepType.ToString());
	return false;
}

bool UKumaStoryFlowSubsystem::ExecuteDialogueStep(FName StepId, const FKumaStoryStepRow& StepRow)
{
	SetDialogueWidgetsVisible(true);
	OnDialogueLineStarted.Broadcast(StepId, StepRow.SpeakerId, StepRow.DialogueText);

	if (!TypingComponent)
	{
		OnDialogueTextUpdated.Broadcast(StepRow.DialogueText);
		OnDialogueLineCompleted.Broadcast(StepId);
		return true;
	}

	if (UWorld* World = GetWorld())
	{
		TypingComponent->StartTyping(StepRow.DialogueText, StepRow.TypingSpeed, World->GetTimerManager());
		return true;
	}

	OnDialogueTextUpdated.Broadcast(StepRow.DialogueText);
	OnDialogueLineCompleted.Broadcast(StepId);
	return true;
}

bool UKumaStoryFlowSubsystem::ExecuteSequenceStep(FName StepId, const FKumaStoryStepRow& StepRow)
{
	if (StepRow.TargetId.IsNone())
	{
		UE_LOG(LogKumaStory, Error, TEXT("[KumaStory] Sequence step has no TargetId. Step=%s"), *StepId.ToString());
		return false;
	}

	OnSequenceRequested.Broadcast(StepId, StepRow.TargetId);
	return true;
}

bool UKumaStoryFlowSubsystem::ExecuteMiniGameStep(FName StepId, const FKumaStoryStepRow& StepRow)
{
	if (StepRow.TargetId.IsNone())
	{
		UE_LOG(LogKumaStory, Error, TEXT("[KumaStory] MiniGame step has no TargetId. Step=%s"), *StepId.ToString());
		return false;
	}

	UE_LOG(LogKumaStory, Log, TEXT("[KumaStory] Request MiniGame Step=%s MiniGame=%s"), *StepId.ToString(), *StepRow.TargetId.ToString());
	OnMiniGameRequested.Broadcast(StepId, StepRow.TargetId);
	return true;
}

bool UKumaStoryFlowSubsystem::CompleteCurrentStep(FName ResultId)
{
	if (CurrentStepId.IsNone())
	{
		UE_LOG(LogKumaStory, Warning, TEXT("[KumaStory] CompleteCurrentStep ignored because there is no current step."));
		return false;
	}

	if (ResultId.IsNone())
	{
		ResultId = KumaStoryResult::Completed;
	}

	const FName CompletedStepId = CurrentStepId;
	LastResultId = ResultId;
	CompletedStepIds.AddUnique(CompletedStepId);

	UE_LOG(LogKumaStory, Log, TEXT("[KumaStory] Complete Step=%s Result=%s"), *CompletedStepId.ToString(), *ResultId.ToString());
	OnStoryStepCompleted.Broadcast(CompletedStepId, ResultId);

	return AdvanceFromResult(CompletedStepId, ResultId);
}

bool UKumaStoryFlowSubsystem::AdvanceFromResult(FName FromStepId, FName ResultId)
{
	const FKumaStoryBranchRow* BranchRow = FindStoryBranch(FromStepId, ResultId);
	if (!BranchRow || BranchRow->ToStepId.IsNone())
	{
		UE_LOG(LogKumaStory, Log, TEXT("[KumaStory] Flow finished or no branch. From=%s Result=%s"), *FromStepId.ToString(), *ResultId.ToString());
		CurrentStepId = NAME_None;
		CurrentStepType = NAME_None;
		CurrentTargetId = NAME_None;
		OnStoryFlowFinished.Broadcast(FromStepId, ResultId);
		return false;
	}

	return ExecuteStep(BranchRow->ToStepId);
}

void UKumaStoryFlowSubsystem::CloseDialogueForNonDialogueStep(FName NextStepId, const FKumaStoryStepRow& StepRow)
{
	if (IsStepTypeAny(StepRow.StepType, TEXT("Dialogue"), TEXT("Dialog")))
	{
		return;
	}

	SetDialogueWidgetsVisible(false);
	OnDialogueLineStarted.Broadcast(NextStepId, FName(TEXT(" ")), FText::GetEmpty());
	OnDialogueTextUpdated.Broadcast(FText::GetEmpty());
	OnDialogueLineCompleted.Broadcast(NextStepId);
	OnDialogueClosed.Broadcast(NextStepId);
}

void UKumaStoryFlowSubsystem::SetDialogueWidgetsVisible(bool bVisible) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (!IsValid(Widget) || Widget->GetWorld() != World || !IsDialogueWidget(Widget))
		{
			continue;
		}

		Widget->SetVisibility(TargetVisibility);
	}
}

bool UKumaStoryFlowSubsystem::IsDialogueWidget(const UUserWidget* Widget) const
{
	if (!Widget || DialogueWidgetClassName.IsNone())
	{
		return false;
	}

	for (const UClass* WidgetClass = Widget->GetClass(); WidgetClass; WidgetClass = WidgetClass->GetSuperClass())
	{
		if (WidgetClass->GetFName() == DialogueWidgetClassName)
		{
			return true;
		}
	}

	return false;
}

bool UKumaStoryFlowSubsystem::ValidateStoryTables() const
{
	if (!StoryStepTable || !StoryBranchTable)
	{
		return false;
	}

	bool bIsValid = true;

	for (const TPair<FName, uint8*>& Pair : StoryBranchTable->GetRowMap())
	{
		const FKumaStoryBranchRow* BranchRow = reinterpret_cast<const FKumaStoryBranchRow*>(Pair.Value);
		if (!BranchRow)
		{
			continue;
		}

		if (!FindStoryStep(BranchRow->FromStepId))
		{
			UE_LOG(LogKumaStory, Error, TEXT("[KumaStory] Branch '%s' has invalid FromStepId '%s'."), *Pair.Key.ToString(), *BranchRow->FromStepId.ToString());
			bIsValid = false;
		}

		if (!BranchRow->ToStepId.IsNone() && !FindStoryStep(BranchRow->ToStepId))
		{
			UE_LOG(LogKumaStory, Error, TEXT("[KumaStory] Branch '%s' has invalid ToStepId '%s'."), *Pair.Key.ToString(), *BranchRow->ToStepId.ToString());
			bIsValid = false;
		}
	}

	return bIsValid;
}

bool UKumaStoryFlowSubsystem::IsCurrentStepType(const TCHAR* TypeName) const
{
	return IsStepType(CurrentStepType, TypeName);
}

bool UKumaStoryFlowSubsystem::IsStepType(FName StepType, const TCHAR* TypeName) const
{
	return StepType == FName(TypeName);
}

bool UKumaStoryFlowSubsystem::IsStepTypeAny(FName StepType, const TCHAR* FirstTypeName, const TCHAR* SecondTypeName) const
{
	return IsStepType(StepType, FirstTypeName) || IsStepType(StepType, SecondTypeName);
}

FName UKumaStoryFlowSubsystem::ResolveStepId(FName RowName, const FKumaStoryStepRow& StepRow) const
{
	return StepRow.StepId.IsNone() ? RowName : StepRow.StepId;
}

void UKumaStoryFlowSubsystem::HandleTypingTextUpdated(const FText& PartialText)
{
	OnDialogueTextUpdated.Broadcast(PartialText);
}

void UKumaStoryFlowSubsystem::HandleTypingCompleted()
{
	OnDialogueLineCompleted.Broadcast(CurrentStepId);
}
