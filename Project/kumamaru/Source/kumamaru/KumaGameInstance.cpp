// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaGameInstance.h"

#include "KumaChapterOneDirector.h"
#include "KumaChapterTwoDirector.h"
#include "KumaChapterIntroWidget.h"
#include "KumaSaveGame.h"
#include "KumaStoryFlowSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogKumaSave, Log, All);

namespace
{
	void SetChapterOnlyModelsVisible(UWorld* World, bool bVisible)
	{
		if (!World)
		{
			return;
		}

		static const TSet<FName> TargetModelNames = {
			TEXT("SM_IR2"),
			TEXT("SM_IR_RMC_IR_RMC"),
			TEXT("SM_IR_RMC_IR_RMC_Button")
		};

		int32 ChangedComponentCount = 0;
		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			AActor* Actor = *ActorIt;
			if (!Actor)
			{
				continue;
			}

			TInlineComponentArray<UStaticMeshComponent*> MeshComponents;
			Actor->GetComponents(MeshComponents);
			for (UStaticMeshComponent* MeshComponent : MeshComponents)
			{
				if (!MeshComponent)
				{
					continue;
				}

				const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
				bool bIsTargetModel = TargetModelNames.Contains(Actor->GetFName())
					|| TargetModelNames.Contains(MeshComponent->GetFName())
					|| (StaticMesh && TargetModelNames.Contains(StaticMesh->GetFName()));
#if WITH_EDITOR
				bIsTargetModel |= TargetModelNames.Contains(FName(*Actor->GetActorLabel()));
#endif
				if (!bIsTargetModel)
				{
					continue;
				}

				MeshComponent->SetVisibility(bVisible, true);
				MeshComponent->SetHiddenInGame(!bVisible, true);
				++ChangedComponentCount;
			}
		}

		UE_LOG(LogKumaSave, Log, TEXT("[KumaChapterVisibility] %d target model component(s) are now %s."), ChangedComponentCount, bVisible ? TEXT("visible") : TEXT("hidden"));
	}
}

void UKumaGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UKumaGameInstance::HandlePostLoadMapWithWorld);
}

void UKumaGameInstance::Shutdown()
{
	StopChapterAmbience();
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
	if (bStartChapterTwoAfterMapLoad && LoadedWorld && LoadedWorld->IsGameWorld())
	{
		bStartChapterTwoAfterMapLoad = false;
		ChapterOneDirector = nullptr;
		ChapterTwoDirector = nullptr;
		LoadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UKumaGameInstance::ShowChapterTwoIntro, LoadedWorld));
	}
	else
	{
		TryStartChapterOne(LoadedWorld);
	}

	if (!bApplyKumaSaveAfterMapLoad || !LoadedWorld)
	{
		return;
	}

	LoadedWorld->GetTimerManager().SetTimerForNextTick(this, &UKumaGameInstance::ApplyPendingWorldState);
}

void UKumaGameInstance::OpenChapterTwo()
{
	bStartChapterTwoAfterMapLoad = true;
	UGameplayStatics::OpenLevel(this, ChapterOneLevelName);
}

void UKumaGameInstance::StartChapterAmbience(UWorld* World)
{
	StopChapterAmbience();
	if (!World)
	{
		return;
	}

	USoundBase* AmbienceSound = ChapterAmbienceSound.LoadSynchronous();
	if (!AmbienceSound)
	{
		UE_LOG(LogKumaSave, Warning, TEXT("[KumaAudio] Could not load chapter ambience: %s"), *ChapterAmbienceSound.ToSoftObjectPath().ToString());
		return;
	}

	bChapterAmbienceShouldLoop = true;
	ChapterAmbienceAudioComponent = UGameplayStatics::SpawnSound2D(World, AmbienceSound, 1.f, 1.f, 0.f, nullptr, false, false);
	if (!ChapterAmbienceAudioComponent)
	{
		bChapterAmbienceShouldLoop = false;
		UE_LOG(LogKumaSave, Warning, TEXT("[KumaAudio] Could not create the chapter ambience audio component."));
		return;
	}

	ChapterAmbienceAudioComponent->OnAudioFinished.AddDynamic(this, &UKumaGameInstance::HandleChapterAmbienceFinished);
	UE_LOG(LogKumaSave, Log, TEXT("[KumaAudio] Chapter ambience started and will loop until the mini-game is cleared."));
}

void UKumaGameInstance::StopChapterAmbience()
{
	bChapterAmbienceShouldLoop = false;
	if (ChapterAmbienceAudioComponent)
	{
		ChapterAmbienceAudioComponent->OnAudioFinished.RemoveDynamic(this, &UKumaGameInstance::HandleChapterAmbienceFinished);
		ChapterAmbienceAudioComponent->Stop();
		ChapterAmbienceAudioComponent = nullptr;
		UE_LOG(LogKumaSave, Log, TEXT("[KumaAudio] Chapter ambience stopped."));
	}
}

void UKumaGameInstance::HandleChapterAmbienceFinished()
{
	if (bChapterAmbienceShouldLoop && ChapterAmbienceAudioComponent)
	{
		ChapterAmbienceAudioComponent->Play(0.f);
	}
}

void UKumaGameInstance::ShowChapterTwoIntro(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;
	SetChapterOnlyModelsVisible(LoadedWorld, true);
	StartChapterAmbience(LoadedWorld);
	ChapterTwoDirector = LoadedWorld->SpawnActor<AKumaChapterTwoDirector>();
	if (!ChapterTwoDirector)
	{
		UE_LOG(LogKumaSave, Error, TEXT("Failed to spawn the Chapter 2 director in level %s."), *LoadedWorld->GetMapName());
		return;
	}

	APlayerController* PC = LoadedWorld->GetFirstPlayerController();
	if (!PC)
	{
		ChapterTwoDirector->StartChapterTwo();
		return;
	}
	UKumaChapterIntroWidget* Intro = CreateWidget<UKumaChapterIntroWidget>(PC, UKumaChapterIntroWidget::StaticClass());
	if (!Intro)
	{
		ChapterTwoDirector->StartChapterTwo();
		return;
	}
	Intro->SetChapterInfo(FText::FromString(TEXT("CHAPTER 2")), FText::FromString(TEXT("Mother, Please")));
	Intro->OnIntroFinished.AddUObject(ChapterTwoDirector, &AKumaChapterTwoDirector::StartChapterTwo);
	Intro->AddToViewport(100);
	Intro->PlayIntro(1.5f, 1.5f);
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

void UKumaGameInstance::TryStartChapterOne(UWorld* LoadedWorld)
{
	if (!bAutoStartChapterOne || !LoadedWorld || !LoadedWorld->IsGameWorld())
	{
		return;
	}

	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(LoadedWorld, true));
	if (ChapterOneLevelName.IsNone() || !CurrentLevelName.IsEqual(ChapterOneLevelName, ENameCase::IgnoreCase))
	{
		return;
	}

	if (ChapterOneDirector && ChapterOneDirector->GetWorld() == LoadedWorld && !ChapterOneDirector->IsActorBeingDestroyed())
	{
		return;
	}

	ChapterOneDirector = nullptr;
	LoadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UKumaGameInstance::SpawnChapterOneDirector, LoadedWorld));
}

void UKumaGameInstance::SpawnChapterOneDirector(UWorld* LoadedWorld)
{
	if (!LoadedWorld || !LoadedWorld->IsGameWorld() || (ChapterOneDirector && !ChapterOneDirector->IsActorBeingDestroyed()))
	{
		return;
	}

	SetChapterOnlyModelsVisible(LoadedWorld, false);
	StartChapterAmbience(LoadedWorld);
	ChapterOneDirector = LoadedWorld->SpawnActor<AKumaChapterOneDirector>();
	if (!ChapterOneDirector)
	{
		UE_LOG(LogKumaSave, Error, TEXT("Failed to spawn the Chapter 1 director in level %s."), *LoadedWorld->GetMapName());
	}
}
