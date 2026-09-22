#include "KumaChapterEndWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
void UKumaChapterEndWidget::NativeOnInitialized() { Super::NativeOnInitialized(); BuildLayout(); }
void UKumaChapterEndWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget) return;
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ChapterEndRoot")); WidgetTree->RootWidget = Root;
	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChapterEndMenu"));
	UCanvasPanelSlot* MenuSlot = Root->AddChildToCanvas(Menu); MenuSlot->SetAnchors(FAnchors(.5f,.5f)); MenuSlot->SetAlignment(FVector2D(.5f,.5f)); MenuSlot->SetAutoSize(true);
	auto AddButton = [this, Menu](const TCHAR* Name, const TCHAR* Label, TObjectPtr<UButton>& OutButton) { OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name); UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("%sText"), Name))); Text->SetText(FText::FromString(Label)); FSlateFontInfo Font=Text->GetFont(); Font.Size=28; Text->SetFont(Font); OutButton->AddChild(Text); UVerticalBoxSlot* Slot=Menu->AddChildToVerticalBox(OutButton); Slot->SetPadding(FMargin(0,8)); };
	AddButton(TEXT("NextChapterButton"), TEXT("next chapter"), NextButton); AddButton(TEXT("MainMenuButton"), TEXT("main menu"), MainMenuButton); AddButton(TEXT("QuitButton"), TEXT("exit"), QuitButton);
	NextButton->OnClicked.AddDynamic(this,&UKumaChapterEndWidget::HandleNext); MainMenuButton->OnClicked.AddDynamic(this,&UKumaChapterEndWidget::HandleMainMenu); QuitButton->OnClicked.AddDynamic(this,&UKumaChapterEndWidget::HandleQuit);
}
void UKumaChapterEndWidget::HandleNext(){ OnNextChapterClicked.Broadcast(); }
void UKumaChapterEndWidget::HandleMainMenu(){ OnMainMenuClicked.Broadcast(); }
void UKumaChapterEndWidget::HandleQuit(){ OnQuitClicked.Broadcast(); }

void UKumaChapterEndWidget::SetNextChapterVisible(bool bVisible)
{
	if (NextButton)
	{
		NextButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
