// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "KumaChapterIntroWidget.generated.h"

class UBorder;
class UAudioComponent;
class USoundBase;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FKumaChapterIntroFinished);

/** Full-screen Chapter 1 title card that fades its own UI away. */
UCLASS()
class KUMAMARU_API UKumaChapterIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sets the two independently styled lines shown on the chapter title card. */
	void SetChapterInfo(const FText& InChapterNumber, const FText& InChapterTitle);
	void PlayIntro(float InHoldSeconds, float InFadeSeconds);

	FKumaChapterIntroFinished OnIntroFinished;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void UpdateFade();
	void StopChapterIntroSound();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BlackoutBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChapterNumberText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChapterNameText;

	/** Plays once when the Chapter 1 or Chapter 2 title card appears. */
	UPROPERTY(EditDefaultsOnly, Category = "Kuma Chapter Intro|Audio")
	TSoftObjectPtr<USoundBase> ChapterIntroSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/chapter_intro.chapter_intro")));

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ChapterIntroAudioComponent;

	float HoldSeconds = 1.5f;
	float FadeSeconds = 1.5f;
	float ElapsedSeconds = 0.f;
	bool bIsPlaying = false;
	FTimerHandle FadeTimerHandle;
};
