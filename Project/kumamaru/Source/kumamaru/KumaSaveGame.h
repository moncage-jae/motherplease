// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KumaStoryTypes.h"
#include "KumaSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FKumaPlayerSaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save|Player")
	bool bHasTransform = false;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save|Player")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save|Player")
	FRotator Rotation = FRotator::ZeroRotator;
};

UCLASS()
class KUMAMARU_API UKumaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UKumaSaveGame();

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save")
	FString SaveVersion;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save")
	FString SlotName;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save")
	int32 UserIndex = 0;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save")
	FString SavedLevelName;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save")
	FKumaPlayerSaveData PlayerData;

	// Dialogue progress can start as simple named flags, then grow into richer structs later.
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save|Dialogue")
	TMap<FName, bool> DialogueFlags;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Kuma Save|Story")
	FKumaStorySaveData StoryData;
};
