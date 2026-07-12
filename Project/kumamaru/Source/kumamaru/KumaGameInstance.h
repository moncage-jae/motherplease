// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "KumaGameInstance.generated.h"

class UKumaSaveGame;

UCLASS()
class KUMAMARU_API UKumaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "Kuma Save")
	bool SaveKumaGame();

	UFUNCTION(BlueprintCallable, Category = "Kuma Save")
	bool LoadKumaGame();

	UFUNCTION(BlueprintCallable, Category = "Kuma Save")
	bool SaveKumaGameToSlot(const FString& SlotName, int32 UserIndex);

	UFUNCTION(BlueprintCallable, Category = "Kuma Save")
	bool LoadKumaGameFromSlot(const FString& SlotName, int32 UserIndex);

	UFUNCTION(BlueprintCallable, Category = "Kuma Save")
	bool DoesKumaSaveExist() const;

	UFUNCTION(BlueprintCallable, Category = "Kuma Save")
	bool DoesKumaSaveExistInSlot(const FString& SlotName, int32 UserIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Kuma Save")
	UKumaSaveGame* GetCurrentKumaSave() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kuma Save")
	FString DefaultSaveSlotName = TEXT("KumaSlot_0");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kuma Save")
	int32 DefaultUserIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Kuma Save")
	TObjectPtr<UKumaSaveGame> CurrentKumaSave;

private:
	bool bApplyKumaSaveAfterMapLoad = false;

	void CaptureWorldState(UKumaSaveGame* SaveGameObject) const;
	bool ApplyWorldState(const UKumaSaveGame* SaveGameObject) const;
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ApplyPendingWorldState();
};
