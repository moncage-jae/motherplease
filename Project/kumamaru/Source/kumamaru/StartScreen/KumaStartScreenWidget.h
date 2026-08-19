// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "Types/SlateEnums.h"
#include "KumaStartScreenWidget.generated.h"

class UBorder;
class UButton;
class UComboBoxString;
class UImage;
class UMediaPlayer;
class UMediaSource;
class USlider;
class USoundClass;
class USoundMix;
class UTextBlock;
class UWidget;

UENUM(BlueprintType)
enum class EKumaStartScreenPopup : uint8
{
	None,
	Start,
	Chapter,
	Option,
	SaveResetWarning,
	Exit
};

USTRUCT(BlueprintType)
struct FKumaStartChapterCardData
{
	GENERATED_BODY()

	FKumaStartChapterCardData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter")
	int32 ChapterNumber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter")
	FText ChapterNumberText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter")
	FText ChapterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter")
	bool bCleared;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter")
	FName ChapterLevelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter")
	FSlateBrush CardBackgroundBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter")
	FSlateBrush ChapterImageBrush;
};

USTRUCT()
struct FKumaStartBoundChapterCardWidgets
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(Transient)
	TObjectPtr<UImage> CardBackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChapterImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NumberText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ClearImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SelectImage;
};

UCLASS(BlueprintType, Blueprintable)
class KUMAMARU_API UKumaStartScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UKumaStartScreenWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Kuma Start Screen")
	void ShowStartPopup();

	UFUNCTION(BlueprintCallable, Category = "Kuma Start Screen")
	void ShowChapterPopup();

	UFUNCTION(BlueprintCallable, Category = "Kuma Start Screen")
	void ShowOptionPopup();

	UFUNCTION(BlueprintCallable, Category = "Kuma Start Screen")
	void ShowExitPopup();

	UFUNCTION(BlueprintCallable, Category = "Kuma Start Screen")
	void ClosePopup();

	UFUNCTION(BlueprintCallable, Category = "Kuma Start Screen|Chapter")
	void SelectChapterCard(int32 ChapterIndex);

	UFUNCTION(BlueprintCallable, Category = "Kuma Start Screen|Option")
	FString GetCurrentLanguage() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Kuma Start Screen")
	void OnKumaStartScreenLanguageChanged(const FString& SelectedLanguage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Kuma Start Screen")
	void OnKumaStartScreenChapterSelected(int32 ChapterIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Kuma Start Screen")
	void OnKumaStartScreenSaveResetPressedButDisabled();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG")
	TObjectPtr<UWidget> MainMenuPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG")
	TObjectPtr<UImage> BackgroundVideoImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG")
	TObjectPtr<UBorder> BrightnessOverlay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG")
	TObjectPtr<UImage> MainLogoImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG")
	TObjectPtr<UTextBlock> PlayTimeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Main Buttons")
	TObjectPtr<UButton> MainStartButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Main Buttons")
	TObjectPtr<UButton> MainChapterButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Main Buttons")
	TObjectPtr<UButton> MainOptionButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Main Buttons")
	TObjectPtr<UButton> MainExitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup")
	TObjectPtr<UWidget> PopupDimPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup")
	TObjectPtr<UWidget> StartPopupPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup")
	TObjectPtr<UWidget> ChapterPopupPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup")
	TObjectPtr<UWidget> OptionPopupPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup")
	TObjectPtr<UWidget> SaveResetWarningPopupPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup")
	TObjectPtr<UWidget> ExitPopupPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup Images")
	TObjectPtr<UImage> StartPopupBackgroundImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup Images")
	TObjectPtr<UImage> ChapterPopupBackgroundImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup Images")
	TObjectPtr<UImage> OptionPopupBackgroundImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup Images")
	TObjectPtr<UImage> SaveResetWarningPopupBackgroundImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup Images")
	TObjectPtr<UImage> ExitPopupBackgroundImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup Images")
	TObjectPtr<UImage> StartPopupLogoImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup Images")
	TObjectPtr<UImage> SaveResetWarningLogoImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Popup Images")
	TObjectPtr<UImage> ExitPopupLogoImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Start Popup")
	TObjectPtr<UButton> StartConfirmButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Start Popup")
	TObjectPtr<UButton> StartCancelButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Chapter Popup")
	TObjectPtr<UButton> ChapterCloseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Option Popup")
	TObjectPtr<UButton> OptionCloseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Option Popup")
	TObjectPtr<USlider> SoundSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Option Popup")
	TObjectPtr<USlider> BrightnessSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Option Popup")
	TObjectPtr<UTextBlock> SoundValueText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Option Popup")
	TObjectPtr<UTextBlock> BrightnessValueText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Option Popup")
	TObjectPtr<UComboBoxString> LanguageComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Option Popup")
	TObjectPtr<UButton> OptionResetButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Option Popup")
	TObjectPtr<UButton> SaveResetRequestButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Save Reset Warning")
	TObjectPtr<UButton> SaveResetConfirmButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Save Reset Warning")
	TObjectPtr<UButton> SaveResetCancelButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Exit Popup")
	TObjectPtr<UButton> ExitConfirmButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Kuma Start Screen|UMG|Exit Popup")
	TObjectPtr<UButton> ExitCancelButton;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Flow", meta = (AllowPrivateAccess = "true"))
	FName GameplayLevelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Flow", meta = (AllowPrivateAccess = "true"))
	bool bOpenChapterLevelOnCardClick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Media", meta = (AllowPrivateAccess = "true"))
	bool bAutoplayBackgroundMedia;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Media", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaPlayer> BackgroundMediaPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Media", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMediaSource> BackgroundMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush BackgroundVideoBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush LogoBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	TArray<FSlateBrush> LogoSpriteFrames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float LogoSpriteFrameInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float LogoPulseSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float LogoPulseScaleAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush PopupBackgroundBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush StartPopupBackgroundBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush ChapterPopupBackgroundBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush OptionPopupBackgroundBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush SaveResetWarningPopupBackgroundBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush ExitPopupBackgroundBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush WarningLogoBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush ClearMarkBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Brushes", meta = (AllowPrivateAccess = "true"))
	FSlateBrush ChapterSelectBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Option", meta = (ClampMin = "0.0", ClampMax = "100.0", AllowPrivateAccess = "true"))
	float DefaultSoundVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Option", meta = (ClampMin = "0.0", ClampMax = "100.0", AllowPrivateAccess = "true"))
	float DefaultBrightness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Option", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess = "true"))
	float MaxBrightnessDimAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Option", meta = (AllowPrivateAccess = "true"))
	FString DefaultLanguage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Option", meta = (AllowPrivateAccess = "true"))
	TArray<FString> LanguageOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Option", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundMix> MasterSoundMix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Option", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter", meta = (AllowPrivateAccess = "true"))
	FText ChapterNumberPrefixText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Start Screen|Chapter", meta = (AllowPrivateAccess = "true"))
	TArray<FKumaStartChapterCardData> ChapterCards;

	UPROPERTY(Transient)
	TArray<FKumaStartBoundChapterCardWidgets> BoundChapterCards;

	EKumaStartScreenPopup ActivePopup;
	int32 SelectedChapterIndex;
	int32 CurrentLogoFrameIndex;
	float LogoFrameAccumulator;
	float LogoPulseTime;
	float SoundValue;
	float BrightnessValue;
	FString CurrentLanguage;

	void BindWidgetEvents();
	void CacheChapterCardWidgets();
	void RefreshOptionControls();
	void RefreshChapterCards();
	void ApplyConfiguredBrushes();
	void ApplyLogoBrushToBoundImages(const FSlateBrush& Brush);
	void ShowPopupPanel(EKumaStartScreenPopup PopupType, UWidget* PopupPanel);
	void ShowSaveResetWarningPopup();
	void OpenGameplayLevel();
	void OpenChapterLevelIfAllowed(int32 ChapterIndex);
	void ApplyBrightness(float InNormalizedBrightness);
	void ApplySoundVolume(float InNormalizedVolume);
	void StartBackgroundMedia();
	void StopBackgroundMedia();
	void RefreshSliderValueText();
	void SetWidgetVisibility(UWidget* Widget, ESlateVisibility InVisibility) const;
	void SetText(UTextBlock* TextBlock, const FText& Text) const;
	void SetImageBrushIfConfigured(UImage* Image, const FSlateBrush& Brush) const;
	void SetImageBrushIfConfigured(UImage* Image, const FSlateBrush& Brush, const FSlateBrush& FallbackBrush) const;
	FText BuildChapterNumberText(const FKumaStartChapterCardData& CardData) const;
	bool HasBrushResource(const FSlateBrush& Brush) const;
	const FSlateBrush* GetInitialLogoBrush() const;

	UFUNCTION()
	void HandleMainStartClicked();

	UFUNCTION()
	void HandleMainChapterClicked();

	UFUNCTION()
	void HandleMainOptionClicked();

	UFUNCTION()
	void HandleMainExitClicked();

	UFUNCTION()
	void HandleConfirmStartClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleConfirmExitClicked();

	UFUNCTION()
	void HandleOptionResetClicked();

	UFUNCTION()
	void HandleSaveResetRequestedClicked();

	UFUNCTION()
	void HandleSaveResetConfirmClicked();

	UFUNCTION()
	void HandleSaveResetCancelClicked();

	UFUNCTION()
	void HandleSoundSliderChanged(float Value);

	UFUNCTION()
	void HandleBrightnessSliderChanged(float Value);

	UFUNCTION()
	void HandleLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleBackgroundMediaEnded();

	UFUNCTION()
	void HandleChapterCard1Hovered();

	UFUNCTION()
	void HandleChapterCard2Hovered();

	UFUNCTION()
	void HandleChapterCard3Hovered();

	UFUNCTION()
	void HandleChapterCard4Hovered();

	UFUNCTION()
	void HandleChapterCard5Hovered();

	UFUNCTION()
	void HandleChapterCard1Clicked();

	UFUNCTION()
	void HandleChapterCard2Clicked();

	UFUNCTION()
	void HandleChapterCard3Clicked();

	UFUNCTION()
	void HandleChapterCard4Clicked();

	UFUNCTION()
	void HandleChapterCard5Clicked();
};
