// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaDialogueSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "KumaDialogueTypingComponent.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogKumaDialogue, Log, All);

UKumaDialogueSubsystem::UKumaDialogueSubsystem()
{
}

void UKumaDialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TypingComponent = NewObject<UKumaDialogueTypingComponent>(this);
	if (TypingComponent)
	{
		TypingComponent->OnCharacterRevealed.AddUObject(this, &UKumaDialogueSubsystem::HandleTypingTextUpdated);
		TypingComponent->OnTypingCompleted.AddUObject(this, &UKumaDialogueSubsystem::HandleTypingCompleted);
	}
}

void UKumaDialogueSubsystem::Deinitialize()
{
	ClearAutoAdvanceTimer();
	CancelTyping();

	if (TypingComponent)
	{
		TypingComponent->OnCharacterRevealed.RemoveAll(this);
		TypingComponent->OnTypingCompleted.RemoveAll(this);
	}

	ActiveLines.Reset();
	ActiveOwnerId = NAME_None;
	LastCompletedLineId = NAME_None;
	ActiveLineIndex = INDEX_NONE;
	bDialogueActive = false;
	bDialogueVisible = false;
	bLineTextCompleted = false;

	Super::Deinitialize();
}

bool UKumaDialogueSubsystem::PlayDialogueLine(FName OwnerId, FName LineId, FName SpeakerId, const FText& Text, FKumaDialogueOptions Options)
{
	FKumaDialogueLine DialogueLine;
	DialogueLine.LineId = LineId;
	DialogueLine.SpeakerId = SpeakerId;
	DialogueLine.Text = Text;
	DialogueLine.Options = Options;

	TArray<FKumaDialogueLine> DialogueLines;
	DialogueLines.Add(DialogueLine);
	return PlayDialogueSequence(OwnerId, DialogueLines);
}

bool UKumaDialogueSubsystem::PlayDialogueSequence(FName OwnerId, const TArray<FKumaDialogueLine>& DialogueLines)
{
	if (DialogueLines.Num() <= 0)
	{
		UE_LOG(LogKumaDialogue, Warning, TEXT("[KumaDialogue] Play ignored because there are no dialogue lines. Owner=%s"), *OwnerId.ToString());
		return false;
	}

	if (bDialogueActive)
	{
		CloseDialogue();
	}
	else
	{
		ClearAutoAdvanceTimer();
		CancelTyping();
	}

	ActiveOwnerId = OwnerId;
	ActiveLines.Reset();
	ActiveLines.Reserve(DialogueLines.Num());
	for (const FKumaDialogueLine& DialogueLine : DialogueLines)
	{
		ActiveLines.Add(NormalizeDialogueLine(DialogueLine));
	}
	ActiveLineIndex = INDEX_NONE;
	bDialogueActive = true;
	bLineTextCompleted = false;
	bDialogueVisible = true;
	SetDialogueWidgetsVisible(true);

	return StartLine(0);
}

bool UKumaDialogueSubsystem::AdvanceDialogue()
{
	if (!bDialogueActive)
	{
		return false;
	}

	ClearAutoAdvanceTimer();

	if (TypingComponent && TypingComponent->IsTyping())
	{
		const FKumaDialogueLine* ActiveLine = GetActiveLine();
		if (!ActiveLine || !ActiveLine->Options.bAllowSkipTyping)
		{
			return false;
		}

		if (UWorld* World = GetWorld())
		{
			TypingComponent->SkipToEnd(World->GetTimerManager());
			return true;
		}

		return false;
	}

	if (!bLineTextCompleted)
	{
		return false;
	}

	return AdvanceAfterLineCompleted();
}

bool UKumaDialogueSubsystem::CloseDialogue()
{
	ClearAutoAdvanceTimer();
	CancelTyping();

	const FName LastLineId = GetActiveLine() ? GetActiveLine()->LineId : LastCompletedLineId;
	ActiveLines.Reset();
	ActiveOwnerId = NAME_None;
	ActiveLineIndex = INDEX_NONE;
	bDialogueActive = false;
	bLineTextCompleted = false;

	if (bDialogueVisible)
	{
		bDialogueVisible = false;
		SetDialogueWidgetsVisible(false);
		OnDialogueTextUpdated.Broadcast(FText::GetEmpty());
		OnDialogueClosed.Broadcast(LastLineId);
	}

	return true;
}

bool UKumaDialogueSubsystem::IsDialogueTyping() const
{
	return TypingComponent && TypingComponent->IsTyping();
}

FName UKumaDialogueSubsystem::GetActiveLineId() const
{
	if (const FKumaDialogueLine* ActiveLine = GetActiveLine())
	{
		return ActiveLine->LineId;
	}

	return NAME_None;
}

FKumaDialogueOptions UKumaDialogueSubsystem::GetActiveOptions() const
{
	if (const FKumaDialogueLine* ActiveLine = GetActiveLine())
	{
		return ActiveLine->Options;
	}

	return FKumaDialogueOptions();
}

bool UKumaDialogueSubsystem::StartLine(int32 LineIndex)
{
	if (!ActiveLines.IsValidIndex(LineIndex))
	{
		FinishDialogueSequence(true);
		return false;
	}

	ClearAutoAdvanceTimer();
	CancelTyping();

	ActiveLineIndex = LineIndex;
	bLineTextCompleted = false;
	bDialogueVisible = true;
	SetDialogueWidgetsVisible(true);

	const FKumaDialogueLine& DialogueLine = ActiveLines[ActiveLineIndex];
	LastCompletedLineId = DialogueLine.LineId;
	OnDialogueLineStarted.Broadcast(DialogueLine.LineId, DialogueLine.SpeakerId, DialogueLine.Text);

	if (!TypingComponent)
	{
		OnDialogueTextUpdated.Broadcast(DialogueLine.Text);
		HandleTypingCompleted();
		return true;
	}

	if (UWorld* World = GetWorld())
	{
		TypingComponent->StartTyping(DialogueLine.Text, DialogueLine.Options.TypingSpeed, World->GetTimerManager());
		return true;
	}

	OnDialogueTextUpdated.Broadcast(DialogueLine.Text);
	HandleTypingCompleted();
	return true;
}

bool UKumaDialogueSubsystem::AdvanceAfterLineCompleted()
{
	if (!bDialogueActive || !bLineTextCompleted)
	{
		return false;
	}

	const FKumaDialogueLine* ActiveLine = GetActiveLine();
	if (!ActiveLine)
	{
		FinishDialogueSequence(true);
		return false;
	}

	if (ActiveLine->Options.AdvancePolicy == EKumaDialogueAdvancePolicy::AutoClose)
	{
		FinishDialogueSequence(true);
		return true;
	}

	const int32 NextLineIndex = ActiveLineIndex + 1;
	if (ActiveLines.IsValidIndex(NextLineIndex))
	{
		return StartLine(NextLineIndex);
	}

	FinishDialogueSequence(ActiveLine->Options.bCloseWhenFinished);
	return true;
}

void UKumaDialogueSubsystem::FinishDialogueSequence(bool bCloseWidget)
{
	ClearAutoAdvanceTimer();
	CancelTyping();

	const FName FinishedOwnerId = ActiveOwnerId;
	const FName LastLineId = GetActiveLine() ? GetActiveLine()->LineId : LastCompletedLineId;
	LastCompletedLineId = LastLineId;

	ActiveLines.Reset();
	ActiveOwnerId = NAME_None;
	ActiveLineIndex = INDEX_NONE;
	bDialogueActive = false;
	bLineTextCompleted = false;

	if (bCloseWidget && bDialogueVisible)
	{
		bDialogueVisible = false;
		SetDialogueWidgetsVisible(false);
		OnDialogueTextUpdated.Broadcast(FText::GetEmpty());
		OnDialogueClosed.Broadcast(LastLineId);
	}

	OnDialogueSequenceFinished.Broadcast(FinishedOwnerId, LastLineId);
}

void UKumaDialogueSubsystem::ClearAutoAdvanceTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	}
}

void UKumaDialogueSubsystem::CancelTyping()
{
	if (TypingComponent)
	{
		if (UWorld* World = GetWorld())
		{
			TypingComponent->CancelTyping(World->GetTimerManager());
		}
	}
}

void UKumaDialogueSubsystem::ScheduleAutoAdvance(float DelaySeconds)
{
	if (DelaySeconds <= 0.f)
	{
		HandleAutoAdvanceTimer();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AutoAdvanceTimerHandle, this, &UKumaDialogueSubsystem::HandleAutoAdvanceTimer, DelaySeconds, false);
		return;
	}

	HandleAutoAdvanceTimer();
}

void UKumaDialogueSubsystem::HandleAutoAdvanceTimer()
{
	AdvanceAfterLineCompleted();
}

void UKumaDialogueSubsystem::HandleTypingTextUpdated(const FText& PartialText)
{
	OnDialogueTextUpdated.Broadcast(PartialText);
}

void UKumaDialogueSubsystem::HandleTypingCompleted()
{
	if (!bDialogueActive || bLineTextCompleted)
	{
		return;
	}

	const FKumaDialogueLine* ActiveLine = GetActiveLine();
	if (!ActiveLine)
	{
		return;
	}

	bLineTextCompleted = true;
	LastCompletedLineId = ActiveLine->LineId;
	OnDialogueLineCompleted.Broadcast(ActiveLine->LineId);

	if (ActiveLine->Options.AdvancePolicy == EKumaDialogueAdvancePolicy::AutoNext)
	{
		ScheduleAutoAdvance(ActiveLine->Options.AutoAdvanceDelay);
	}
	else if (ActiveLine->Options.AdvancePolicy == EKumaDialogueAdvancePolicy::AutoClose)
	{
		ScheduleAutoAdvance(ActiveLine->Options.AutoCloseDelay);
	}
}

void UKumaDialogueSubsystem::SetDialogueWidgetsVisible(bool bVisible) const
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

bool UKumaDialogueSubsystem::IsDialogueWidget(const UUserWidget* Widget) const
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

const FKumaDialogueLine* UKumaDialogueSubsystem::GetActiveLine() const
{
	if (ActiveLines.IsValidIndex(ActiveLineIndex))
	{
		return &ActiveLines[ActiveLineIndex];
	}

	return nullptr;
}

FKumaDialogueLine UKumaDialogueSubsystem::NormalizeDialogueLine(const FKumaDialogueLine& DialogueLine) const
{
	FKumaDialogueLine NormalizedLine = DialogueLine;
	NormalizedLine.Text = NormalizeDialogueText(DialogueLine.Text);
	return NormalizedLine;
}

FText UKumaDialogueSubsystem::NormalizeDialogueText(const FText& Text) const
{
	FString DisplayString = Text.ToString();
	if (!DisplayString.Contains(TEXT("\\n")) && !DisplayString.Contains(TEXT("\\r")))
	{
		return Text;
	}

	DisplayString.ReplaceInline(TEXT("\\r\\n"), TEXT("\n"), ESearchCase::CaseSensitive);
	DisplayString.ReplaceInline(TEXT("\\n"), TEXT("\n"), ESearchCase::CaseSensitive);
	DisplayString.ReplaceInline(TEXT("\\r"), TEXT("\n"), ESearchCase::CaseSensitive);

	return FText::FromString(DisplayString);
}
