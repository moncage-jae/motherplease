// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaDialogueTypingComponent.h"

#include "TimerManager.h"

void UKumaDialogueTypingComponent::StartTyping(const FText& InFullText, float CharsPerSecond, FTimerManager& TimerManager)
{
	TimerManager.ClearTimer(TypingTimerHandle);

	SourceText = InFullText;
	SourceString = InFullText.ToString();
	TotalCharCount = SourceString.Len();
	RevealedCharCount = 0;
	ActiveTimerManager = &TimerManager;

	if (TotalCharCount <= 0)
	{
		OnCharacterRevealed.Broadcast(FText::GetEmpty());
		CompleteTyping();
		return;
	}

	if (CharsPerSecond <= 0.f)
	{
		RevealedCharCount = TotalCharCount;
		OnCharacterRevealed.Broadcast(SourceText);
		CompleteTyping();
		return;
	}

	State = EKumaDialogueTypingState::Typing;
	OnCharacterRevealed.Broadcast(FText::GetEmpty());

	const float Interval = 1.0f / CharsPerSecond;
	TimerManager.SetTimer(TypingTimerHandle, this, &UKumaDialogueTypingComponent::RevealNextCharacter, Interval, true);
}

void UKumaDialogueTypingComponent::SkipToEnd(FTimerManager& TimerManager)
{
	if (State != EKumaDialogueTypingState::Typing)
	{
		return;
	}

	TimerManager.ClearTimer(TypingTimerHandle);
	ActiveTimerManager = &TimerManager;
	RevealedCharCount = TotalCharCount;
	OnCharacterRevealed.Broadcast(SourceText);
	CompleteTyping();
}

void UKumaDialogueTypingComponent::CancelTyping(FTimerManager& TimerManager)
{
	TimerManager.ClearTimer(TypingTimerHandle);
	State = EKumaDialogueTypingState::Idle;
	ActiveTimerManager = nullptr;
}

void UKumaDialogueTypingComponent::RevealNextCharacter()
{
	++RevealedCharCount;

	if (RevealedCharCount >= TotalCharCount)
	{
		if (ActiveTimerManager)
		{
			ActiveTimerManager->ClearTimer(TypingTimerHandle);
		}

		OnCharacterRevealed.Broadcast(SourceText);
		CompleteTyping();
		return;
	}

	OnCharacterRevealed.Broadcast(FText::FromString(SourceString.Left(RevealedCharCount)));
}

void UKumaDialogueTypingComponent::CompleteTyping()
{
	State = EKumaDialogueTypingState::Completed;
	OnTypingCompleted.Broadcast();
}
