// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KumaDialogueBoxWidget.generated.h"

class UBorder;
class UTextBlock;
class UKumaDialogueSubsystem;

/** A self-contained subtitle box with speaker name, typewriter text, and automatic wrapping. */
UCLASS()
class KUMAMARU_API UKumaDialogueBoxWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void BindDialogueSubsystem();
	void UnbindDialogueSubsystem();

	UFUNCTION()
	void HandleDialogueLineStarted(FName LineId, FName SpeakerId, const FText& FullText);

	UFUNCTION()
	void HandleDialogueTextUpdated(const FText& PartialText);

	UFUNCTION()
	void HandleDialogueClosed(FName LastLineId);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DialogueBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpeakerText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DialogueText;

	TWeakObjectPtr<UKumaDialogueSubsystem> DialogueSubsystem;
};
