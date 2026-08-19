// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaStartScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
template <typename WidgetType>
WidgetType* FindNamedWidget(UWidgetTree* WidgetTree, const FString& WidgetName)
{
	return WidgetTree ? Cast<WidgetType>(WidgetTree->FindWidget(FName(*WidgetName))) : nullptr;
}

FString BuildChapterWidgetName(int32 ChapterIndex, const TCHAR* Suffix)
{
	return FString::Printf(TEXT("Chapter%d%s"), ChapterIndex + 1, Suffix);
}
}

FKumaStartChapterCardData::FKumaStartChapterCardData()
	: ChapterNumber(1)
	, ChapterNumberText(FText::GetEmpty())
	, ChapterName(NSLOCTEXT("KumaStartScreen", "DefaultChapterName", "Chapter"))
	, bCleared(false)
	, ChapterLevelName(NAME_None)
{
}

UKumaStartScreenWidget::UKumaStartScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GameplayLevelName(TEXT("ThirdPersonMap"))
	, bOpenChapterLevelOnCardClick(false)
	, bAutoplayBackgroundMedia(true)
	, LogoSpriteFrameInterval(0.12f)
	, LogoPulseSpeed(3.0f)
	, LogoPulseScaleAmount(0.025f)
	, DefaultSoundVolume(100.0f)
	, DefaultBrightness(100.0f)
	, MaxBrightnessDimAlpha(0.7f)
	, DefaultLanguage(TEXT("한국어"))
	, ChapterNumberPrefixText(FText::FromString(TEXT("CHAPTER")))
	, ActivePopup(EKumaStartScreenPopup::None)
	, SelectedChapterIndex(INDEX_NONE)
	, CurrentLogoFrameIndex(0)
	, LogoFrameAccumulator(0.0f)
	, LogoPulseTime(0.0f)
	, SoundValue(100.0f)
	, BrightnessValue(100.0f)
	, CurrentLanguage(TEXT("한국어"))
{
	LanguageOptions.Add(TEXT("english"));
	LanguageOptions.Add(TEXT("한국어"));

	for (int32 Index = 0; Index < 5; ++Index)
	{
		FKumaStartChapterCardData CardData;
		CardData.ChapterNumber = Index + 1;
		CardData.ChapterName = FText::FromString(FString::Printf(TEXT("Chapter %02d"), Index + 1));
		CardData.bCleared = Index < 2;
		ChapterCards.Add(CardData);
	}
}

void UKumaStartScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LanguageOptions.IsEmpty())
	{
		LanguageOptions.Add(TEXT("english"));
		LanguageOptions.Add(TEXT("한국어"));
	}

	CurrentLanguage = LanguageOptions.Contains(DefaultLanguage) ? DefaultLanguage : LanguageOptions[0];
	SoundValue = FMath::Clamp(DefaultSoundVolume, 0.0f, 100.0f);
	BrightnessValue = FMath::Clamp(DefaultBrightness, 0.0f, 100.0f);
	SelectedChapterIndex = INDEX_NONE;

	CacheChapterCardWidgets();
	BindWidgetEvents();
	ApplyConfiguredBrushes();
	RefreshOptionControls();
	RefreshChapterCards();
	ClosePopup();

	SetWidgetVisibility(MainMenuPanel, ESlateVisibility::Visible);
	SetText(PlayTimeText, FText::FromString(TEXT("00:00:00")));
	ApplySoundVolume(SoundValue / 100.0f);
	ApplyBrightness(BrightnessValue / 100.0f);
	StartBackgroundMedia();

	if (!MainStartButton || !StartPopupPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KumaStartScreen] Required UMG widgets are not bound. Check widget names in the Widget Blueprint."));
	}
}

void UKumaStartScreenWidget::NativeDestruct()
{
	StopBackgroundMedia();
	Super::NativeDestruct();
}

void UKumaStartScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	LogoPulseTime += InDeltaTime;
	const float PulseScale = 1.0f + FMath::Sin(LogoPulseTime * LogoPulseSpeed) * LogoPulseScaleAmount;

	if (MainLogoImage)
	{
		MainLogoImage->SetRenderScale(FVector2D(PulseScale, PulseScale));
	}

	if (StartPopupLogoImage)
	{
		StartPopupLogoImage->SetRenderScale(FVector2D(PulseScale, PulseScale));
	}

	if (ExitPopupLogoImage)
	{
		ExitPopupLogoImage->SetRenderScale(FVector2D(PulseScale, PulseScale));
	}

	if (LogoSpriteFrames.IsEmpty())
	{
		return;
	}

	LogoFrameAccumulator += InDeltaTime;
	const float FrameInterval = FMath::Max(LogoSpriteFrameInterval, KINDA_SMALL_NUMBER);
	if (LogoFrameAccumulator < FrameInterval)
	{
		return;
	}

	LogoFrameAccumulator = 0.0f;
	CurrentLogoFrameIndex = (CurrentLogoFrameIndex + 1) % LogoSpriteFrames.Num();

	const FSlateBrush& CurrentBrush = LogoSpriteFrames[CurrentLogoFrameIndex];
	if (HasBrushResource(CurrentBrush))
	{
		ApplyLogoBrushToBoundImages(CurrentBrush);
	}
}

void UKumaStartScreenWidget::ShowStartPopup()
{
	ShowPopupPanel(EKumaStartScreenPopup::Start, StartPopupPanel);
}

void UKumaStartScreenWidget::ShowChapterPopup()
{
	ShowPopupPanel(EKumaStartScreenPopup::Chapter, ChapterPopupPanel);
}

void UKumaStartScreenWidget::ShowOptionPopup()
{
	ShowPopupPanel(EKumaStartScreenPopup::Option, OptionPopupPanel);
	RefreshOptionControls();
}

void UKumaStartScreenWidget::ShowExitPopup()
{
	ShowPopupPanel(EKumaStartScreenPopup::Exit, ExitPopupPanel);
}

void UKumaStartScreenWidget::ClosePopup()
{
	ActivePopup = EKumaStartScreenPopup::None;

	SetWidgetVisibility(PopupDimPanel, ESlateVisibility::Collapsed);
	SetWidgetVisibility(StartPopupPanel, ESlateVisibility::Collapsed);
	SetWidgetVisibility(ChapterPopupPanel, ESlateVisibility::Collapsed);
	SetWidgetVisibility(OptionPopupPanel, ESlateVisibility::Collapsed);
	SetWidgetVisibility(SaveResetWarningPopupPanel, ESlateVisibility::Collapsed);
	SetWidgetVisibility(ExitPopupPanel, ESlateVisibility::Collapsed);
}

void UKumaStartScreenWidget::SelectChapterCard(int32 ChapterIndex)
{
	if (!BoundChapterCards.IsValidIndex(ChapterIndex))
	{
		return;
	}

	SelectedChapterIndex = ChapterIndex;
	OnKumaStartScreenChapterSelected(ChapterIndex);

	for (int32 Index = 0; Index < BoundChapterCards.Num(); ++Index)
	{
		SetWidgetVisibility(
			BoundChapterCards[Index].SelectImage,
			Index == SelectedChapterIndex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

FString UKumaStartScreenWidget::GetCurrentLanguage() const
{
	return CurrentLanguage;
}

void UKumaStartScreenWidget::BindWidgetEvents()
{
	if (MainStartButton)
	{
		MainStartButton->OnClicked.RemoveAll(this);
		MainStartButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleMainStartClicked);
	}

	if (MainChapterButton)
	{
		MainChapterButton->OnClicked.RemoveAll(this);
		MainChapterButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleMainChapterClicked);
	}

	if (MainOptionButton)
	{
		MainOptionButton->OnClicked.RemoveAll(this);
		MainOptionButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleMainOptionClicked);
	}

	if (MainExitButton)
	{
		MainExitButton->OnClicked.RemoveAll(this);
		MainExitButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleMainExitClicked);
	}

	if (StartConfirmButton)
	{
		StartConfirmButton->OnClicked.RemoveAll(this);
		StartConfirmButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleConfirmStartClicked);
	}

	if (StartCancelButton)
	{
		StartCancelButton->OnClicked.RemoveAll(this);
		StartCancelButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleCancelClicked);
	}

	if (ChapterCloseButton)
	{
		ChapterCloseButton->OnClicked.RemoveAll(this);
		ChapterCloseButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleCancelClicked);
	}

	if (OptionCloseButton)
	{
		OptionCloseButton->OnClicked.RemoveAll(this);
		OptionCloseButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleCancelClicked);
	}

	if (OptionResetButton)
	{
		OptionResetButton->OnClicked.RemoveAll(this);
		OptionResetButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleOptionResetClicked);
	}

	if (SaveResetRequestButton)
	{
		SaveResetRequestButton->OnClicked.RemoveAll(this);
		SaveResetRequestButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleSaveResetRequestedClicked);
	}

	if (SaveResetConfirmButton)
	{
		SaveResetConfirmButton->OnClicked.RemoveAll(this);
		SaveResetConfirmButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleSaveResetConfirmClicked);
	}

	if (SaveResetCancelButton)
	{
		SaveResetCancelButton->OnClicked.RemoveAll(this);
		SaveResetCancelButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleSaveResetCancelClicked);
	}

	if (ExitConfirmButton)
	{
		ExitConfirmButton->OnClicked.RemoveAll(this);
		ExitConfirmButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleConfirmExitClicked);
	}

	if (ExitCancelButton)
	{
		ExitCancelButton->OnClicked.RemoveAll(this);
		ExitCancelButton->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleCancelClicked);
	}

	if (SoundSlider)
	{
		SoundSlider->OnValueChanged.RemoveAll(this);
		SoundSlider->OnValueChanged.AddDynamic(this, &UKumaStartScreenWidget::HandleSoundSliderChanged);
	}

	if (BrightnessSlider)
	{
		BrightnessSlider->OnValueChanged.RemoveAll(this);
		BrightnessSlider->OnValueChanged.AddDynamic(this, &UKumaStartScreenWidget::HandleBrightnessSliderChanged);
	}

	if (LanguageComboBox)
	{
		LanguageComboBox->OnSelectionChanged.RemoveAll(this);
		LanguageComboBox->ClearOptions();
		for (const FString& LanguageOption : LanguageOptions)
		{
			if (!LanguageOption.IsEmpty())
			{
				LanguageComboBox->AddOption(LanguageOption);
			}
		}
		LanguageComboBox->OnSelectionChanged.AddDynamic(this, &UKumaStartScreenWidget::HandleLanguageChanged);
	}

	if (BoundChapterCards.Num() > 0 && BoundChapterCards[0].Button)
	{
		BoundChapterCards[0].Button->OnHovered.RemoveAll(this);
		BoundChapterCards[0].Button->OnClicked.RemoveAll(this);
		BoundChapterCards[0].Button->OnHovered.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard1Hovered);
		BoundChapterCards[0].Button->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard1Clicked);
	}

	if (BoundChapterCards.Num() > 1 && BoundChapterCards[1].Button)
	{
		BoundChapterCards[1].Button->OnHovered.RemoveAll(this);
		BoundChapterCards[1].Button->OnClicked.RemoveAll(this);
		BoundChapterCards[1].Button->OnHovered.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard2Hovered);
		BoundChapterCards[1].Button->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard2Clicked);
	}

	if (BoundChapterCards.Num() > 2 && BoundChapterCards[2].Button)
	{
		BoundChapterCards[2].Button->OnHovered.RemoveAll(this);
		BoundChapterCards[2].Button->OnClicked.RemoveAll(this);
		BoundChapterCards[2].Button->OnHovered.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard3Hovered);
		BoundChapterCards[2].Button->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard3Clicked);
	}

	if (BoundChapterCards.Num() > 3 && BoundChapterCards[3].Button)
	{
		BoundChapterCards[3].Button->OnHovered.RemoveAll(this);
		BoundChapterCards[3].Button->OnClicked.RemoveAll(this);
		BoundChapterCards[3].Button->OnHovered.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard4Hovered);
		BoundChapterCards[3].Button->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard4Clicked);
	}

	if (BoundChapterCards.Num() > 4 && BoundChapterCards[4].Button)
	{
		BoundChapterCards[4].Button->OnHovered.RemoveAll(this);
		BoundChapterCards[4].Button->OnClicked.RemoveAll(this);
		BoundChapterCards[4].Button->OnHovered.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard5Hovered);
		BoundChapterCards[4].Button->OnClicked.AddDynamic(this, &UKumaStartScreenWidget::HandleChapterCard5Clicked);
	}
}

void UKumaStartScreenWidget::CacheChapterCardWidgets()
{
	BoundChapterCards.Empty();

	const int32 CardCount = FMath::Min(ChapterCards.Num(), 5);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		FKumaStartBoundChapterCardWidgets CardWidgets;
		CardWidgets.Button = FindNamedWidget<UButton>(WidgetTree, BuildChapterWidgetName(Index, TEXT("Button")));
		CardWidgets.CardBackgroundImage = FindNamedWidget<UImage>(WidgetTree, BuildChapterWidgetName(Index, TEXT("CardBackgroundImage")));
		CardWidgets.ChapterImage = FindNamedWidget<UImage>(WidgetTree, BuildChapterWidgetName(Index, TEXT("Image")));
		CardWidgets.NumberText = FindNamedWidget<UTextBlock>(WidgetTree, BuildChapterWidgetName(Index, TEXT("NumberText")));
		CardWidgets.NameText = FindNamedWidget<UTextBlock>(WidgetTree, BuildChapterWidgetName(Index, TEXT("NameText")));
		CardWidgets.ClearImage = FindNamedWidget<UImage>(WidgetTree, BuildChapterWidgetName(Index, TEXT("ClearImage")));
		CardWidgets.SelectImage = FindNamedWidget<UImage>(WidgetTree, BuildChapterWidgetName(Index, TEXT("SelectImage")));
		BoundChapterCards.Add(CardWidgets);
	}
}

void UKumaStartScreenWidget::RefreshOptionControls()
{
	if (SoundSlider)
	{
		SoundSlider->SetValue(SoundValue / 100.0f);
	}

	if (BrightnessSlider)
	{
		BrightnessSlider->SetValue(BrightnessValue / 100.0f);
	}

	if (LanguageComboBox)
	{
		LanguageComboBox->SetSelectedOption(CurrentLanguage);
	}

	RefreshSliderValueText();
}

void UKumaStartScreenWidget::RefreshChapterCards()
{
	for (int32 Index = 0; Index < BoundChapterCards.Num(); ++Index)
	{
		if (!ChapterCards.IsValidIndex(Index))
		{
			continue;
		}

		const FKumaStartChapterCardData& CardData = ChapterCards[Index];
		FKumaStartBoundChapterCardWidgets& CardWidgets = BoundChapterCards[Index];

		SetImageBrushIfConfigured(CardWidgets.CardBackgroundImage, CardData.CardBackgroundBrush);
		SetImageBrushIfConfigured(CardWidgets.ChapterImage, CardData.ChapterImageBrush);
		SetImageBrushIfConfigured(CardWidgets.ClearImage, ClearMarkBrush);
		SetImageBrushIfConfigured(CardWidgets.SelectImage, ChapterSelectBrush);

		SetText(CardWidgets.NumberText, BuildChapterNumberText(CardData));
		SetText(CardWidgets.NameText, CardData.ChapterName);
		SetWidgetVisibility(CardWidgets.ClearImage, CardData.bCleared ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		SetWidgetVisibility(CardWidgets.SelectImage, Index == SelectedChapterIndex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UKumaStartScreenWidget::ApplyConfiguredBrushes()
{
	SetImageBrushIfConfigured(BackgroundVideoImage, BackgroundVideoBrush);
	SetImageBrushIfConfigured(StartPopupBackgroundImage, StartPopupBackgroundBrush, PopupBackgroundBrush);
	SetImageBrushIfConfigured(ChapterPopupBackgroundImage, ChapterPopupBackgroundBrush, PopupBackgroundBrush);
	SetImageBrushIfConfigured(OptionPopupBackgroundImage, OptionPopupBackgroundBrush, PopupBackgroundBrush);
	SetImageBrushIfConfigured(SaveResetWarningPopupBackgroundImage, SaveResetWarningPopupBackgroundBrush, PopupBackgroundBrush);
	SetImageBrushIfConfigured(ExitPopupBackgroundImage, ExitPopupBackgroundBrush, PopupBackgroundBrush);
	SetImageBrushIfConfigured(SaveResetWarningLogoImage, WarningLogoBrush);

	if (const FSlateBrush* InitialLogoBrush = GetInitialLogoBrush())
	{
		ApplyLogoBrushToBoundImages(*InitialLogoBrush);
	}
}

void UKumaStartScreenWidget::ApplyLogoBrushToBoundImages(const FSlateBrush& Brush)
{
	SetImageBrushIfConfigured(MainLogoImage, Brush);
	SetImageBrushIfConfigured(StartPopupLogoImage, Brush);
	SetImageBrushIfConfigured(ExitPopupLogoImage, Brush);
}

void UKumaStartScreenWidget::ShowPopupPanel(EKumaStartScreenPopup PopupType, UWidget* PopupPanel)
{
	ClosePopup();
	ActivePopup = PopupType;
	SetWidgetVisibility(PopupDimPanel, ESlateVisibility::Visible);
	SetWidgetVisibility(PopupPanel, ESlateVisibility::Visible);
}

void UKumaStartScreenWidget::ShowSaveResetWarningPopup()
{
	ShowPopupPanel(EKumaStartScreenPopup::SaveResetWarning, SaveResetWarningPopupPanel);
}

void UKumaStartScreenWidget::OpenGameplayLevel()
{
	if (GameplayLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[KumaStartScreen] GameplayLevelName is empty."));
		ClosePopup();
		return;
	}

	UGameplayStatics::OpenLevel(this, GameplayLevelName);
}

void UKumaStartScreenWidget::OpenChapterLevelIfAllowed(int32 ChapterIndex)
{
	if (!bOpenChapterLevelOnCardClick || !ChapterCards.IsValidIndex(ChapterIndex))
	{
		return;
	}

	const FName ChapterLevelName = ChapterCards[ChapterIndex].ChapterLevelName;
	if (ChapterLevelName.IsNone())
	{
		return;
	}

	UGameplayStatics::OpenLevel(this, ChapterLevelName);
}

void UKumaStartScreenWidget::ApplyBrightness(float InNormalizedBrightness)
{
	const float Clamped = FMath::Clamp(InNormalizedBrightness, 0.0f, 1.0f);
	const float DimAlpha = FMath::Lerp(MaxBrightnessDimAlpha, 0.0f, Clamped);

	if (BrightnessOverlay)
	{
		BrightnessOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, DimAlpha));
	}
}

void UKumaStartScreenWidget::ApplySoundVolume(float InNormalizedVolume)
{
	if (!MasterSoundMix || !MasterSoundClass)
	{
		return;
	}

	const float Volume = FMath::Clamp(InNormalizedVolume, 0.0f, 1.0f);
	UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix, MasterSoundClass, Volume, 1.0f, 0.05f, true);
	UGameplayStatics::PushSoundMixModifier(this, MasterSoundMix);
}

void UKumaStartScreenWidget::StartBackgroundMedia()
{
	if (!BackgroundMediaPlayer)
	{
		return;
	}

	BackgroundMediaPlayer->OnEndReached.RemoveDynamic(this, &UKumaStartScreenWidget::HandleBackgroundMediaEnded);
	BackgroundMediaPlayer->OnEndReached.AddDynamic(this, &UKumaStartScreenWidget::HandleBackgroundMediaEnded);
	BackgroundMediaPlayer->SetLooping(true);

	if (bAutoplayBackgroundMedia && BackgroundMediaSource)
	{
		BackgroundMediaPlayer->OpenSource(BackgroundMediaSource);
		BackgroundMediaPlayer->Play();
	}
}

void UKumaStartScreenWidget::StopBackgroundMedia()
{
	if (!BackgroundMediaPlayer)
	{
		return;
	}

	BackgroundMediaPlayer->OnEndReached.RemoveDynamic(this, &UKumaStartScreenWidget::HandleBackgroundMediaEnded);
	BackgroundMediaPlayer->Pause();
}

void UKumaStartScreenWidget::RefreshSliderValueText()
{
	SetText(SoundValueText, FText::AsNumber(FMath::RoundToInt(SoundValue)));
	SetText(BrightnessValueText, FText::AsNumber(FMath::RoundToInt(BrightnessValue)));
}

void UKumaStartScreenWidget::SetWidgetVisibility(UWidget* Widget, ESlateVisibility InVisibility) const
{
	if (Widget)
	{
		Widget->SetVisibility(InVisibility);
	}
}

void UKumaStartScreenWidget::SetText(UTextBlock* TextBlock, const FText& Text) const
{
	if (TextBlock)
	{
		TextBlock->SetText(Text);
	}
}

void UKumaStartScreenWidget::SetImageBrushIfConfigured(UImage* Image, const FSlateBrush& Brush) const
{
	if (Image && HasBrushResource(Brush))
	{
		Image->SetBrush(Brush);
	}
}

void UKumaStartScreenWidget::SetImageBrushIfConfigured(UImage* Image, const FSlateBrush& Brush, const FSlateBrush& FallbackBrush) const
{
	SetImageBrushIfConfigured(Image, HasBrushResource(Brush) ? Brush : FallbackBrush);
}

FText UKumaStartScreenWidget::BuildChapterNumberText(const FKumaStartChapterCardData& CardData) const
{
	if (!CardData.ChapterNumberText.IsEmpty())
	{
		return CardData.ChapterNumberText;
	}

	const FString Prefix = ChapterNumberPrefixText.IsEmpty() ? TEXT("CHAPTER") : ChapterNumberPrefixText.ToString();
	return FText::FromString(FString::Printf(TEXT("%s %02d"), *Prefix, CardData.ChapterNumber));
}

bool UKumaStartScreenWidget::HasBrushResource(const FSlateBrush& Brush) const
{
	return Brush.GetResourceObject() != nullptr;
}

const FSlateBrush* UKumaStartScreenWidget::GetInitialLogoBrush() const
{
	if (HasBrushResource(LogoBrush))
	{
		return &LogoBrush;
	}

	for (const FSlateBrush& FrameBrush : LogoSpriteFrames)
	{
		if (HasBrushResource(FrameBrush))
		{
			return &FrameBrush;
		}
	}

	return nullptr;
}

void UKumaStartScreenWidget::HandleMainStartClicked()
{
	ShowStartPopup();
}

void UKumaStartScreenWidget::HandleMainChapterClicked()
{
	ShowChapterPopup();
}

void UKumaStartScreenWidget::HandleMainOptionClicked()
{
	ShowOptionPopup();
}

void UKumaStartScreenWidget::HandleMainExitClicked()
{
	ShowExitPopup();
}

void UKumaStartScreenWidget::HandleConfirmStartClicked()
{
	OpenGameplayLevel();
}

void UKumaStartScreenWidget::HandleCancelClicked()
{
	ClosePopup();
}

void UKumaStartScreenWidget::HandleConfirmExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UKumaStartScreenWidget::HandleOptionResetClicked()
{
	SoundValue = FMath::Clamp(DefaultSoundVolume, 0.0f, 100.0f);
	BrightnessValue = FMath::Clamp(DefaultBrightness, 0.0f, 100.0f);
	CurrentLanguage = LanguageOptions.Contains(DefaultLanguage) ? DefaultLanguage : LanguageOptions[0];

	RefreshOptionControls();
	ApplySoundVolume(SoundValue / 100.0f);
	ApplyBrightness(BrightnessValue / 100.0f);
	OnKumaStartScreenLanguageChanged(CurrentLanguage);
}

void UKumaStartScreenWidget::HandleSaveResetRequestedClicked()
{
	ShowSaveResetWarningPopup();
}

void UKumaStartScreenWidget::HandleSaveResetConfirmClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[KumaStartScreen] Save reset is disabled in this UI prototype."));
	OnKumaStartScreenSaveResetPressedButDisabled();
	ShowOptionPopup();
}

void UKumaStartScreenWidget::HandleSaveResetCancelClicked()
{
	ShowOptionPopup();
}

void UKumaStartScreenWidget::HandleSoundSliderChanged(float Value)
{
	SoundValue = FMath::Clamp(Value, 0.0f, 1.0f) * 100.0f;
	ApplySoundVolume(SoundValue / 100.0f);
	RefreshSliderValueText();
}

void UKumaStartScreenWidget::HandleBrightnessSliderChanged(float Value)
{
	BrightnessValue = FMath::Clamp(Value, 0.0f, 1.0f) * 100.0f;
	ApplyBrightness(BrightnessValue / 100.0f);
	RefreshSliderValueText();
}

void UKumaStartScreenWidget::HandleLanguageChanged(FString SelectedItem, ESelectInfo::Type)
{
	if (SelectedItem.IsEmpty())
	{
		return;
	}

	CurrentLanguage = SelectedItem;
	OnKumaStartScreenLanguageChanged(CurrentLanguage);
}

void UKumaStartScreenWidget::HandleBackgroundMediaEnded()
{
	if (BackgroundMediaPlayer)
	{
		BackgroundMediaPlayer->Rewind();
		BackgroundMediaPlayer->Play();
	}
}

void UKumaStartScreenWidget::HandleChapterCard1Hovered()
{
	SelectChapterCard(0);
}

void UKumaStartScreenWidget::HandleChapterCard2Hovered()
{
	SelectChapterCard(1);
}

void UKumaStartScreenWidget::HandleChapterCard3Hovered()
{
	SelectChapterCard(2);
}

void UKumaStartScreenWidget::HandleChapterCard4Hovered()
{
	SelectChapterCard(3);
}

void UKumaStartScreenWidget::HandleChapterCard5Hovered()
{
	SelectChapterCard(4);
}

void UKumaStartScreenWidget::HandleChapterCard1Clicked()
{
	SelectChapterCard(0);
	OpenChapterLevelIfAllowed(0);
}

void UKumaStartScreenWidget::HandleChapterCard2Clicked()
{
	SelectChapterCard(1);
	OpenChapterLevelIfAllowed(1);
}

void UKumaStartScreenWidget::HandleChapterCard3Clicked()
{
	SelectChapterCard(2);
	OpenChapterLevelIfAllowed(2);
}

void UKumaStartScreenWidget::HandleChapterCard4Clicked()
{
	SelectChapterCard(3);
	OpenChapterLevelIfAllowed(3);
}

void UKumaStartScreenWidget::HandleChapterCard5Clicked()
{
	SelectChapterCard(4);
	OpenChapterLevelIfAllowed(4);
}
