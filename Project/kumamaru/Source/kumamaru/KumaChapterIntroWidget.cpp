// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaChapterIntroWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UKumaChapterIntroWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// The root must be ready before UUserWidget creates its Slate content.
	BuildLayout();
}

void UKumaChapterIntroWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetRenderOpacity(1.f);
}

void UKumaChapterIntroWidget::SetTitle(const FText& InTitle)
{
	if (ChapterTitleText) ChapterTitleText->SetText(InTitle);
}

void UKumaChapterIntroWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTimerHandle);
	}

	bIsPlaying = false;

	Super::NativeDestruct();
}

void UKumaChapterIntroWidget::PlayIntro(float InHoldSeconds, float InFadeSeconds)
{
	HoldSeconds = FMath::Max(0.f, InHoldSeconds);
	FadeSeconds = FMath::Max(0.f, InFadeSeconds);
	ElapsedSeconds = 0.f;
	bIsPlaying = true;
	SetRenderOpacity(1.f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FadeTimerHandle, this, &UKumaChapterIntroWidget::UpdateFade, 1.f / 60.f, true);
	}
}

void UKumaChapterIntroWidget::UpdateFade()
{
	if (!bIsPlaying)
	{
		return;
	}

	ElapsedSeconds += 1.f / 60.f;
	if (ElapsedSeconds <= HoldSeconds)
	{
		return;
	}

	const float SafeFadeSeconds = FMath::Max(FadeSeconds, KINDA_SMALL_NUMBER);
	const float FadeProgress = FMath::Clamp((ElapsedSeconds - HoldSeconds) / SafeFadeSeconds, 0.f, 1.f);
	SetRenderOpacity(1.f - FadeProgress);

	if (FadeProgress < 1.f)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTimerHandle);
	}

	bIsPlaying = false;
	RemoveFromParent();
	OnIntroFinished.Broadcast();
}

void UKumaChapterIntroWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ChapterIntroRoot"));
	WidgetTree->RootWidget = RootPanel;

	BlackoutBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlackoutBorder"));
	BlackoutBorder->SetBrush(FSlateColorBrush(FLinearColor::Black));
	UCanvasPanelSlot* BlackoutSlot = RootPanel->AddChildToCanvas(BlackoutBorder);
	BlackoutSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BlackoutSlot->SetOffsets(FMargin(0.f));

	ChapterTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChapterTitleText"));
	ChapterTitleText->SetText(FText::FromString(TEXT("CHAPTER 1\nMy Home")));
	ChapterTitleText->SetJustification(ETextJustify::Center);
	ChapterTitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	UCanvasPanelSlot* TitleSlot = RootPanel->AddChildToCanvas(ChapterTitleText);
	TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	TitleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	TitleSlot->SetAutoSize(true);
}
