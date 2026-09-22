#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KumaChapterEndWidget.generated.h"
class UButton;
DECLARE_MULTICAST_DELEGATE(FKumaNextChapterClicked);
DECLARE_MULTICAST_DELEGATE(FKumaMainMenuClicked);
DECLARE_MULTICAST_DELEGATE(FKumaQuitClicked);
UCLASS()
class KUMAMARU_API UKumaChapterEndWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	FKumaNextChapterClicked OnNextChapterClicked;
	FKumaMainMenuClicked OnMainMenuClicked;
	FKumaQuitClicked OnQuitClicked;

	/** Shows or hides the optional next-chapter button. */
	void SetNextChapterVisible(bool bVisible);
protected:
	virtual void NativeOnInitialized() override;
private:
	void BuildLayout();
	UFUNCTION() void HandleNext();
	UFUNCTION() void HandleMainMenu();
	UFUNCTION() void HandleQuit();
	UPROPERTY(Transient) TObjectPtr<UButton> NextButton;
	UPROPERTY(Transient) TObjectPtr<UButton> MainMenuButton;
	UPROPERTY(Transient) TObjectPtr<UButton> QuitButton;
};
