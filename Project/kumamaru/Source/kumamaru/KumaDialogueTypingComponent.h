// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KumaDialogueTypingComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FKumaOnDialogueCharacterRevealedNative, const FText& /*PartialText*/);
DECLARE_MULTICAST_DELEGATE(FKumaOnDialogueTypingCompletedNative);

enum class EKumaDialogueTypingState : uint8
{
	Idle,
	Typing,
	Completed
};

UCLASS()
class KUMAMARU_API UKumaDialogueTypingComponent : public UObject
{
	GENERATED_BODY()

public:
	void StartTyping(const FText& InFullText, float CharsPerSecond, FTimerManager& TimerManager);
	void SkipToEnd(FTimerManager& TimerManager);
	void CancelTyping(FTimerManager& TimerManager);

	bool IsTyping() const { return State == EKumaDialogueTypingState::Typing; }

	FKumaOnDialogueCharacterRevealedNative OnCharacterRevealed;
	FKumaOnDialogueTypingCompletedNative OnTypingCompleted;

private:
	void RevealNextCharacter();
	void CompleteTyping();

	FText SourceText;
	FString SourceString;
	int32 RevealedCharCount = 0;
	int32 TotalCharCount = 0;
	EKumaDialogueTypingState State = EKumaDialogueTypingState::Idle;
	FTimerHandle TypingTimerHandle;
	FTimerManager* ActiveTimerManager = nullptr;
};
