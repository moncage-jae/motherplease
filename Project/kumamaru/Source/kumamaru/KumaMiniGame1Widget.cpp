#include "KumaMiniGame1Widget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace KumaMiniGame1UI
{
	constexpr float InstructionHoldSeconds = 2.f;
	constexpr float InstructionFadeSeconds = 0.5f;
	constexpr float FailureFadeSeconds = 0.75f;
	constexpr float UpdateRate = 1.f / 60.f;
}

void UKumaMiniGame1Widget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UKumaMiniGame1Widget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InstructionTimer);
		World->GetTimerManager().ClearTimer(FailureFadeTimer);
	}
	Super::NativeDestruct();
}

void UKumaMiniGame1Widget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MiniGame1Root"));
	WidgetTree->RootWidget = Root;

	FailureFade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FailureFade"));
	FailureFade->SetBrushColor(FLinearColor::Black);
	FailureFade->SetRenderOpacity(0.f);
	FailureFade->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* FadeSlot = Root->AddChildToCanvas(FailureFade);
	FadeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	FadeSlot->SetOffsets(FMargin(0.f));

	CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Countdown"));
	CountdownText->SetText(FText::FromString(TEXT("01:00")));
	FSlateFontInfo CountdownFont = CountdownText->GetFont();
	CountdownFont.Size = 34;
	CountdownText->SetFont(CountdownFont);
	CountdownText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.73f, 0.73f, 1.f)));
	CountdownText->SetShadowColorAndOpacity(FLinearColor::Black);
	CountdownText->SetShadowOffset(FVector2D(2.f, 2.f));
	CountdownText->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CountdownSlot = Root->AddChildToCanvas(CountdownText);
	CountdownSlot->SetAnchors(FAnchors(0.5f, 0.07f));
	CountdownSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CountdownSlot->SetAutoSize(true);

	InstructionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Instruction"));
	FSlateFontInfo InstructionFont = InstructionText->GetFont();
	InstructionFont.Size = 28;
	InstructionText->SetFont(InstructionFont);
	InstructionText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	InstructionText->SetShadowColorAndOpacity(FLinearColor::Black);
	InstructionText->SetShadowOffset(FVector2D(2.f, 2.f));
	InstructionText->SetJustification(ETextJustify::Center);
	InstructionText->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* InstructionSlot = Root->AddChildToCanvas(InstructionText);
	InstructionSlot->SetAnchors(FAnchors(0.5f, 0.78f));
	InstructionSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	InstructionSlot->SetAutoSize(true);
}

void UKumaMiniGame1Widget::BeginMiniGame()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetCountdownVisible(false);
	HideInstruction();
	if (FailureFade)
	{
		FailureFade->SetRenderOpacity(0.f);
		FailureFade->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UKumaMiniGame1Widget::EndMiniGame()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InstructionTimer);
		World->GetTimerManager().ClearTimer(FailureFadeTimer);
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UKumaMiniGame1Widget::ShowInstruction(const FText& Instruction)
{
	if (!InstructionText)
	{
		return;
	}

	InstructionText->SetText(Instruction);
	InstructionText->SetRenderOpacity(1.f);
	InstructionText->SetVisibility(ESlateVisibility::HitTestInvisible);
	InstructionElapsed = 0.f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InstructionTimer);
		World->GetTimerManager().SetTimer(InstructionTimer, this, &UKumaMiniGame1Widget::UpdateInstructionFade, KumaMiniGame1UI::UpdateRate, true);
	}
}

void UKumaMiniGame1Widget::HideInstruction()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InstructionTimer);
	}
	if (InstructionText)
	{
		InstructionText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UKumaMiniGame1Widget::SetCountdownVisible(bool bVisible)
{
	if (CountdownText)
	{
		CountdownText->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UKumaMiniGame1Widget::SetCountdownSeconds(float RemainingSeconds)
{
	if (!CountdownText)
	{
		return;
	}

	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	CountdownText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60)));
}

void UKumaMiniGame1Widget::BeginFailureFade()
{
	HideInstruction();
	SetCountdownVisible(false);
	if (!FailureFade)
	{
		return;
	}

	FailureFadeElapsed = 0.f;
	FailureFade->SetRenderOpacity(0.f);
	FailureFade->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FailureFadeTimer);
		World->GetTimerManager().SetTimer(FailureFadeTimer, this, &UKumaMiniGame1Widget::UpdateFailureFade, KumaMiniGame1UI::UpdateRate, true);
	}
}

void UKumaMiniGame1Widget::UpdateInstructionFade()
{
	InstructionElapsed += KumaMiniGame1UI::UpdateRate;
	if (!InstructionText)
	{
		return;
	}

	const float Alpha = InstructionElapsed <= KumaMiniGame1UI::InstructionHoldSeconds
		? 1.f
		: FMath::Clamp(1.f - (InstructionElapsed - KumaMiniGame1UI::InstructionHoldSeconds) / KumaMiniGame1UI::InstructionFadeSeconds, 0.f, 1.f);
	InstructionText->SetRenderOpacity(Alpha);
	if (Alpha <= 0.f)
	{
		HideInstruction();
		OnInstructionFinished.Broadcast();
	}
}

void UKumaMiniGame1Widget::UpdateFailureFade()
{
	FailureFadeElapsed += KumaMiniGame1UI::UpdateRate;
	if (!FailureFade)
	{
		return;
	}

	FailureFade->SetRenderOpacity(FMath::Clamp(FailureFadeElapsed / KumaMiniGame1UI::FailureFadeSeconds, 0.f, 1.f));
	if (FailureFadeElapsed >= KumaMiniGame1UI::FailureFadeSeconds)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FailureFadeTimer);
		}
	}
}
