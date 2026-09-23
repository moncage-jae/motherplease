// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaChapterTwoDirector.h"

#include "AMPPlayerCharacter.h"
#include "ArmControlComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KumaChapterEndWidget.h"
#include "KumaGameInstance.h"
#include "KumaDialogueBoxWidget.h"
#include "KumaInputRouterComponent.h"
#include "KumaMiniGame1CameraShake.h"
#include "KumaMiniGame1Widget.h"
#include "InputCoreTypes.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "MovieScene.h"
#include "MovieSceneObjectBindingID.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKumaChapterTwo, Log, All);

namespace KumaChapterTwo
{
	const FName Opening(TEXT("CH02_OPENING"));
	const FName HankCall(TEXT("CH02_HANK_CALL"));
	const FName PostHank(TEXT("CH02_POST_HANK"));
	const FName MotherDialogue(TEXT("CH02_MOTHER_DIALOGUE"));
	const FName MotherMachine(TEXT("CH02_MOTHER_MACHINE"));
	const FName PostMachine(TEXT("CH02_POST_MACHINE"));
	const FName HeatNarration(TEXT("CH02_HEAT_NARRATION"));
	const FName MiniGame1FirstMove(TEXT("CH02_MG1_FIRST_MOVE"));
	const FName MiniGame1SecondMove(TEXT("CH02_MG1_SECOND_MOVE"));
	const FName MiniGame1Success(TEXT("CH02_MG1_SUCCESS"));
	const FName MiniGame1Failure(TEXT("CH02_MG1_FAILURE"));
	const FName MiniGame1SoundStart(TEXT("CH02_HEAT_05"));

	const FName Tomas(TEXT("tomas"));
	const FName Hank(TEXT("hank"));
	const FName Annie(TEXT("annie"));

	bool IsActorBoundToName(const AActor* Actor, const FString& BindingName)
	{
		if (!Actor)
		{
			return false;
		}

		auto MatchesBindingName = [&BindingName](const FString& CandidateName)
		{
			return CandidateName.Equals(BindingName, ESearchCase::IgnoreCase)
				|| CandidateName.Contains(BindingName, ESearchCase::IgnoreCase);
		};

		if (MatchesBindingName(Actor->GetName()))
		{
			return true;
		}
#if WITH_EDITOR
		if (MatchesBindingName(Actor->GetActorLabel()))
		{
			return true;
		}
#endif

		if (const USkeletalMeshComponent* SkeletalMeshComponent = Actor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
			{
				if (MatchesBindingName(SkeletalMesh->GetName())
					|| (BindingName.Contains(TEXT("SkeletalMeshActor"), ESearchCase::IgnoreCase)
						&& SkeletalMesh->GetName().Equals(TEXT("SM_Annie"), ESearchCase::IgnoreCase)))
				{
					return true;
				}
			}
		}

		if (const UStaticMeshComponent* StaticMeshComponent = Actor->FindComponentByClass<UStaticMeshComponent>())
		{
			if (const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
			{
				return MatchesBindingName(StaticMesh->GetName());
			}
		}

		return false;
	}

	AActor* FindActorByStaticMeshAssetName(UWorld* World, const FName StaticMeshAssetName)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			AActor* Actor = *ActorIt;
			const UStaticMeshComponent* StaticMeshComponent = Actor ? Actor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
			const UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
			if (StaticMesh && StaticMesh->GetFName().IsEqual(StaticMeshAssetName, ENameCase::IgnoreCase))
			{
				return Actor;
			}
		}

		return nullptr;
	}

	USkeletalMeshComponent* FindCTestMotherMeshComponent(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			USkeletalMeshComponent* SkeletalMeshComponent = ActorIt->FindComponentByClass<USkeletalMeshComponent>();
			const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent ? SkeletalMeshComponent->GetSkeletalMeshAsset() : nullptr;
			if (SkeletalMesh && SkeletalMesh->GetFName().IsEqual(TEXT("SM_Annie"), ENameCase::IgnoreCase))
			{
				return SkeletalMeshComponent;
			}
		}

		return nullptr;
	}
}

AKumaChapterTwoDirector::AKumaChapterTwoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	HankSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Test/videotape_sequence1.videotape_sequence1")));
	MotherEnterSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Characters/Mother_ANI/Comein.Comein")));
	MotherMachineSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Characters/Mother_ANI/playmachine1.playmachine1")));
	MotherExitSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Characters/Mother_ANI/goout.goout")));
	MotherMiniGame1EndSequence = TSoftObjectPtr<ULevelSequence>(FSoftObjectPath(TEXT("/Game/Characters/Mother_ANI/minigame1end1.minigame1end1")));
	WorkerSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/worker.worker")));
	MiniGame1Sound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/minigame1.minigame1")));
}

void AKumaChapterTwoDirector::BeginPlay()
{
	Super::BeginPlay();

	if (UKumaDialogueSubsystem* DialogueSubsystem = GetDialogueSubsystem())
	{
		DialogueSubsystem->OnDialogueSequenceFinished.AddDynamic(this, &AKumaChapterTwoDirector::HandleDialogueSequenceFinished);
		DialogueSubsystem->OnDialogueLineStarted.AddDynamic(this, &AKumaChapterTwoDirector::HandleDialogueLineStarted);
	}
}

void AKumaChapterTwoDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UKumaDialogueSubsystem* DialogueSubsystem = GetDialogueSubsystem())
	{
		DialogueSubsystem->OnDialogueSequenceFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleDialogueSequenceFinished);
		DialogueSubsystem->OnDialogueLineStarted.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleDialogueLineStarted);
	}
	StopMiniGame1Sound();

	if (HankSequencePlayer) HankSequencePlayer->Stop();
	if (MotherEnterSequencePlayer)
	{
		MotherEnterSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMotherEnterSequenceFinished);
		MotherEnterSequencePlayer->Stop();
	}
	if (MotherMachineSequencePlayer)
	{
		MotherMachineSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMotherMachineSequenceFinished);
		MotherMachineSequencePlayer->Stop();
	}
	if (MotherExitSequencePlayer)
	{
		MotherExitSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMotherExitSequenceFinished);
		MotherExitSequencePlayer->Stop();
	}
	if (MotherMiniGame1EndSequencePlayer)
	{
		MotherMiniGame1EndSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1EndSequenceFinished);
		MotherMiniGame1EndSequencePlayer->Stop();
	}
	if (MiniGame1InputRouter)
	{
		MiniGame1InputRouter->OnDirectionInputRouted.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1DirectionInput);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MiniGame1RestartTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void AKumaChapterTwoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PollMiniGame1DirectionKeys();

	if (MiniGame1Phase != EKumaMiniGame1Phase::ButtonCountdown)
	{
		return;
	}

	MiniGame1RemainingSeconds -= DeltaSeconds;
	if (MiniGame1Widget)
	{
		MiniGame1Widget->SetCountdownSeconds(MiniGame1RemainingSeconds);
	}

	if (MiniGame1RemainingSeconds <= 0.f)
	{
		StartMiniGame1Failure();
		return;
	}

	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		PlayerController && PlayerController->WasInputKeyJustPressed(EKeys::E) && IsMiniGame1ButtonInteractionAvailable())
	{
		StartMiniGame1Success();
	}
}

void AKumaChapterTwoDirector::StartChapterTwo()
{
	if (bHasStarted)
	{
		return;
	}

#if !UE_BUILD_SHIPPING
	// Temporary runtime verification path. It is available only to a local
	// development build and is removed after the automated input check.
	if (FParse::Param(FCommandLine::Get(), TEXT("KumaTestMiniGame1")))
	{
		bHasStarted = true;
		StartMiniGame1();
		return;
	}
#endif

	bHasStarted = true;
	SetPlayerArmInputEnabled(false);
	CreateDialogueWidget();
	StartOpeningNarration();
}

void AKumaChapterTwoDirector::StartOpeningNarration()
{
	if (USoundBase* Worker = WorkerSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Worker);
	}
	else
	{
		UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] Could not load the Chapter 2 worker sound: %s"), *WorkerSound.ToSoftObjectPath().ToString());
	}

	PlayDialogue(KumaChapterTwo::Opening, BuildOpeningDialogue());
}

void AKumaChapterTwoDirector::StartHankCall()
{
	HankSequencePlayer = CreateSequencePlayer(HankSequence, true, HankSequenceActor);
	if (HankSequencePlayer)
	{
		HankSequencePlayer->PlayLooping(-1);
	}

	PlayDialogue(KumaChapterTwo::HankCall, BuildHankDialogue());
}

void AKumaChapterTwoDirector::StartPostHankNarration()
{
	PlayDialogue(KumaChapterTwo::PostHank, BuildPostHankDialogue());
}

void AKumaChapterTwoDirector::StartMotherEnter()
{
	MotherEnterSequencePlayer = CreateSequencePlayer(MotherEnterSequence, false, MotherEnterSequenceActor);
	if (!MotherEnterSequencePlayer)
	{
		UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] Mother entrance sequence could not start. Continuing with dialogue."));
		StartMotherDialogue();
		return;
	}

	MotherEnterSequencePlayer->OnFinished.AddDynamic(this, &AKumaChapterTwoDirector::HandleMotherEnterSequenceFinished);
	MotherEnterSequencePlayer->Play();
}

void AKumaChapterTwoDirector::StartMotherDialogue()
{
	PlayDialogue(KumaChapterTwo::MotherDialogue, BuildMotherDialogue());
}

void AKumaChapterTwoDirector::StartMotherMachineTreatment()
{
	bMotherMachineSequenceFinished = false;
	bMotherMachineDialogueFinished = false;
	bPostMachineDialogueStarted = false;
	MotherMachineSequencePlayer = CreateSequencePlayer(MotherMachineSequence, false, MotherMachineSequenceActor);
	if (MotherMachineSequencePlayer)
	{
		MotherMachineSequencePlayer->OnFinished.AddDynamic(this, &AKumaChapterTwoDirector::HandleMotherMachineSequenceFinished);
		MotherMachineSequencePlayer->Play();
	}
	else
	{
		UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] Mother machine sequence could not start. Continuing when its dialogue ends."));
		bMotherMachineSequenceFinished = true;
	}

	// Required simultaneous beat: dialogue starts immediately after playmachine1 begins.
	PlayDialogue(KumaChapterTwo::MotherMachine, BuildMachineDialogue());
}

void AKumaChapterTwoDirector::TryStartPostMachineDialogue()
{
	if (!bPostMachineDialogueStarted && bMotherMachineSequenceFinished && bMotherMachineDialogueFinished)
	{
		bPostMachineDialogueStarted = true;
		StartPostMachineDialogue();
	}
}

void AKumaChapterTwoDirector::StartPostMachineDialogue()
{
	PlayDialogue(KumaChapterTwo::PostMachine, BuildPostMachineDialogue());
}

void AKumaChapterTwoDirector::StartMotherExit()
{
	// playmachine1 freezes Annie's skeletal mesh at its exact final sequencer
	// pose. Turn ticking back on before the next sequence takes control.
	if (USkeletalMeshComponent* MotherMeshComponent = KumaChapterTwo::FindCTestMotherMeshComponent(GetWorld()))
	{
		MotherMeshComponent->SetComponentTickEnabled(true);
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] Re-enabled Annie skeletal ticking for goout."));
	}

	MotherExitSequencePlayer = CreateSequencePlayer(MotherExitSequence, false, MotherExitSequenceActor);
	if (!MotherExitSequencePlayer)
	{
		UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] Mother exit sequence could not start. Continuing with heat narration."));
		StartHeatNarration();
		return;
	}

	MotherExitSequencePlayer->OnFinished.AddDynamic(this, &AKumaChapterTwoDirector::HandleMotherExitSequenceFinished);
	MotherExitSequencePlayer->Play();
}

void AKumaChapterTwoDirector::StartHeatNarration()
{
	PlayDialogue(KumaChapterTwo::HeatNarration, BuildHeatDialogue());
}

void AKumaChapterTwoDirector::FinishBeforeMiniGame1()
{
	if (bHasFinished)
	{
		return;
	}

	bHasFinished = true;
	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] Story content complete. MiniGame 1 is ready to begin."));
	OnChapterTwoReadyForMiniGame1.Broadcast();
	StartMiniGame1();
}

void AKumaChapterTwoDirector::StartMiniGame1()
{
	// BP_Arm already owns the player's direction-key movement. MiniGame 1 must
	// observe that existing input, not lock or replace it, so Tomas can first
	// demonstrate that the arm has begun to move.
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerController)
	{
		// Keep the camera and the character root still. Raw arrow-key state still
		// reaches BP_Arm's router, which is the only movement MiniGame 1 needs.
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
		PlayerController->SetShowMouseCursor(false);
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		if (PlayerPawn)
		{
			PlayerController->SetViewTargetWithBlend(PlayerPawn, 0.f);
		}
		UE_LOG(LogKumaChapterTwo, Log,
			TEXT("[KumaChapterTwo] MiniGame 1 forced GameOnly input. Camera/root movement locked; BP_Arm remains routable. Controller=%s Pawn=%s"),
			*GetNameSafe(PlayerController), *GetNameSafe(PlayerPawn));
	}

	BindMiniGame1InputRouter();
	if (UArmControlComponent* ArmControl = PlayerPawn ? PlayerPawn->FindComponentByClass<UArmControlComponent>() : nullptr)
	{
		ArmControl->ResetHandTarget();
	}
	SetPlayerArmInputEnabled(false);
	MiniGame1RemainingSeconds = 60.f;
	bMiniGame1DirectionKeyWasDown = false;
	MiniGame1Phase = EKumaMiniGame1Phase::WaitingForFirstPrompt;

	if (!MiniGame1Widget && PlayerController)
	{
		MiniGame1Widget = CreateWidget<UKumaMiniGame1Widget>(PlayerController, UKumaMiniGame1Widget::StaticClass());
		if (MiniGame1Widget)
		{
			MiniGame1Widget->AddToViewport(180);
		}
	}

	if (MiniGame1Widget)
	{
		MiniGame1Widget->OnInstructionFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1InstructionFinished);
		MiniGame1Widget->OnInstructionFinished.AddDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1InstructionFinished);
		MiniGame1Widget->BeginMiniGame();
		MiniGame1Widget->ShowInstruction(FText::FromString(TEXT("움직이세요\n(방향키를 조작하세요)")));
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 started. Arm input is locked until the first instruction fades out."));
		return;
	}

	// The UI could not be created. Avoid a soft-lock in this fallback path.
	SetPlayerArmInputEnabled(true);
	MiniGame1Phase = EKumaMiniGame1Phase::WaitingForFirstDirection;
	UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] MiniGame 1 UI is unavailable. Arm input was enabled without an instruction prompt."));
}

void AKumaChapterTwoDirector::StartMiniGame1SecondDirectionPrompt()
{
	SetPlayerArmInputEnabled(false);
	MiniGame1Phase = EKumaMiniGame1Phase::WaitingForSecondPrompt;
	if (MiniGame1Widget)
	{
		MiniGame1Widget->ShowInstruction(FText::FromString(TEXT("움직이세요\n(방향키를 조작하세요)")));
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 is waiting for the second instruction to fade out."));
		return;
	}

	SetPlayerArmInputEnabled(true);
	MiniGame1Phase = EKumaMiniGame1Phase::WaitingForSecondDirection;
	UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] MiniGame 1 UI is unavailable. Waiting for second direction key."));
}

void AKumaChapterTwoDirector::StartMiniGame1ButtonCountdown()
{
	MiniGame1RemainingSeconds = 60.f;
	SetPlayerArmInputEnabled(false);
	MiniGame1Phase = EKumaMiniGame1Phase::WaitingForButtonPrompt;
	if (MiniGame1Widget)
	{
		MiniGame1Widget->ShowInstruction(FText::FromString(TEXT("버튼을 눌러 기계의 작동을 멈추세요\n(E 키로 상호작용)")));
		MiniGame1Widget->SetCountdownVisible(false);
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 is waiting for the button instruction to fade out."));
		return;
	}

	BeginMiniGame1ButtonCountdown();
}

void AKumaChapterTwoDirector::BeginMiniGame1ButtonCountdown()
{
	MiniGame1RemainingSeconds = 60.f;
	SetPlayerArmInputEnabled(true);
	if (MiniGame1Widget)
	{
		MiniGame1Widget->SetCountdownSeconds(MiniGame1RemainingSeconds);
		MiniGame1Widget->SetCountdownVisible(true);
	}
	MiniGame1Phase = EKumaMiniGame1Phase::ButtonCountdown;
	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 countdown started: 60 seconds. Arm input enabled."));
}

void AKumaChapterTwoDirector::StartMiniGame1Success()
{
	StopMiniGame1Sound();
	if (UKumaGameInstance* GameInstance = GetGameInstance<UKumaGameInstance>())
	{
		GameInstance->StopChapterAmbience();
	}
	SetPlayerArmInputEnabled(false);
	MiniGame1Phase = EKumaMiniGame1Phase::SuccessSequence;
	if (MiniGame1Widget)
	{
		MiniGame1Widget->EndMiniGame();
	}

	MotherMiniGame1EndSequencePlayer = CreateSequencePlayer(MotherMiniGame1EndSequence, false, MotherMiniGame1EndSequenceActor);
	if (!MotherMiniGame1EndSequencePlayer)
	{
		UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] MiniGame 1 end sequence could not start. Continuing with survival dialogue."));
		StartMiniGame1SuccessDialogue();
		return;
	}

	MotherMiniGame1EndSequencePlayer->OnFinished.AddDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1EndSequenceFinished);
	MotherMiniGame1EndSequencePlayer->Play();
	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 button interaction succeeded."));
}

void AKumaChapterTwoDirector::StartMiniGame1SuccessDialogue()
{
	MiniGame1Phase = EKumaMiniGame1Phase::SuccessDialogue;
	PlayDialogue(KumaChapterTwo::MiniGame1Success, BuildMiniGame1SuccessDialogue());
}

void AKumaChapterTwoDirector::StartMiniGame1Failure()
{
	StopMiniGame1Sound();
	SetPlayerArmInputEnabled(false);
	MiniGame1Phase = EKumaMiniGame1Phase::FailureDialogue;
	if (MiniGame1Widget)
	{
		MiniGame1Widget->SetCountdownVisible(false);
		MiniGame1Widget->HideInstruction();
	}
	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 timer expired."));
	PlayDialogue(KumaChapterTwo::MiniGame1Failure, BuildMiniGame1FailureDialogue());
}

void AKumaChapterTwoDirector::RestartMiniGame1()
{
	StartMiniGame1Sound();
	StartMiniGame1();
}

void AKumaChapterTwoDirector::DeferMiniGame1ArmInputLock()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AKumaChapterTwoDirector::LockMiniGame1ArmInput));
	}
}

void AKumaChapterTwoDirector::LockMiniGame1ArmInput()
{
	SetPlayerArmInputEnabled(false);
}

void AKumaChapterTwoDirector::HandleMiniGame1InstructionFinished()
{
	if (MiniGame1Phase == EKumaMiniGame1Phase::WaitingForFirstPrompt)
	{
		SetPlayerArmInputEnabled(true);
		MiniGame1Phase = EKumaMiniGame1Phase::WaitingForFirstDirection;
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] First instruction finished. BP_Arm input enabled."));
	}
	else if (MiniGame1Phase == EKumaMiniGame1Phase::WaitingForSecondPrompt)
	{
		SetPlayerArmInputEnabled(true);
		MiniGame1Phase = EKumaMiniGame1Phase::WaitingForSecondDirection;
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] Second instruction finished. BP_Arm input enabled."));
	}
	else if (MiniGame1Phase == EKumaMiniGame1Phase::WaitingForButtonPrompt)
	{
		BeginMiniGame1ButtonCountdown();
	}
}

void AKumaChapterTwoDirector::PollMiniGame1DirectionKeys()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	FVector2D DirectionValue = FVector2D::ZeroVector;
	TArray<FString, TInlineAllocator<4>> PressedKeys;
	if (PlayerController->IsInputKeyDown(EKeys::Up))
	{
		DirectionValue.Y += 1.f;
		PressedKeys.Add(TEXT("Up"));
	}
	if (PlayerController->IsInputKeyDown(EKeys::Down))
	{
		DirectionValue.Y -= 1.f;
		PressedKeys.Add(TEXT("Down"));
	}
	if (PlayerController->IsInputKeyDown(EKeys::Right))
	{
		DirectionValue.X += 1.f;
		PressedKeys.Add(TEXT("Right"));
	}
	if (PlayerController->IsInputKeyDown(EKeys::Left))
	{
		DirectionValue.X -= 1.f;
		PressedKeys.Add(TEXT("Left"));
	}

	const bool bDirectionKeyIsDown = !DirectionValue.IsNearlyZero();
	const bool bWaitingForDirection =
		MiniGame1Phase == EKumaMiniGame1Phase::WaitingForFirstDirection ||
		MiniGame1Phase == EKumaMiniGame1Phase::WaitingForSecondDirection;

	if (bWaitingForDirection && bDirectionKeyIsDown && !bMiniGame1DirectionKeyWasDown)
	{
		const FString KeyList = FString::Join(PressedKeys, TEXT(", "));
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 direct keyboard input: %s."), *KeyList);
		HandleMiniGame1DirectionInput(DirectionValue.GetSafeNormal(), 0.f);
	}

	// A key must be released before a later stage can count it as the next input.
	bMiniGame1DirectionKeyWasDown = bDirectionKeyIsDown;
}

void AKumaChapterTwoDirector::BindMiniGame1InputRouter()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UKumaInputRouterComponent* InputRouter = PlayerPawn ? PlayerPawn->FindComponentByClass<UKumaInputRouterComponent>() : nullptr;
	if (!InputRouter)
	{
		UE_LOG(LogKumaChapterTwo, Error, TEXT("[KumaChapterTwo] MiniGame 1 could not find the player's direction input router."));
		return;
	}

	if (MiniGame1InputRouter && MiniGame1InputRouter != InputRouter)
	{
		MiniGame1InputRouter->OnDirectionInputRouted.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1DirectionInput);
	}
	MiniGame1InputRouter = InputRouter;
	MiniGame1InputRouter->OnDirectionInputRouted.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1DirectionInput);
	MiniGame1InputRouter->OnDirectionInputRouted.AddDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1DirectionInput);
	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 is listening to the player's routed direction input."));
}

void AKumaChapterTwoDirector::HandleMiniGame1DirectionInput(FVector2D DirectionValue, float DeltaTime)
{
	if (DirectionValue.IsNearlyZero())
	{
		return;
	}

	if (MiniGame1Phase == EKumaMiniGame1Phase::WaitingForFirstDirection)
	{
		MiniGame1Phase = EKumaMiniGame1Phase::WaitingForFirstDialogue;
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 received its first direction input: %s."), *DirectionValue.ToString());
		DeferMiniGame1ArmInputLock();
		TriggerMiniGame1Shake();
		PlayDialogue(KumaChapterTwo::MiniGame1FirstMove, BuildMiniGame1FirstMoveDialogue());
	}
	else if (MiniGame1Phase == EKumaMiniGame1Phase::WaitingForSecondDirection)
	{
		MiniGame1Phase = EKumaMiniGame1Phase::WaitingForSecondDialogue;
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 received its second direction input: %s."), *DirectionValue.ToString());
		DeferMiniGame1ArmInputLock();
		PlayDialogue(KumaChapterTwo::MiniGame1SecondMove, BuildMiniGame1SecondMoveDialogue());
	}
}

bool AKumaChapterTwoDirector::IsMiniGame1ButtonInteractionAvailable() const
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UArmControlComponent* ArmControl = PlayerPawn ? PlayerPawn->FindComponentByClass<UArmControlComponent>() : nullptr;
	// The C_Test actor is named SM_IR_RMC_IR_RMC_Button, while the mesh it uses
	// is named SM_IR_RMC_Button. This finder deliberately matches the mesh asset.
	AActor* ButtonActor = KumaChapterTwo::FindActorByStaticMeshAssetName(GetWorld(), TEXT("SM_IR_RMC_Button"));
	if (!ArmControl || !ButtonActor)
	{
		UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] Cannot test MiniGame 1 interaction. Arm=%s Button=%s"), ArmControl ? TEXT("found") : TEXT("missing"), ButtonActor ? TEXT("found") : TEXT("missing"));
		return false;
	}

	const FVector ButtonCenter = ButtonActor->GetComponentsBoundingBox(true).GetCenter();
	const float DistanceToButton = FVector::Dist(ArmControl->GetHandWorldLocation(), ButtonCenter);
	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 button found. Actor=%s Center=%s Hand-to-button distance: %.2f cm."),
		*GetNameSafe(ButtonActor), *ButtonCenter.ToCompactString(), DistanceToButton);
	return DistanceToButton <= 5.f;
}

void AKumaChapterTwoDirector::TriggerMiniGame1Shake() const
{
	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PlayerController->ClientStartCameraShake(UKumaMiniGame1CameraShake::StaticClass());
	}
}

void AKumaChapterTwoDirector::ShowChapterEndMenu()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController || ChapterEndWidget)
	{
		return;
	}

	if (DialogueWidget)
	{
		DialogueWidget->RemoveFromParent();
	}

	ChapterEndWidget = CreateWidget<UKumaChapterEndWidget>(PlayerController, UKumaChapterEndWidget::StaticClass());
	if (!ChapterEndWidget)
	{
		UE_LOG(LogKumaChapterTwo, Error, TEXT("[KumaChapterTwo] Could not create the Chapter 2 end menu."));
		return;
	}

	// Chapter 2 does not have a next stage yet.
	ChapterEndWidget->SetNextChapterVisible(false);
	ChapterEndWidget->AddToViewport(1000);

	PlayerController->SetIgnoreLookInput(true);
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetShowMouseCursor(true);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ChapterEndWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);

	ChapterEndWidget->OnMainMenuClicked.AddLambda([this]()
	{
		UGameplayStatics::OpenLevel(this, FName(TEXT("StartScene")));
	});
	ChapterEndWidget->OnQuitClicked.AddLambda([PlayerController]()
	{
		UKismetSystemLibrary::QuitGame(PlayerController, PlayerController, EQuitPreference::Quit, false);
	});

	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] Chapter 2 end menu opened. Buttons=main menu, exit."));
}

void AKumaChapterTwoDirector::StartMiniGame1Sound()
{
	StopMiniGame1Sound();
	USoundBase* Sound = MiniGame1Sound.LoadSynchronous();
	if (!Sound)
	{
		UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] Could not load MiniGame 1 sound: %s"), *MiniGame1Sound.ToSoftObjectPath().ToString());
		return;
	}

	bMiniGame1SoundShouldLoop = true;
	MiniGame1AudioComponent = UGameplayStatics::SpawnSound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, false, false);
	if (!MiniGame1AudioComponent)
	{
		bMiniGame1SoundShouldLoop = false;
		UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] Could not create the MiniGame 1 audio component."));
		return;
	}

	MiniGame1AudioComponent->OnAudioFinished.AddDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1SoundFinished);
	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 sound started and is looping."));
}

void AKumaChapterTwoDirector::StopMiniGame1Sound()
{
	bMiniGame1SoundShouldLoop = false;
	if (MiniGame1AudioComponent)
	{
		MiniGame1AudioComponent->OnAudioFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1SoundFinished);
		MiniGame1AudioComponent->Stop();
		MiniGame1AudioComponent = nullptr;
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 sound stopped."));
	}
}

void AKumaChapterTwoDirector::HandleMiniGame1SoundFinished()
{
	if (bMiniGame1SoundShouldLoop && MiniGame1AudioComponent)
	{
		MiniGame1AudioComponent->Play(0.f);
	}
}

void AKumaChapterTwoDirector::CreateDialogueWidget()
{
	if (DialogueWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogKumaChapterTwo, Error, TEXT("[KumaChapterTwo] Dialogue UI could not be created because there is no player controller."));
		return;
	}

	DialogueWidget = CreateWidget<UKumaDialogueBoxWidget>(PlayerController, UKumaDialogueBoxWidget::StaticClass());
	if (DialogueWidget)
	{
		DialogueWidget->AddToViewport(200);
	}
}

void AKumaChapterTwoDirector::SetPlayerArmInputEnabled(bool bEnabled) const
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UArmControlComponent* ArmControl = PlayerPawn ? PlayerPawn->FindComponentByClass<UArmControlComponent>() : nullptr;
	if (ArmControl)
	{
		ArmControl->SetArmInputEnabled(bEnabled);
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] BP_Arm input is now %s. Pawn=%s"),
			bEnabled ? TEXT("enabled") : TEXT("disabled"), *GetNameSafe(PlayerPawn));
		return;
	}

	UE_LOG(LogKumaChapterTwo, Error, TEXT("[KumaChapterTwo] BP_Arm input could not be changed because ArmControl is missing. Pawn=%s"), *GetNameSafe(PlayerPawn));
}

ULevelSequencePlayer* AKumaChapterTwoDirector::CreateSequencePlayer(const TSoftObjectPtr<ULevelSequence>& SequenceAsset, bool bLooping, TObjectPtr<ALevelSequenceActor>& OutSequenceActor) const
{
	OutSequenceActor = nullptr;
	UWorld* World = GetWorld();
	ULevelSequence* LoadedSequence = SequenceAsset.LoadSynchronous();
	if (!World || !LoadedSequence)
	{
		UE_LOG(LogKumaChapterTwo, Error, TEXT("[KumaChapterTwo] Failed to load Level Sequence: %s"), *SequenceAsset.ToSoftObjectPath().ToString());
		return nullptr;
	}

	const FSoftObjectPath AssetPath = SequenceAsset.ToSoftObjectPath();
	const bool bNeedsCTestRebinding = AssetPath == MotherEnterSequence.ToSoftObjectPath()
		|| AssetPath == MotherMachineSequence.ToSoftObjectPath()
		|| AssetPath == MotherExitSequence.ToSoftObjectPath()
		|| AssetPath == MotherMiniGame1EndSequence.ToSoftObjectPath();
	const bool bKeepFinalMotherPose = AssetPath == MotherEnterSequence.ToSoftObjectPath()
		|| AssetPath == MotherMachineSequence.ToSoftObjectPath()
		|| AssetPath == MotherExitSequence.ToSoftObjectPath()
		|| AssetPath == MotherMiniGame1EndSequence.ToSoftObjectPath();

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.LoopCount.Value = bLooping ? -1 : 0;
	// Do not pause at the end. A pause keeps the pre-animation state captured,
	// which can let a skeletal pose restore later. Let the player stop normally,
	// as Chapter 1 does, so ForceKeepState discards that restore state.
	PlaybackSettings.bPauseAtEnd = false;
	PlaybackSettings.FinishCompletionStateOverride = bKeepFinalMotherPose
		? EMovieSceneCompletionModeOverride::ForceKeepState
		: EMovieSceneCompletionModeOverride::None;
	ALevelSequenceActor* CreatedSequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LoadedSequence, PlaybackSettings, CreatedSequenceActor);
	OutSequenceActor = CreatedSequenceActor;
	if (!SequencePlayer || !OutSequenceActor)
	{
		UE_LOG(LogKumaChapterTwo, Error, TEXT("[KumaChapterTwo] Failed to create a playback actor for: %s"), *LoadedSequence->GetPathName());
		return nullptr;
	}

	OutSequenceActor->SetReplicatePlayback(false);
	// Chapter 2 cinematics contain camera cuts. They must be allowed to take
	// the player's view, just as Chapter 1's sequence player does.
	SequencePlayer->SetDisableCameraCuts(false);

	// videotape_sequence1 and playmachine1 already own C_Test bindings. Only
	// the two sequences authored in JAE_Test1 must be rebound at runtime.
	if (bNeedsCTestRebinding)
	{
		ApplyCTestBindingOverrides(LoadedSequence, OutSequenceActor);
	}

	// Preserve the exact last pose and transform of Annie after every one of
	// her Chapter 2 sequences, matching Chapter 1's completed-sequence logic.
	if (bKeepFinalMotherPose)
	{
		OutSequenceActor->PlaybackSettings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceKeepState;
		SequencePlayer->SetCompletionModeOverride(EMovieSceneCompletionModeOverride::ForceKeepState);
	}

	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] Created sequence player. Sequence=%s Looping=%s"), *LoadedSequence->GetPathName(), bLooping ? TEXT("true") : TEXT("false"));
	return SequencePlayer;
}

void AKumaChapterTwoDirector::ApplyCTestBindingOverrides(ULevelSequence* SequenceAsset, ALevelSequenceActor* SequenceActor) const
{
	UMovieScene* MovieScene = SequenceAsset ? SequenceAsset->GetMovieScene() : nullptr;
	UWorld* World = GetWorld();
	if (!MovieScene || !SequenceActor || !World)
	{
		return;
	}

	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		const FString& BindingName = Binding.GetName();
		TArray<AActor*> MatchingActors;
		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			AActor* Actor = *ActorIt;
			if (KumaChapterTwo::IsActorBoundToName(Actor, BindingName))
			{
				MatchingActors.Add(Actor);
			}
		}

		if (MatchingActors.IsEmpty())
		{
			UE_LOG(LogKumaChapterTwo, Warning, TEXT("[KumaChapterTwo] No C_Test actor found for sequence binding '%s' in %s."), *BindingName, *SequenceAsset->GetName());
			continue;
		}

		const FMovieSceneObjectBindingID BindingId(UE::MovieScene::FRelativeObjectBindingID(Binding.GetObjectGuid()));
		SequenceActor->SetBinding(BindingId, MatchingActors, false);
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] Rebound '%s' in %s to %d C_Test actor(s)."), *BindingName, *SequenceAsset->GetName(), MatchingActors.Num());
	}
}

void AKumaChapterTwoDirector::LockMotherInMachineFinalPose() const
{
	USkeletalMeshComponent* MotherMeshComponent = KumaChapterTwo::FindCTestMotherMeshComponent(GetWorld());
	if (!MotherMeshComponent)
	{
		UE_LOG(LogKumaChapterTwo, Error, TEXT("[KumaChapterTwo] Could not freeze Annie at playmachine1's final pose: C_Test skeletal mesh is missing."));
		return;
	}

	// ForceKeepState leaves Sequencer's final pose on the component. Disabling
	// only the skeletal-mesh tick preserves those exact bone transforms instead
	// of replacing them with a separate idle animation.
	MotherMeshComponent->SetComponentTickEnabled(false);
	UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] Froze C_Test Annie at playmachine1's exact final sequencer pose."));
}

void AKumaChapterTwoDirector::PlayDialogue(FName OwnerId, const TArray<FKumaDialogueLine>& Lines) const
{
	if (UKumaDialogueSubsystem* DialogueSubsystem = GetDialogueSubsystem())
	{
		DialogueSubsystem->PlayDialogueSequence(OwnerId, Lines);
		return;
	}

	UE_LOG(LogKumaChapterTwo, Error, TEXT("[KumaChapterTwo] Dialogue subsystem is unavailable. Owner=%s"), *OwnerId.ToString());
}

FKumaDialogueLine AKumaChapterTwoDirector::MakeAutoLine(FName LineId, FName SpeakerId, const TCHAR* Text) const
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

UKumaDialogueSubsystem* AKumaChapterTwoDirector::GetDialogueSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UKumaDialogueSubsystem>();
	}

	return nullptr;
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildOpeningDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_OPEN_01"), KumaChapterTwo::Tomas, TEXT("Morning. Voices from downstairs. A visitor?")),
		MakeAutoLine(TEXT("CH02_OPEN_02"), KumaChapterTwo::Tomas, TEXT("A machine I've never seen before. What on earth is that?"))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildHankDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_HANK_01"), KumaChapterTwo::Hank, TEXT("Tomas? It's Uncle Hank. Heard from the workers today — the treatment device should have arrived at the house by now.")),
		MakeAutoLine(TEXT("CH02_HANK_02"), KumaChapterTwo::Hank, TEXT("It's something the military's been testing. They say it treats paralysis using thermal waves.. Too much to explain, I couldn't catch all of it.")),
		MakeAutoLine(TEXT("CH02_HANK_03"), KumaChapterTwo::Hank, TEXT("It's still military-issue, so without yours truly, you wouldn't be getting your hands on it — I'll tell you that!")),
		MakeAutoLine(TEXT("CH02_HANK_04"), KumaChapterTwo::Hank, TEXT("Still experimental, so who knows how effective it'll be. But worth trying something, right.")),
		MakeAutoLine(TEXT("CH02_HANK_05"), KumaChapterTwo::Hank, TEXT("Hope it helps! If there's any problem, just sa— oh, right. You can't say anything.")),
		MakeAutoLine(TEXT("CH02_HANK_06"), KumaChapterTwo::Hank, TEXT("Anyway, I'm rooting for you, Tomas! Talk again soon."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildPostHankDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_POST_HANK_01"), KumaChapterTwo::Tomas, TEXT("I miss Uncle Hank.")),
		MakeAutoLine(TEXT("CH02_POST_HANK_02"), KumaChapterTwo::Tomas, TEXT("Will this machine actually cure me? Uncle Hank has sent so many 'treatment devices' over the years. So many failures.")),
		MakeAutoLine(TEXT("CH02_POST_HANK_03"), KumaChapterTwo::Tomas, TEXT("The massage machine two years ago was the worst. Two hours of being tickled — genuine torture.")),
		MakeAutoLine(TEXT("CH02_POST_HANK_04"), KumaChapterTwo::Tomas, TEXT("I remember Mom finally throwing it out only after I'd cried my eyes out.")),
		MakeAutoLine(TEXT("CH02_POST_HANK_05"), KumaChapterTwo::Tomas, TEXT("I'm sure Mother put in another enormous request. Uncle Hank's voice sounded so thoroughly fed up."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildMotherDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_MOTHER_01"), KumaChapterTwo::Annie, TEXT("Thank you! Yes, yes~")),
		MakeAutoLine(TEXT("CH02_MOTHER_02"), KumaChapterTwo::Annie, TEXT("Tomas sweetie, isn't it wonderful? It's called the THERMACORE-3000. They say it was developed to treat soldiers paralyzed by poison gas!")),
		MakeAutoLine(TEXT("CH02_MOTHER_03"), KumaChapterTwo::Annie, TEXT("Oh, I just have a feeling this time — you're really going to get up. I'm so excited!")),
		MakeAutoLine(TEXT("CH02_MOTHER_04"), KumaChapterTwo::Annie, TEXT("Hank really should have told us about this sooner.")),
		MakeAutoLine(TEXT("CH02_MOTHER_05"), KumaChapterTwo::Annie, TEXT("I'm already looking forward to winter! I'll bake you the most delicious chocolate cake."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildMachineDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_MACHINE_01"), KumaChapterTwo::Annie, TEXT("How does it feel, sweetheart? Is the treatment working?")),
		MakeAutoLine(TEXT("CH02_MACHINE_02"), KumaChapterTwo::Annie, TEXT("If only you could say even a little something.."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildPostMachineDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_POST_MACHINE_01"), KumaChapterTwo::Annie, TEXT("I'm not expecting you to be cured all at once. Keep at it and I'm sure you'll get better.")),
		MakeAutoLine(TEXT("CH02_POST_MACHINE_02"), KumaChapterTwo::Annie, TEXT("Oh! What am I doing standing around.")),
		MakeAutoLine(TEXT("CH02_POST_MACHINE_03"), KumaChapterTwo::Annie, TEXT("I'm going to fetch Father Joseph from the church, sweetie. This treatment device needs to be blessed.")),
		MakeAutoLine(TEXT("CH02_POST_MACHINE_04"), KumaChapterTwo::Annie, TEXT("I'll be right back, don't worry!"))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildHeatDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_HEAT_01"), KumaChapterTwo::Tomas, TEXT("Warmth begins to seep through the thin sheet.")),
		MakeAutoLine(TEXT("CH02_HEAT_02"), KumaChapterTwo::Tomas, TEXT("Probably just another glorified heater. No sudden urge to move.")),
		MakeAutoLine(TEXT("CH02_HEAT_03"), KumaChapterTwo::Tomas, TEXT("It is warm, though. I'll give it that.")),
		MakeAutoLine(TEXT("CH02_HEAT_04"), KumaChapterTwo::Tomas, TEXT("Actually, it's getting a little hot.")),
		MakeAutoLine(TEXT("CH02_HEAT_05"), KumaChapterTwo::Tomas, TEXT("No — it's definitely hot. Something is wrong.")),
		MakeAutoLine(TEXT("CH02_HEAT_06"), KumaChapterTwo::Tomas, TEXT("This is nothing like that tickling massage machine. If I can't stop this thing, I'll slowly cook to death before Mother gets back.")),
		MakeAutoLine(TEXT("CH02_HEAT_07"), KumaChapterTwo::Tomas, TEXT("I can't scream. And even if I could, there's no one to hear it.")),
		MakeAutoLine(TEXT("CH02_HEAT_08"), KumaChapterTwo::Tomas, TEXT("I have to move.")),
		MakeAutoLine(TEXT("CH02_HEAT_09"), KumaChapterTwo::Tomas, TEXT("I have to move!"))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildMiniGame1FirstMoveDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_MG1_MOVE_01"), KumaChapterTwo::Tomas, TEXT("Move!! Please!!"))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildMiniGame1SecondMoveDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_MG1_MOVE_02"), KumaChapterTwo::Tomas, TEXT("It's moving! I can move!")),
		MakeAutoLine(TEXT("CH02_MG1_MOVE_03"), KumaChapterTwo::Tomas, TEXT("Even if it's just a little, I can move!"))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildMiniGame1SuccessDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_MG1_SUCCESS_01"), KumaChapterTwo::Annie, TEXT("Tomas? Sweetie, Father Joseph is here.")),
		MakeAutoLine(TEXT("CH02_MG1_SUCCESS_02"), KumaChapterTwo::Annie, TEXT("Tomas? Tomas!")),
		MakeAutoLine(TEXT("CH02_MG1_SUCCESS_03"), KumaChapterTwo::Tomas, TEXT("My mind grows faint. The tension drains away in the relief of survival."))
	};
}

TArray<FKumaDialogueLine> AKumaChapterTwoDirector::BuildMiniGame1FailureDialogue() const
{
	return {
		MakeAutoLine(TEXT("CH02_MG1_FAILURE_01"), KumaChapterTwo::Tomas, TEXT("Mom.. Mom.."))
	};
}

void AKumaChapterTwoDirector::HandleDialogueSequenceFinished(FName OwnerId, FName LastLineId)
{
	if (OwnerId == KumaChapterTwo::Opening)
	{
		StartHankCall();
	}
	else if (OwnerId == KumaChapterTwo::HankCall)
	{
		if (HankSequencePlayer) HankSequencePlayer->Stop();
		StartPostHankNarration();
	}
	else if (OwnerId == KumaChapterTwo::PostHank)
	{
		StartMotherEnter();
	}
	else if (OwnerId == KumaChapterTwo::MotherDialogue)
	{
		StartMotherMachineTreatment();
	}
	else if (OwnerId == KumaChapterTwo::MotherMachine)
	{
		bMotherMachineDialogueFinished = true;
		TryStartPostMachineDialogue();
	}
	else if (OwnerId == KumaChapterTwo::PostMachine)
	{
		StartMotherExit();
	}
	else if (OwnerId == KumaChapterTwo::HeatNarration)
	{
		FinishBeforeMiniGame1();
	}
	else if (OwnerId == KumaChapterTwo::MiniGame1FirstMove)
	{
		StartMiniGame1SecondDirectionPrompt();
	}
	else if (OwnerId == KumaChapterTwo::MiniGame1SecondMove)
	{
		StartMiniGame1ButtonCountdown();
	}
	else if (OwnerId == KumaChapterTwo::MiniGame1Success)
	{
		MiniGame1Phase = EKumaMiniGame1Phase::Complete;
		UE_LOG(LogKumaChapterTwo, Log, TEXT("[KumaChapterTwo] MiniGame 1 completed successfully."));
		ShowChapterEndMenu();
	}
	else if (OwnerId == KumaChapterTwo::MiniGame1Failure)
	{
		MiniGame1Phase = EKumaMiniGame1Phase::FailureFade;
		if (MiniGame1Widget)
		{
			MiniGame1Widget->BeginFailureFade();
		}
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(MiniGame1RestartTimer, this, &AKumaChapterTwoDirector::RestartMiniGame1, 0.85f, false);
		}
	}
}

void AKumaChapterTwoDirector::HandleDialogueLineStarted(FName LineId, FName SpeakerId, const FText& FullText)
{
	if (LineId == KumaChapterTwo::MiniGame1SoundStart)
	{
		StartMiniGame1Sound();
	}
}

void AKumaChapterTwoDirector::HandleMotherEnterSequenceFinished()
{
	if (MotherEnterSequencePlayer)
	{
		MotherEnterSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMotherEnterSequenceFinished);
	}
	StartMotherDialogue();
}

void AKumaChapterTwoDirector::HandleMotherMachineSequenceFinished()
{
	if (MotherMachineSequencePlayer)
	{
		MotherMachineSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMotherMachineSequenceFinished);
		UE_LOG(LogKumaChapterTwo, Log,
			TEXT("[KumaChapterTwo] playmachine1 reached its final frame. Frame=%d Completion=%d Paused=%s"),
			MotherMachineSequencePlayer->GetCurrentTime().Time.FrameNumber.Value,
			static_cast<int32>(MotherMachineSequencePlayer->GetCompletionModeOverride()),
			MotherMachineSequencePlayer->IsPaused() ? TEXT("true") : TEXT("false"));
	}
	LockMotherInMachineFinalPose();
	bMotherMachineSequenceFinished = true;
	TryStartPostMachineDialogue();
}

void AKumaChapterTwoDirector::HandleMotherExitSequenceFinished()
{
	if (MotherExitSequencePlayer)
	{
		MotherExitSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMotherExitSequenceFinished);
	}
	StartHeatNarration();
}

void AKumaChapterTwoDirector::HandleMiniGame1EndSequenceFinished()
{
	if (MotherMiniGame1EndSequencePlayer)
	{
		MotherMiniGame1EndSequencePlayer->OnFinished.RemoveDynamic(this, &AKumaChapterTwoDirector::HandleMiniGame1EndSequenceFinished);
	}
	StartMiniGame1SuccessDialogue();
}
