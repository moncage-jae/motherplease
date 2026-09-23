// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaChapterOneDirector.h"

#include "ArmControlComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "KumaChapterIntroWidget.h"
#include "KumaDialogueBoxWidget.h"
#include "KumaMiniGame0Widget.h"
#include "KumaChapterEndWidget.h"
#include "KumaGameInstance.h"
#include "AMPPlayerCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MediaPlayer.h"
#include "FileMediaSource.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogKumaChapterOne, Log, All);

namespace KumaChapterOne
{
	const FName RoomNarration(TEXT("CH01_ROOM_NARRATION"));
	const FName HankCall(TEXT("CH01_HANK_CALL"));
	const FName MotherEnter(TEXT("CH01_MOTHER_ENTER"));
	const FName MotherConversation(TEXT("CH01_MOTHER_CONVERSATION"));
	const FName MotherExit(TEXT("CH01_MOTHER_EXIT"));
	const FName MiniGameFirstFound(TEXT("MG0_FIRST_FOUND"));
	const FName MiniGameSecondFound(TEXT("MG0_SECOND_FOUND"));
	const FName MiniGameTVReaction(TEXT("MG0_TV_REACTION"));
	const FName MiniGameEnd(TEXT("MG0_END"));

	const FName Tomas(TEXT("tomas"));
	const FName Hank(TEXT("hank"));
	const FName Annie(TEXT("annie"));
	constexpr float ButterflyReticleTolerance = 7.5f;
}

AKumaChapterOneDirector::AKumaChapterOneDirector()
{
	PrimaryActorTick.bCanEverTick = true;

	HankSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Test/videotape_sequence1.videotape_sequence1")));
	MotherEnterSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Characters/Mother_ANI/Comein1.Comein1")));
	MotherExitSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Characters/Mother_ANI/goout1.goout1")));
	TVMediaPlayerAsset = TSoftObjectPtr<UMediaPlayer>(FSoftObjectPath(TEXT("/Game/Test/video_test/NewMediaPlayer.NewMediaPlayer")));
	TVMediaSourceAsset = TSoftObjectPtr<UFileMediaSource>(FSoftObjectPath(TEXT("/Game/Test/video_test/vid1.vid1")));
	TVVideoMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Test/video_test/NewMediaPlayer_Video_Mat.NewMediaPlayer_Video_Mat")));
	BirdSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/bird.bird")));
	ButterflySequences[0] = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Test/Butter_Fly_Sequence/ButterFly.ButterFly")));
	ButterflySequences[1] = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Test/Butter_Fly_Sequence/ButterFly1.ButterFly1")));
	ButterflySequences[2] = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Test/Butter_Fly_Sequence/ButterFly2.ButterFly2")));
	TVSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Test/Dummy_sequence.Dummy_sequence")));
}

void AKumaChapterOneDirector::BeginPlay()
{
	Super::BeginPlay();

	if (UKumaDialogueSubsystem* DialogueSubsystem = GetDialogueSubsystem())
	{
		DialogueSubsystem->OnDialogueSequenceFinished.AddDynamic(this, &AKumaChapterOneDirector::HandleDialogueSequenceFinished);
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AKumaChapterOneDirector::StartChapterOne);
}

void AKumaChapterOneDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MotherEnterDialogueTimerHandle);
	GetWorldTimerManager().ClearTimer(TVVideoStartTimer);

	if (UKumaDialogueSubsystem* DialogueSubsystem = GetDialogueSubsystem())
	{
		DialogueSubsystem->OnDialogueSequenceFinished.RemoveDynamic(this, &AKumaChapterOneDirector::HandleDialogueSequenceFinished);
	}

	if (HankSequencePlayer)
	{
		HankSequencePlayer->Stop();
	}
	if (ActiveTVMediaPlayer)
	{
		ActiveTVMediaPlayer->OnMediaOpened.RemoveDynamic(this, &AKumaChapterOneDirector::HandleTVMediaOpened);
		ActiveTVMediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &AKumaChapterOneDirector::HandleTVMediaOpenFailed);
		ActiveTVMediaPlayer->OnEndReached.RemoveDynamic(this, &AKumaChapterOneDirector::HandleTVVideoEnded);
		ActiveTVMediaPlayer->Close();
	}
	RestoreTVOriginalMaterial();

	if (MotherEnterSequencePlayer)
	{
		MotherEnterSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterOneDirector::HandleMotherEnterSequenceFinished);
		MotherEnterSequencePlayer->Stop();
	}

	if (MotherExitSequencePlayer)
	{
		MotherExitSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterOneDirector::HandleMotherExitSequenceFinished);
		MotherExitSequencePlayer->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void AKumaChapterOneDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bMiniGame0Searching) return;
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC) return;

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	FVector CameraForward = CameraRotation.Vector();

	// This is a geometry test, not a collision trace: the reticle is the exact
	// centre ray of the camera, and the target is the butterfly bounds centre.
	for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
	{
		AActor* ButterflyActor = *ActorIt;
		if (!IsButterflyHit(ButterflyActor)) continue;

		const FVector ButterflyCenter = ButterflyActor->GetComponentsBoundingBox(true).GetCenter();
		const FVector ToButterfly = ButterflyCenter - CameraLocation;
		const float TargetDepth = FVector::DotProduct(ToButterfly, CameraForward);
		if (TargetDepth <= 0.f) continue;

		const FVector ReticlePointAtTargetDepth = CameraLocation + CameraForward * TargetDepth;
		const float DistanceFromReticleCenter = FVector::Dist(ButterflyCenter, ReticlePointAtTargetDepth);
		if (DistanceFromReticleCenter <= KumaChapterOne::ButterflyReticleTolerance)
		{
			UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] Butterfly aligned. Move=%d Distance=%.2f Tolerance=%.2f"), ButterflyMoveIndex + 1, DistanceFromReticleCenter, KumaChapterOne::ButterflyReticleTolerance);
			HandleButterflyFound();
			return;
		}
	}
}

void AKumaChapterOneDirector::StartChapterOne()
{
	if (bHasStarted)
	{
		return;
	}

	bHasStarted = true;
	SetPlayerArmInputEnabled(false);
	CreateDialogueWidget();
	CreateChapterIntroWidget();
}

void AKumaChapterOneDirector::StartRoomNarration()
{
	if (USoundBase* Bird = BirdSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Bird);
	}
	PlayDialogue(KumaChapterOne::RoomNarration, BuildRoomNarration());
}

void AKumaChapterOneDirector::ScheduleTVVideoAfterFinalButterfly()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TVVideoStartTimer);
	GetWorldTimerManager().SetTimer(TVVideoStartTimer, this, &AKumaChapterOneDirector::StartTVVideoPlayback, 1.f, false);
	UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] Final butterfly found. TV video will start in 1.0 seconds."));
}

void AKumaChapterOneDirector::StartTVVideoPlayback()
{
	UMediaPlayer* MediaPlayer = TVMediaPlayerAsset.LoadSynchronous();
	UFileMediaSource* MediaSource = TVMediaSourceAsset.LoadSynchronous();
	UMaterialInterface* VideoMaterial = TVVideoMaterialAsset.LoadSynchronous();
	if (!MediaPlayer || !MediaSource || !VideoMaterial)
	{
		UE_LOG(LogKumaChapterOne, Error, TEXT("[KumaChapterOne] TV video assets could not be loaded. Player=%s Source=%s Material=%s"), *GetNameSafe(MediaPlayer), *GetNameSafe(MediaSource), *GetNameSafe(VideoMaterial));
		return;
	}

	UStaticMeshComponent* TVMesh = nullptr;
	for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!Actor)
		{
			continue;
		}

		bool bIsTargetTV = Actor->GetName().Contains(TEXT("Dummy_TVStand3"), ESearchCase::IgnoreCase);
#if WITH_EDITOR
		bIsTargetTV |= Actor->GetActorLabel().Equals(TEXT("Dummy_TVStand3"), ESearchCase::IgnoreCase);
#endif
		if (bIsTargetTV)
		{
			TVMesh = Actor->FindComponentByClass<UStaticMeshComponent>();
			break;
		}
	}
	if (!TVMesh || TVMesh->GetNumMaterials() == 0)
	{
		UE_LOG(LogKumaChapterOne, Error, TEXT("[KumaChapterOne] Dummy_TVStand3 or its material slot was not found."));
		return;
	}

	ActiveTVMesh = TVMesh;
	OriginalTVMaterial = ActiveTVMesh->GetMaterial(0);
	ActiveTVMesh->SetMaterial(0, VideoMaterial);
	ActiveTVMediaPlayer = MediaPlayer;
	ActiveTVMediaPlayer->OnMediaOpened.RemoveDynamic(this, &AKumaChapterOneDirector::HandleTVMediaOpened);
	ActiveTVMediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &AKumaChapterOneDirector::HandleTVMediaOpenFailed);
	ActiveTVMediaPlayer->OnEndReached.RemoveDynamic(this, &AKumaChapterOneDirector::HandleTVVideoEnded);
	ActiveTVMediaPlayer->OnMediaOpened.AddDynamic(this, &AKumaChapterOneDirector::HandleTVMediaOpened);
	ActiveTVMediaPlayer->OnMediaOpenFailed.AddDynamic(this, &AKumaChapterOneDirector::HandleTVMediaOpenFailed);
	ActiveTVMediaPlayer->OnEndReached.AddDynamic(this, &AKumaChapterOneDirector::HandleTVVideoEnded);
	ActiveTVMediaPlayer->SetLooping(false);
	ActiveTVMediaPlayer->Close();
	const bool bOpenRequested = ActiveTVMediaPlayer->OpenSource(MediaSource);
	UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] TV video requested 1 second after final butterfly. Actor=Dummy_TVStand3 Slot=0 Source=%s OpenRequested=%s"), *MediaSource->GetPathName(), bOpenRequested ? TEXT("true") : TEXT("false"));
}

void AKumaChapterOneDirector::RestoreTVOriginalMaterial()
{
	if (ActiveTVMesh)
	{
		ActiveTVMesh->SetMaterial(0, OriginalTVMaterial);
		UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] Dummy_TVStand3 material restored after TV video."));
	}

	ActiveTVMesh = nullptr;
	OriginalTVMaterial = nullptr;
}

void AKumaChapterOneDirector::HandleTVMediaOpened(FString OpenedUrl)
{
	if (ActiveTVMediaPlayer)
	{
		ActiveTVMediaPlayer->Play();
	}
	UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] TV video opened and playing: %s"), *OpenedUrl);
}

void AKumaChapterOneDirector::HandleTVMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogKumaChapterOne, Error, TEXT("[KumaChapterOne] TV video failed to open: %s"), *FailedUrl);
	RestoreTVOriginalMaterial();
}

void AKumaChapterOneDirector::HandleTVVideoEnded()
{
	if (ActiveTVMediaPlayer)
	{
		ActiveTVMediaPlayer->Pause();
	}
	RestoreTVOriginalMaterial();
}

void AKumaChapterOneDirector::StartHankCall()
{
	HankSequencePlayer = CreateSequencePlayer(HankSequence, true, HankSequenceActor);
	if (HankSequencePlayer)
	{
		HankSequencePlayer->PlayLooping(-1);
	}

	PlayDialogue(KumaChapterOne::HankCall, BuildHankDialogue());
}

void AKumaChapterOneDirector::StartMotherEnter()
{
	bMotherEnterSequenceFinished = false;
	bMotherEnterDialogueFinished = false;
	MotherEnterSequencePlayer = CreateSequencePlayer(MotherEnterSequence, false, MotherEnterSequenceActor);
	if (!MotherEnterSequencePlayer)
	{
		UE_LOG(LogKumaChapterOne, Warning, TEXT("[KumaChapterOne] Mother entrance sequence could not start. Continuing with the timed introduction dialogue."));
		bMotherEnterSequenceFinished = true;
		GetWorldTimerManager().SetTimer(MotherEnterDialogueTimerHandle, this, &AKumaChapterOneDirector::StartMotherEnterDialogue, MotherEnterDialogueDelaySeconds, false);
		return;
	}

	MotherEnterSequencePlayer->OnFinished.AddDynamic(this, &AKumaChapterOneDirector::HandleMotherEnterSequenceFinished);
	MotherEnterSequencePlayer->SetCompletionModeOverride(EMovieSceneCompletionModeOverride::ForceKeepState);
	MotherEnterSequencePlayer->Play();
	GetWorldTimerManager().SetTimer(MotherEnterDialogueTimerHandle, this, &AKumaChapterOneDirector::StartMotherEnterDialogue, MotherEnterDialogueDelaySeconds, false);
}

void AKumaChapterOneDirector::StartMotherEnterDialogue()
{
	PlayDialogue(KumaChapterOne::MotherEnter, BuildMotherEnterDialogue());
}

void AKumaChapterOneDirector::StartMotherConversation()
{
	PlayDialogue(KumaChapterOne::MotherConversation, BuildMotherConversation());
}

void AKumaChapterOneDirector::StartMotherExit()
{
	bMotherExitSequenceFinished = false;
	bMotherExitDialogueFinished = false;
	MotherExitSequencePlayer = CreateSequencePlayer(MotherExitSequence, false, MotherExitSequenceActor);
	if (!MotherExitSequencePlayer)
	{
		UE_LOG(LogKumaChapterOne, Warning, TEXT("[KumaChapterOne] Mother exit sequence could not start. Starting the exit dialogue immediately."));
		bMotherExitSequenceFinished = true;
		PlayDialogue(KumaChapterOne::MotherExit, BuildMotherExitDialogue());
		return;
	}

	MotherExitSequencePlayer->OnFinished.AddDynamic(this, &AKumaChapterOneDirector::HandleMotherExitSequenceFinished);
	MotherExitSequencePlayer->SetCompletionModeOverride(EMovieSceneCompletionModeOverride::ForceKeepState);
	MotherExitSequencePlayer->Play();
}

void AKumaChapterOneDirector::FinishBeforeMiniGame0()
{
	if (bHasFinished)
	{
		return;
	}

	bHasFinished = true;
	UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] Chapter 1 cinematic is complete. MiniGame 0 is ready to start."));
	OnChapterOneReadyForMiniGame0.Broadcast();
	StartMiniGame0();
}

void AKumaChapterOneDirector::TryFinishBeforeMiniGame0()
{
	if (bMotherExitSequenceFinished && bMotherExitDialogueFinished)
	{
		FinishBeforeMiniGame0();
	}
}

void AKumaChapterOneDirector::CreateDialogueWidget()
{
	if (DialogueWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogKumaChapterOne, Warning, TEXT("[KumaChapterOne] Dialogue box could not be created because no player controller was found."));
		return;
	}

	DialogueWidget = CreateWidget<UKumaDialogueBoxWidget>(PlayerController, UKumaDialogueBoxWidget::StaticClass());
	if (DialogueWidget)
	{
		DialogueWidget->AddToViewport(10);
		if (UKumaDialogueSubsystem* DialogueSubsystem = GetDialogueSubsystem())
		{
			DialogueSubsystem->RegisterDialogueWidget(DialogueWidget);
		}
		else
		{
			DialogueWidget->SetVisibility(ESlateVisibility::Collapsed);
			UE_LOG(LogKumaChapterOne, Warning, TEXT("[KumaChapterOne] Dialogue widget was created, but the dialogue subsystem was unavailable."));
		}
	}
}

void AKumaChapterOneDirector::CreateChapterIntroWidget()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogKumaChapterOne, Warning, TEXT("[KumaChapterOne] No player controller was found. Skipping the chapter title UI."));
		StartRoomNarration();
		return;
	}

	ChapterIntroWidget = CreateWidget<UKumaChapterIntroWidget>(PlayerController, UKumaChapterIntroWidget::StaticClass());
	if (!ChapterIntroWidget)
	{
		UE_LOG(LogKumaChapterOne, Warning, TEXT("[KumaChapterOne] Chapter title UI could not be created. Skipping directly to narration."));
		StartRoomNarration();
		return;
	}

	ChapterIntroWidget->SetChapterInfo(FText::FromString(TEXT("CHAPTER 1")), FText::FromString(TEXT("My Home")));
	ChapterIntroWidget->OnIntroFinished.AddUObject(this, &AKumaChapterOneDirector::StartRoomNarration);
	ChapterIntroWidget->AddToViewport(100);
	ChapterIntroWidget->PlayIntro(IntroHoldSeconds, IntroFadeSeconds);
}

void AKumaChapterOneDirector::SetPlayerArmInputEnabled(bool bEnabled) const
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UArmControlComponent* ArmControl = PlayerPawn ? PlayerPawn->FindComponentByClass<UArmControlComponent>() : nullptr;
	if (!ArmControl)
	{
		UE_LOG(LogKumaChapterOne, Warning, TEXT("[KumaChapterOne] Player arm control was not found. Arm input was not changed."));
		return;
	}

	ArmControl->SetArmInputEnabled(bEnabled);
}

ULevelSequencePlayer* AKumaChapterOneDirector::CreateSequencePlayer(const TSoftObjectPtr<ULevelSequence>& SequenceAsset, bool bLooping, TObjectPtr<ALevelSequenceActor>& OutSequenceActor, bool bPauseAtEnd) const
{
	OutSequenceActor = nullptr;
	UWorld* World = GetWorld();
	ULevelSequence* LoadedSequence = SequenceAsset.LoadSynchronous();
	if (!World || !LoadedSequence)
	{
		UE_LOG(LogKumaChapterOne, Error, TEXT("[KumaChapterOne] Failed to load a required Level Sequence: %s"), *SequenceAsset.ToSoftObjectPath().ToString());
		return nullptr;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.LoopCount.Value = bLooping ? -1 : 0;
	PlaybackSettings.bPauseAtEnd = bPauseAtEnd;
	ALevelSequenceActor* CreatedSequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LoadedSequence, PlaybackSettings, CreatedSequenceActor);
	OutSequenceActor = CreatedSequenceActor;
	if (!SequencePlayer || !OutSequenceActor)
	{
		UE_LOG(LogKumaChapterOne, Error, TEXT("[KumaChapterOne] Failed to create a playback actor for Level Sequence: %s"), *LoadedSequence->GetPathName());
		return nullptr;
	}

	OutSequenceActor->SetReplicatePlayback(false);
	SequencePlayer->SetDisableCameraCuts(false);
	UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] Created sequence player. Sequence=%s Actor=%s Looping=%s"), *LoadedSequence->GetPathName(), *OutSequenceActor->GetName(), bLooping ? TEXT("true") : TEXT("false"));
	return SequencePlayer;
}

void AKumaChapterOneDirector::PlayDialogue(FName OwnerId, const TArray<FKumaDialogueLine>& Lines) const
{
	if (UKumaDialogueSubsystem* DialogueSubsystem = GetDialogueSubsystem())
	{
		DialogueSubsystem->PlayDialogueSequence(OwnerId, Lines);
		return;
	}

	UE_LOG(LogKumaChapterOne, Error, TEXT("[KumaChapterOne] Dialogue subsystem is unavailable. Owner=%s"), *OwnerId.ToString());
}

FKumaDialogueLine AKumaChapterOneDirector::MakeAutoLine(FName LineId, FName SpeakerId, const TCHAR* Text) const
{
	FKumaDialogueLine Line;
	Line.LineId = LineId;
	Line.SpeakerId = SpeakerId;
	Line.Text = FText::FromString(Text);
	Line.Options.TypingSpeed = TypingCharactersPerSecond;
	Line.Options.AdvancePolicy = EKumaDialogueAdvancePolicy::AutoNext;
	Line.Options.AutoAdvanceDelay = AutoAdvanceDelaySeconds;
	Line.Options.bAllowSkipTyping = false;
	Line.Options.bCloseWhenFinished = true;
	Line.Options.bBlockGameplayInput = true;
	return Line;
}

UKumaDialogueSubsystem* AKumaChapterOneDirector::GetDialogueSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UKumaDialogueSubsystem>();
	}

	return nullptr;
}

TArray<FKumaDialogueLine> AKumaChapterOneDirector::BuildRoomNarration() const
{
	return {
		MakeAutoLine(TEXT("CH01_ROOM_01"), KumaChapterOne::Tomas, TEXT("Birds are chirping.")),
		MakeAutoLine(TEXT("CH01_ROOM_02"), KumaChapterOne::Tomas, TEXT("It sounds like Mother is cooking downstairs.")),
		MakeAutoLine(TEXT("CH01_ROOM_03"), KumaChapterOne::Tomas, TEXT("A normal afternoon. Dull and repetitive."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterOneDirector::BuildHankDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH01_HANK_01"), KumaChapterOne::Hank, TEXT("Hello? Tomas! It's Uncle Hank. How are you doing?")),
		MakeAutoLine(TEXT("CH01_HANK_02"), KumaChapterOne::Hank, TEXT("Guess that's not quite the right question.. But I heard you've been doing better lately.")),
		MakeAutoLine(TEXT("CH01_HANK_03"), KumaChapterOne::Hank, TEXT("Annie's sounding much better too. She's been asking me all sorts of things about military medical equipment.")),
		MakeAutoLine(TEXT("CH01_HANK_04"), KumaChapterOne::Hank, TEXT("You know, she's had it rough since the fever.")),
		MakeAutoLine(TEXT("CH01_HANK_05"), KumaChapterOne::Tomas, TEXT("The fever. The reason I can't move. In a rural town like this, with the war on top of it — medicine couldn't reach us in time. That was just how it was.")),
		MakeAutoLine(TEXT("CH01_HANK_06"), KumaChapterOne::Tomas, TEXT("In the summer of 1942, when I was three years old, the fever took my father.")),
		MakeAutoLine(TEXT("CH01_HANK_07"), KumaChapterOne::Tomas, TEXT("Then in the fall of that same year, it came for me. I was lucky enough to survive. But after that, I couldn't move anything.")),
		MakeAutoLine(TEXT("CH01_HANK_08"), KumaChapterOne::Hank, TEXT("Is the answering machine working alright? Can't check from out here.")),
		MakeAutoLine(TEXT("CH01_HANK_09"), KumaChapterOne::Hank, TEXT("Your old uncle is working on a new route. Asia's definitely the land of opportunity after the war.")),
		MakeAutoLine(TEXT("CH01_HANK_10"), KumaChapterOne::Hank, TEXT("Once this route is up and running, there'll be a massive flow of cargo, and every merchant in the world will know my name.")),
		MakeAutoLine(TEXT("CH01_HANK_11"), KumaChapterOne::Hank, TEXT("I've got workers lined up to blast a tunnel through a mountain near the docks. The bedrock's solid, but enough dynamite should do the trick.")),
		MakeAutoLine(TEXT("CH01_HANK_12"), KumaChapterOne::Hank, TEXT("Oops — got carried away again.")),
		MakeAutoLine(TEXT("CH01_HANK_13"), KumaChapterOne::Hank, TEXT("Anyway, once this deal's sorted, I'm thinking of coming back home.")),
		MakeAutoLine(TEXT("CH01_HANK_14"), KumaChapterOne::Hank, TEXT("I'll have enough money to take care of you and Annie for the rest of your lives.")),
		MakeAutoLine(TEXT("CH01_HANK_15"), KumaChapterOne::Hank, TEXT("Alright, back to work. Always rooting for you, Tomas. Take care."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterOneDirector::BuildMotherEnterDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH01_MOTHER_ENTER_01"), KumaChapterOne::Tomas, TEXT("My mother. Annie Caldwell."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterOneDirector::BuildMotherConversation() const
{
	return {
		MakeAutoLine(TEXT("CH01_MOTHER_01"), KumaChapterOne::Tomas, TEXT("My mother. Annie Caldwell.")),
		MakeAutoLine(TEXT("CH01_MOTHER_02"), KumaChapterOne::Annie, TEXT("Oh sweetie, are you awake? Are you feeling alright?")),
		MakeAutoLine(TEXT("CH01_MOTHER_03"), KumaChapterOne::Annie, TEXT("I'm thinking of going to church today.")),
		MakeAutoLine(TEXT("CH01_MOTHER_04"), KumaChapterOne::Annie, TEXT("I baked some cookies for Father Joseph.")),
		MakeAutoLine(TEXT("CH01_MOTHER_05"), KumaChapterOne::Tomas, TEXT("She's talking about Father Joseph. He's a good man. He came every Wednesday to teach me English, since I couldn't go to school.")),
		MakeAutoLine(TEXT("CH01_MOTHER_06"), KumaChapterOne::Tomas, TEXT("Not that I believe in God. If there is one, He clearly has a grudge against our family.")),
		MakeAutoLine(TEXT("CH01_MOTHER_07"), KumaChapterOne::Annie, TEXT("I'll leave the light on, sweetie. I'll be back before sundown — don't you worry."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterOneDirector::BuildMotherExitDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH01_EXIT_01"), KumaChapterOne::Tomas, TEXT("Alone again.")),
		MakeAutoLine(TEXT("CH01_EXIT_02"), KumaChapterOne::Tomas, TEXT("It's natural for any countryside to have little entertainment. For me, there's even less.")),
		MakeAutoLine(TEXT("CH01_EXIT_03"), KumaChapterOne::Tomas, TEXT("This is the 'staring' game I've played hundreds of times.")),
		MakeAutoLine(TEXT("CH01_EXIT_04"), KumaChapterOne::Tomas, TEXT("I move my eyes to trace anything that moves. Something no one else would ever do for fun. For me, it was exercise, a game, and work all at once."))
	};
}

void AKumaChapterOneDirector::HandleDialogueSequenceFinished(FName OwnerId, FName LastLineId)
{
	if (OwnerId == KumaChapterOne::RoomNarration)
	{
		StartHankCall();
		return;
	}

	if (OwnerId == KumaChapterOne::HankCall)
	{
		if (HankSequencePlayer)
		{
			HankSequencePlayer->Stop();
		}

		StartMotherEnter();
		return;
	}

	if (OwnerId == KumaChapterOne::MotherEnter)
	{
		bMotherEnterDialogueFinished = true;
		TryStartMotherConversation();
		return;
	}

	if (OwnerId == KumaChapterOne::MotherConversation)
	{
		StartMotherExit();
		return;
	}

	if (OwnerId == KumaChapterOne::MotherExit)
	{
		bMotherExitDialogueFinished = true;
		TryFinishBeforeMiniGame0();
		return;
	}

	if (OwnerId == KumaChapterOne::MiniGameFirstFound)
	{
		PlayButterflyMove(1);
		return;
	}

	if (OwnerId == KumaChapterOne::MiniGameSecondFound)
	{
		PlayButterflyMove(2);
		return;
	}

	if (OwnerId == KumaChapterOne::MiniGameEnd)
	{
		ShowChapterEndMenu();
	}
}

void AKumaChapterOneDirector::HandleMotherEnterSequenceFinished()
{
	if (MotherEnterSequencePlayer)
	{
		MotherEnterSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterOneDirector::HandleMotherEnterSequenceFinished);
	}

	bMotherEnterSequenceFinished = true;
	TryStartMotherConversation();
}

void AKumaChapterOneDirector::TryStartMotherConversation()
{
	if (bMotherEnterSequenceFinished && bMotherEnterDialogueFinished)
	{
		StartMotherConversation();
	}
}

void AKumaChapterOneDirector::HandleMotherExitSequenceFinished()
{
	if (MotherExitSequencePlayer)
	{
		MotherExitSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterOneDirector::HandleMotherExitSequenceFinished);
	}

	bMotherExitSequenceFinished = true;
	PlayDialogue(KumaChapterOne::MotherExit, BuildMotherExitDialogue());
}

void AKumaChapterOneDirector::StartMiniGame0()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC) return;
	SetPlayerArmInputEnabled(false);
	PC->SetIgnoreMoveInput(true);
	SetMiniGameLookInput(false);
	MiniGame0Widget = CreateWidget<UKumaMiniGame0Widget>(PC, UKumaMiniGame0Widget::StaticClass());
	if (MiniGame0Widget) { MiniGame0Widget->AddToViewport(500); MiniGame0Widget->BeginMiniGame(); }
	PlayButterflyMove(0);
}

void AKumaChapterOneDirector::PlayButterflyMove(int32 MoveIndex)
{
	if (!ButterflySequences[MoveIndex].ToSoftObjectPath().IsValid()) return;
	bMiniGame0Searching = false;
	SetMiniGameLookInput(false);
	if (ButterflySequencePlayer && ButterflySequencePlayer->IsPaused())
	{
		// The previous Spawnable stays visible while searching. Remove it only
		// once the player has found it and the next butterfly move is beginning.
		ButterflySequencePlayer->Stop();
	}
	ButterflyMoveIndex = MoveIndex;
	ButterflySequencePlayer = CreateSequencePlayer(ButterflySequences[MoveIndex], false, ButterflySequenceActor, true);
	if (!ButterflySequencePlayer) { BeginButterflySearch(); return; }
	// The butterfly should move, but its sequence must never take over the
	// fixed player camera used for the look mini-game.
	ButterflySequencePlayer->SetDisableCameraCuts(true);
	// Do not restore a butterfly's transform when a move reaches its last frame.
	// This applies to all three ButterflySequences entries.
	ButterflySequenceActor->PlaybackSettings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceKeepState;
	ButterflySequencePlayer->SetCompletionModeOverride(EMovieSceneCompletionModeOverride::ForceKeepState);
	ButterflySequencePlayer->OnFinished.AddDynamic(this, &AKumaChapterOneDirector::HandleButterflySequenceFinished);
	ButterflySequencePlayer->Play();
}

void AKumaChapterOneDirector::HandleButterflySequenceFinished()
{
	if (ButterflySequencePlayer) ButterflySequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterOneDirector::HandleButterflySequenceFinished);

	bool bButterflyRetained = false;
	for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
	{
		AActor* ButterflyActor = *ActorIt;
		if (IsButterflyHit(ButterflyActor))
		{
			bButterflyRetained = true;
			UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] Butterfly retained after move %d at %s"), ButterflyMoveIndex + 1, *ButterflyActor->GetActorLocation().ToCompactString());
		}
	}
	if (!bButterflyRetained)
	{
		UE_LOG(LogKumaChapterOne, Warning, TEXT("[KumaChapterOne] No retained butterfly actor was found after move %d."), ButterflyMoveIndex + 1);
	}
	BeginButterflySearch();
}

void AKumaChapterOneDirector::BeginButterflySearch()
{
	bMiniGame0Searching = true;
	SetMiniGameLookInput(true);
}

bool AKumaChapterOneDirector::IsButterflyHit(AActor* HitActor) const
{
	if (!HitActor) return false;
	if (HitActor->GetName().Contains(TEXT("Butterfly"), ESearchCase::IgnoreCase)) return true;
	if (const USkeletalMeshComponent* MeshComponent = HitActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (const USkeletalMesh* Mesh = MeshComponent->GetSkeletalMeshAsset()) return Mesh->GetName().Contains(TEXT("Butterfly"), ESearchCase::IgnoreCase);
	}
	return false;
}

void AKumaChapterOneDirector::HandleButterflyFound()
{
	bMiniGame0Searching = false;
	SetMiniGameLookInput(false);
	if (ButterflyMoveIndex == 0)
	{
		TArray<FKumaDialogueLine> Lines;
		Lines.Add(MakeAutoLine(TEXT("MG0_01"), KumaChapterOne::Tomas, TEXT("There you are, butterfly.")));
		Lines.Add(MakeAutoLine(TEXT("MG0_02"), KumaChapterOne::Tomas, TEXT("Where are you off to this time?")));
		PlayDialogue(KumaChapterOne::MiniGameFirstFound, Lines);
	}
	else if (ButterflyMoveIndex == 1)
	{
		TArray<FKumaDialogueLine> Lines;
		Lines.Add(MakeAutoLine(TEXT("MG0_03"), KumaChapterOne::Tomas, TEXT("That's not the sun.")));
		PlayDialogue(KumaChapterOne::MiniGameSecondFound, Lines);
	}
	else
	{
		if (UKumaGameInstance* GameInstance = GetGameInstance<UKumaGameInstance>())
		{
			GameInstance->StopChapterAmbience();
		}
		ScheduleTVVideoAfterFinalButterfly();
		PlayTVSequence();
	}
}

void AKumaChapterOneDirector::PlayTVSequence()
{
	SetMiniGameLookInput(false);
	TVSequencePlayer = CreateSequencePlayer(TVSequence, false, TVSequenceActor);
	if (!TVSequencePlayer) { HandleTVSequenceFinished(); return; }
	TVSequencePlayer->OnFinished.AddDynamic(this, &AKumaChapterOneDirector::HandleTVSequenceFinished);
	TVSequencePlayer->Play();
}

void AKumaChapterOneDirector::HandleTVSequenceFinished()
{
	if (TVSequencePlayer) TVSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterOneDirector::HandleTVSequenceFinished);
	TArray<FKumaDialogueLine> Lines;
	Lines.Add(MakeAutoLine(TEXT("MG0_04"), KumaChapterOne::Tomas, TEXT("Whoa! Is it broken?")));
	Lines.Add(MakeAutoLine(TEXT("MG0_05"), KumaChapterOne::Tomas, TEXT("My eyes are sore. That's enough for today.")));
	Lines.Add(MakeAutoLine(TEXT("MG0_06"), KumaChapterOne::Tomas, TEXT("When there's nothing else to do, I sleep. If I stay awake any longer, even existing starts to feel like torment.")));
	if (MiniGame0Widget) MiniGame0Widget->EndMiniGame();
	PlayDialogue(KumaChapterOne::MiniGameEnd, Lines);
}

void AKumaChapterOneDirector::ShowChapterEndMenu()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC) return;
	ChapterEndWidget = CreateWidget<UKumaChapterEndWidget>(PC, UKumaChapterEndWidget::StaticClass());
	if (!ChapterEndWidget) return;
	ChapterEndWidget->AddToViewport(1000);
	PC->SetIgnoreLookInput(true);
	PC->SetIgnoreMoveInput(true);
	PC->SetShowMouseCursor(true);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ChapterEndWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	ChapterEndWidget->OnNextChapterClicked.AddLambda([this]() { if (UKumaGameInstance* GI = GetGameInstance<UKumaGameInstance>()) GI->OpenChapterTwo(); });
	ChapterEndWidget->OnMainMenuClicked.AddLambda([this]() { UGameplayStatics::OpenLevel(this, FName(TEXT("StartScene"))); });
	ChapterEndWidget->OnQuitClicked.AddLambda([PC]() { UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false); });
}

void AKumaChapterOneDirector::SetMiniGameLookInput(bool bEnabled) const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC) return;
	PC->ResetIgnoreLookInput();
	PC->SetIgnoreLookInput(!bEnabled);
	if (bEnabled && PC->GetPawn())
	{
		// Butterfly sequences can leave the controller viewing a cinematic camera.
		// Restore the player pawn camera before allowing free look.
		PC->SetViewTargetWithBlend(PC->GetPawn(), 0.f);
	}
	if (AAMPPlayerCharacter* PlayerCharacter = Cast<AAMPPlayerCharacter>(PC->GetPawn()))
	{
		PlayerCharacter->SetMiniGameLookCameraEnabled(bEnabled);
	}
	PC->SetShowMouseCursor(false);
	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
	UE_LOG(LogKumaChapterOne, Log, TEXT("[KumaChapterOne] MiniGame look allowed=%s ignored=%s controller=%s pawn=%s"), bEnabled ? TEXT("true") : TEXT("false"), PC->IsLookInputIgnored() ? TEXT("true") : TEXT("false"), *GetNameSafe(PC), *GetNameSafe(PC->GetPawn()));
}
