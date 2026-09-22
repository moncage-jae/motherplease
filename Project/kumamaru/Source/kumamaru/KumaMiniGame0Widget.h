#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KumaMiniGame0Widget.generated.h"

class UTextBlock;

UCLASS()
class KUMAMARU_API UKumaMiniGame0Widget : public UUserWidget
{
	GENERATED_BODY()
public:
	void BeginMiniGame();
	void EndMiniGame();
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
private:
	void BuildLayout();
	void UpdateInstructionFade();
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CrosshairText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InstructionText;
	FTimerHandle InstructionTimer;
	float InstructionElapsed = 0.f;
};
