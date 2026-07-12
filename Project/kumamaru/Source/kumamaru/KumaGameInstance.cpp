// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaGameInstance.h"

#include "KumaSaveGame.h"
#include "KumaStoryFlowSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogKumaSave, Log, All);

void UKumaGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UKumaGameInstance::HandlePostLoadMapWithWorld);
}

void UKumaGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Shutdown();
}

bool UKumaGameInstance::SaveKumaGame()
{
	return SaveKumaGameToSlot(DefaultSaveSlotName, DefaultUserIndex);
}

bool UKumaGameInstance::LoadKumaGame()
{
	return LoadKumaGameFromSlot(DefaultSaveSlotName, DefaultUserIndex);
}

bool UKumaGameInstance::SaveKumaGameToSlot(const FString& SlotName, int32 UserIndex)
{
	UKumaSaveGame* SaveGameObject = Cast<UKumaSaveGame>(UGameplayStatics::CreateSaveGameObject(UKumaSaveGame::StaticClass()));
	if (!SaveGameObject)
	{
		UE_LOG(LogKumaSave, Error, TEXT("Failed to create Kuma save game object."));
		return false;
	}

	SaveGameObject->SlotName = SlotName;
	SaveGameObject->UserIndex = UserIndex;
	CaptureWorldState(SaveGameObject);

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGameObject, SlotName, UserIndex);
	if (bSaved)
	{
		CurrentKumaSave = SaveGameObject;
		UE_LOG(LogKumaSave, Log, TEXT("Saved Kuma game. Slot: %s, Level: %s"), *SlotName, *SaveGameObject->SavedLevelName);
	}
	else
	{
		UE_LOG(LogKumaSave, Error, TEXT("Failed to save Kuma game. Slot: %s"), *SlotName);
	}

	return bSaved;
}

bool UKumaGameInstance::LoadKumaGameFromSlot(const FString& SlotName, int32 UserIndex)
{
	if (!DoesKumaSaveExistInSlot(SlotName, UserIndex))
	{
		UE_LOG(LogKumaSave, Warning, TEXT("No Kuma save exists. Slot: %s"), *SlotName);
		return false;
	}

	UKumaSaveGame* LoadedSave = Cast<UKumaSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!LoadedSave)
	{
		UE_LOG(LogKumaSave, Error, TEXT("Failed to load Kuma save. Slot: %s"), *SlotName);
		return false;
	}

	LoadedSave->SlotName = SlotName;
	LoadedSave->UserIndex = UserIndex;
	CurrentKumaSave = LoadedSave;

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!LoadedSave->SavedLevelName.IsEmpty() && LoadedSave->SavedLevelName != CurrentLevelName)
	{
		bApplyKumaSaveAfterMapLoad = true;
		UE_LOG(LogKumaSave, Log, TEXT("Opening saved level before applying Kuma save. From: %s, To: %s"), *CurrentLevelName, *LoadedSave->SavedLevelName);
		UGameplayStatics::OpenLevel(this, FName(*LoadedSave->SavedLevelName));
		return true;
	}

	return ApplyWorldState(LoadedSave);
}

bool UKumaGameInstance::DoesKumaSaveExist() const
{
	return DoesKumaSaveExistInSlot(DefaultSaveSlotName, DefaultUserIndex);
}

bool UKumaGameInstance::DoesKumaSaveExistInSlot(const FString& SlotName, int32 UserIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

UKumaSaveGame* UKumaGameInstance::GetCurrentKumaSave() const
{
	return CurrentKumaSave;
}

void UKumaGameInstance::CaptureWorldState(UKumaSaveGame* SaveGameObject) const
{
	if (!SaveGameObject)
	{
		return;
	}

	SaveGameObject->SavedLevelName = UGameplayStatics::GetCurrentLevelName(this, true);

	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!PlayerCharacter)
	{
		SaveGameObject->PlayerData.bHasTransform = false;
		UE_LOG(LogKumaSave, Warning, TEXT("Saved Kuma game without player transform because no player character was found."));
	}
	else
	{
		SaveGameObject->PlayerData.bHasTransform = true;
		SaveGameObject->PlayerData.Location = PlayerCharacter->GetActorLocation();
		SaveGameObject->PlayerData.Rotation = PlayerCharacter->GetActorRotation();
	}

	if (UKumaStoryFlowSubsystem* StoryFlowSubsystem = GetSubsystem<UKumaStoryFlowSubsystem>())
	{
		StoryFlowSubsystem->CaptureStorySaveData(SaveGameObject->StoryData);
	}
}

bool UKumaGameInstance::ApplyWorldState(const UKumaSaveGame* SaveGameObject) const
{
	if (!SaveGameObject)
	{
		return false;
	}

	bool bAppliedAnyState = false;

	if (!SaveGameObject->PlayerData.bHasTransform)
	{
		UE_LOG(LogKumaSave, Warning, TEXT("Kuma save has no player transform to apply."));
	}
	else
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (!PlayerCharacter)
		{
			UE_LOG(LogKumaSave, Warning, TEXT("Could not apply Kuma save because no player character was found."));
		}
		else
		{
			PlayerCharacter->SetActorLocationAndRotation(
				SaveGameObject->PlayerData.Location,
				SaveGameObject->PlayerData.Rotation,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);

			if (AController* Controller = PlayerCharacter->GetController())
			{
				Controller->SetControlRotation(SaveGameObject->PlayerData.Rotation);
			}

			UE_LOG(LogKumaSave, Log, TEXT("Applied Kuma save. Location: %s, Rotation: %s"),
				*SaveGameObject->PlayerData.Location.ToString(),
				*SaveGameObject->PlayerData.Rotation.ToString());

			bAppliedAnyState = true;
		}
	}

	if (UKumaStoryFlowSubsystem* StoryFlowSubsystem = GetSubsystem<UKumaStoryFlowSubsystem>())
	{
		StoryFlowSubsystem->RestoreStorySaveData(SaveGameObject->StoryData);
		bAppliedAnyState = bAppliedAnyState || SaveGameObject->StoryData.bHasStoryStep;
	}

	return bAppliedAnyState;
}

void UKumaGameInstance::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!bApplyKumaSaveAfterMapLoad || !LoadedWorld)
	{
		return;
	}

	LoadedWorld->GetTimerManager().SetTimerForNextTick(this, &UKumaGameInstance::ApplyPendingWorldState);
}

void UKumaGameInstance::ApplyPendingWorldState()
{
	if (!bApplyKumaSaveAfterMapLoad)
	{
		return;
	}

	bApplyKumaSaveAfterMapLoad = false;
	ApplyWorldState(CurrentKumaSave);
}
