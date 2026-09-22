// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaDialogueBoxWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "KumaDialogueSubsystem.h"

void UKumaDialogueBoxWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// UUserWidget builds its Slate content after initialization. The root must
	// exist here, rather than in NativeConstruct, for it to be rendered.
	BuildLayout();
}

void UKumaDialogueBoxWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindDialogueSubsystem();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UKumaDialogueBoxWidget::NativeDestruct()
{
	if (DialogueSubsystem.IsValid())
	{
		DialogueSubsystem->UnregisterDialogueWidget(this);
	}

	UnbindDialogueSubsystem();

	Super::NativeDestruct();
}

void UKumaDialogueBoxWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DialogueRoot"));
	WidgetTree->RootWidget = RootPanel;

	USizeBox* DialogueSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DialogueSizeBox"));
	DialogueSizeBox->SetWidthOverride(1300.f);
	DialogueSizeBox->SetMinDesiredHeight(156.f);

	DialogueBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueBox"));
	DialogueBox->SetBrush(FSlateColorBrush(FLinearColor(0.10f, 0.09f, 0.07f, 0.94f)));
	DialogueBox->SetPadding(FMargin(36.f, 20.f, 36.f, 24.f));
	DialogueSizeBox->SetContent(DialogueBox);

	UVerticalBox* TextContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueTextContainer"));
	DialogueBox->SetContent(TextContainer);

	SpeakerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeakerText"));
	FSlateFontInfo SpeakerFont = SpeakerText->GetFont();
	SpeakerFont.Size = 26;
	SpeakerText->SetFont(SpeakerFont);
	SpeakerText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.45f, 0.48f, 1.f)));
	SpeakerText->SetAutoWrapText(false);
	TextContainer->AddChildToVerticalBox(SpeakerText);

	DialogueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DialogueText"));
	FSlateFontInfo DialogueFont = DialogueText->GetFont();
	DialogueFont.Size = 30;
	DialogueText->SetFont(DialogueFont);
	DialogueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DialogueText->SetAutoWrapText(true);
	DialogueText->SetWrapTextAt(1160.f);
	DialogueText->SetJustification(ETextJustify::Left);
	UVerticalBoxSlot* DialogueTextSlot = TextContainer->AddChildToVerticalBox(DialogueText);
	DialogueTextSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	UCanvasPanelSlot* DialogueSlot = RootPanel->AddChildToCanvas(DialogueSizeBox);
	DialogueSlot->SetAnchors(FAnchors(0.5f, 1.f));
	DialogueSlot->SetAlignment(FVector2D(0.5f, 1.f));
	DialogueSlot->SetPosition(FVector2D(0.f, -52.f));
	DialogueSlot->SetAutoSize(true);
}

void UKumaDialogueBoxWidget::BindDialogueSubsystem()
{
	UGameInstance* GameInstance = GetGameInstance();
	DialogueSubsystem = GameInstance ? GameInstance->GetSubsystem<UKumaDialogueSubsystem>() : nullptr;
	if (!DialogueSubsystem.IsValid())
	{
		return;
	}

	DialogueSubsystem->OnDialogueLineStarted.AddDynamic(this, &UKumaDialogueBoxWidget::HandleDialogueLineStarted);
	DialogueSubsystem->OnDialogueTextUpdated.AddDynamic(this, &UKumaDialogueBoxWidget::HandleDialogueTextUpdated);
	DialogueSubsystem->OnDialogueClosed.AddDynamic(this, &UKumaDialogueBoxWidget::HandleDialogueClosed);
}

void UKumaDialogueBoxWidget::UnbindDialogueSubsystem()
{
	if (!DialogueSubsystem.IsValid())
	{
		return;
	}

	DialogueSubsystem->OnDialogueLineStarted.RemoveDynamic(this, &UKumaDialogueBoxWidget::HandleDialogueLineStarted);
	DialogueSubsystem->OnDialogueTextUpdated.RemoveDynamic(this, &UKumaDialogueBoxWidget::HandleDialogueTextUpdated);
	DialogueSubsystem->OnDialogueClosed.RemoveDynamic(this, &UKumaDialogueBoxWidget::HandleDialogueClosed);
	DialogueSubsystem.Reset();
}

void UKumaDialogueBoxWidget::HandleDialogueLineStarted(FName LineId, FName SpeakerId, const FText& FullText)
{
	if (SpeakerText)
	{
		SpeakerText->SetText(FText::FromName(SpeakerId));
	}

	if (DialogueText)
	{
		DialogueText->SetText(FText::GetEmpty());
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKumaDialogueBoxWidget::HandleDialogueTextUpdated(const FText& PartialText)
{
	if (DialogueText)
	{
		DialogueText->SetText(PartialText);
	}
}

void UKumaDialogueBoxWidget::HandleDialogueClosed(FName LastLineId)
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (SpeakerText)
	{
		SpeakerText->SetText(FText::GetEmpty());
	}

	if (DialogueText)
	{
		DialogueText->SetText(FText::GetEmpty());
	}
}
