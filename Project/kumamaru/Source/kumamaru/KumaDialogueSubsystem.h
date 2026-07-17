// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KumaDialogueSubsystem.generated.h"

class UKumaDialogueTypingComponent;
class UUserWidget;

UENUM(BlueprintType)
enum class EKumaDialogueAdvancePolicy : uint8
{
	WaitForInput UMETA(DisplayName = "Wait For Input"),
	AutoNext UMETA(DisplayName = "Auto Next"),
	AutoClose UMETA(DisplayName = "Auto Close")
};

USTRUCT(BlueprintType)
struct FKumaDialogueOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue", meta = (ClampMin = "0.0"))
	float TypingSpeed = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue")
	EKumaDialogueAdvancePolicy AdvancePolicy = EKumaDialogueAdvancePolicy::WaitForInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue", meta = (ClampMin = "0.0"))
	float AutoAdvanceDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue", meta = (ClampMin = "0.0"))
	float AutoCloseDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue")
	bool bAllowSkipTyping = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue")
	bool bCloseWhenFinished = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue")
	bool bBlockGameplayInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue")
	bool bHideCrosshair = false;
};

USTRUCT(BlueprintType)
struct FKumaDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue")
	FName LineId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue")
	FName SpeakerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue", meta = (MultiLine = "true"))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Dialogue")
	FKumaDialogueOptions Options;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKumaDialogueLineStartedSignature, FName, LineId, FName, SpeakerId, const FText&, FullText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKumaDialogueTextUpdatedSignature, const FText&, PartialText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKumaDialogueLineCompletedSignature, FName, LineId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKumaDialogueClosedSignature, FName, LastLineId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaDialogueSequenceFinishedSignature, FName, OwnerId, FName, LastLineId);

UCLASS()
class KUMAMARU_API UKumaDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UKumaDialogueSubsystem();

	UFUNCTION(BlueprintCallable, Category = "Kuma Dialogue")
	bool PlayDialogueLine(FName OwnerId, FName LineId, FName SpeakerId, const FText& Text, FKumaDialogueOptions Options);

	UFUNCTION(BlueprintCallable, Category = "Kuma Dialogue")
	bool PlayDialogueSequence(FName OwnerId, const TArray<FKumaDialogueLine>& DialogueLines);

	UFUNCTION(BlueprintCallable, Category = "Kuma Dialogue")
	bool AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "Kuma Dialogue")
	bool CloseDialogue();

	UFUNCTION(BlueprintPure, Category = "Kuma Dialogue")
	bool IsDialogueActive() const { return bDialogueActive; }

	UFUNCTION(BlueprintPure, Category = "Kuma Dialogue")
	bool IsDialogueVisible() const { return bDialogueVisible; }

	UFUNCTION(BlueprintPure, Category = "Kuma Dialogue")
	bool IsDialogueTyping() const;

	UFUNCTION(BlueprintPure, Category = "Kuma Dialogue")
	FName GetActiveOwnerId() const { return ActiveOwnerId; }

	UFUNCTION(BlueprintPure, Category = "Kuma Dialogue")
	FName GetActiveLineId() const;

	UFUNCTION(BlueprintPure, Category = "Kuma Dialogue")
	FKumaDialogueOptions GetActiveOptions() const;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Dialogue|Events")
	FKumaDialogueLineStartedSignature OnDialogueLineStarted;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Dialogue|Events")
	FKumaDialogueTextUpdatedSignature OnDialogueTextUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Dialogue|Events")
	FKumaDialogueLineCompletedSignature OnDialogueLineCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Dialogue|Events")
	FKumaDialogueClosedSignature OnDialogueClosed;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Dialogue|Events")
	FKumaDialogueSequenceFinishedSignature OnDialogueSequenceFinished;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool StartLine(int32 LineIndex);
	bool AdvanceAfterLineCompleted();
	void FinishDialogueSequence(bool bCloseWidget);
	void ClearAutoAdvanceTimer();
	void CancelTyping();
	void ScheduleAutoAdvance(float DelaySeconds);
	void HandleAutoAdvanceTimer();
	void HandleTypingTextUpdated(const FText& PartialText);
	void HandleTypingCompleted();
	void SetDialogueWidgetsVisible(bool bVisible) const;
	bool IsDialogueWidget(const UUserWidget* Widget) const;
	const FKumaDialogueLine* GetActiveLine() const;
	FKumaDialogueLine NormalizeDialogueLine(const FKumaDialogueLine& DialogueLine) const;
	FText NormalizeDialogueText(const FText& Text) const;

	UPROPERTY(EditDefaultsOnly, Category = "Kuma Dialogue")
	FName DialogueWidgetClassName = FName(TEXT("WBP_Dialogue_C"));

	UPROPERTY(Transient)
	TObjectPtr<UKumaDialogueTypingComponent> TypingComponent;

	TArray<FKumaDialogueLine> ActiveLines;
	FName ActiveOwnerId;
	FName LastCompletedLineId;
	int32 ActiveLineIndex = INDEX_NONE;
	bool bDialogueActive = false;
	bool bDialogueVisible = false;
	bool bLineTextCompleted = false;
	FTimerHandle AutoAdvanceTimerHandle;
};
