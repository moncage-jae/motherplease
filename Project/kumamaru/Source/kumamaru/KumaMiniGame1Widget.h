#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KumaMiniGame1Widget.generated.h"

class UBorder;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKumaMiniGame1InstructionFinished);

/** Code-only overlay for MiniGame 1: instruction, countdown, and failure fade. */
UCLASS()
class KUMAMARU_API UKumaMiniGame1Widget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Kuma MiniGame 1|Events")
	FKumaMiniGame1InstructionFinished OnInstructionFinished;

	void BeginMiniGame();
	void EndMiniGame();
	void ShowInstruction(const FText& Instruction);
	void HideInstruction();
	void SetCountdownVisible(bool bVisible);
	void SetCountdownSeconds(float RemainingSeconds);
	void BeginFailureFade();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void UpdateInstructionFade();
	void UpdateFailureFade();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InstructionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FailureFade;

	FTimerHandle InstructionTimer;
	FTimerHandle FailureFadeTimer;
	float InstructionElapsed = 0.f;
	float FailureFadeElapsed = 0.f;
};
