#include "KumaMiniGame0Widget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/World.h"

void UKumaMiniGame0Widget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UKumaMiniGame0Widget::NativeDestruct()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(InstructionTimer);
	Super::NativeDestruct();
}

void UKumaMiniGame0Widget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget) return;
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MiniGame0Root"));
	WidgetTree->RootWidget = Root;
	CrosshairText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Crosshair"));
	CrosshairText->SetText(FText::FromString(TEXT("+")));
	FSlateFontInfo CrosshairFont = CrosshairText->GetFont(); CrosshairFont.Size = 42; CrosshairText->SetFont(CrosshairFont);
	CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	UCanvasPanelSlot* CrosshairSlot = Root->AddChildToCanvas(CrosshairText);
	CrosshairSlot->SetAnchors(FAnchors(0.5f, 0.5f)); CrosshairSlot->SetAlignment(FVector2D(0.5f, 0.5f)); CrosshairSlot->SetAutoSize(true);
	InstructionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Instruction"));
	InstructionText->SetText(FText::FromString(TEXT("화면을 드래그해 나비를 찾으세요")));
	UFont* SBAggroFont = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/SB_Aggro_M_Font.SB_Aggro_M_Font"));
	if (!SBAggroFont) UE_LOG(LogTemp, Warning, TEXT("[KumaMiniGame0] Could not load SB Aggro font. Using the engine default font."));
	InstructionText->SetFont(FSlateFontInfo(SBAggroFont, 60));
	InstructionText->SetJustification(ETextJustify::Center);
	InstructionText->SetColorAndOpacity(FSlateColor(FLinearColor::White)); InstructionText->SetShadowColorAndOpacity(FLinearColor::Black); InstructionText->SetShadowOffset(FVector2D(2.f, 2.f));
	UCanvasPanelSlot* InstructionSlot = Root->AddChildToCanvas(InstructionText);
	InstructionSlot->SetAnchors(FAnchors(0.5f, 0.5f)); InstructionSlot->SetAlignment(FVector2D(0.5f, 0.5f)); InstructionSlot->SetAutoSize(true);
}

void UKumaMiniGame0Widget::BeginMiniGame()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	InstructionElapsed = 0.f;
	if (CrosshairText) CrosshairText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (InstructionText) { InstructionText->SetVisibility(ESlateVisibility::HitTestInvisible); InstructionText->SetRenderOpacity(1.f); }
	if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(InstructionTimer, this, &UKumaMiniGame0Widget::UpdateInstructionFade, 1.f / 60.f, true);
}

void UKumaMiniGame0Widget::EndMiniGame()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(InstructionTimer);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UKumaMiniGame0Widget::UpdateInstructionFade()
{
	InstructionElapsed += 1.f / 60.f;
	if (!InstructionText) return;
	const float Alpha = InstructionElapsed <= 2.f ? 1.f : FMath::Clamp(1.f - (InstructionElapsed - 2.f) / 0.5f, 0.f, 1.f);
	InstructionText->SetRenderOpacity(Alpha);
	if (Alpha <= 0.f) { InstructionText->SetVisibility(ESlateVisibility::Collapsed); if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(InstructionTimer); }
}
