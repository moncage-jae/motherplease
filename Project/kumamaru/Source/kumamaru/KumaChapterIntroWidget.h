// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "KumaChapterIntroWidget.generated.h"

class UBorder;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FKumaChapterIntroFinished);

/** Full-screen Chapter 1 title card that fades its own UI away. */
UCLASS()
class KUMAMARU_API UKumaChapterIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTitle(const FText& InTitle);
	void PlayIntro(float InHoldSeconds, float InFadeSeconds);

	FKumaChapterIntroFinished OnIntroFinished;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void UpdateFade();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BlackoutBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChapterTitleText;

	float HoldSeconds = 1.5f;
	float FadeSeconds = 1.5f;
	float ElapsedSeconds = 0.f;
	bool bIsPlaying = false;
	FTimerHandle FadeTimerHandle;
};
