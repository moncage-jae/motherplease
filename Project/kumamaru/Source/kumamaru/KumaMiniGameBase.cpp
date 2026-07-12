// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaMiniGameBase.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KumaInputRouterComponent.h"
#include "KumaStoryFlowSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogKumaMiniGame, Log, All);

AKumaMiniGameBase::AKumaMiniGameBase()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);
}

void AKumaMiniGameBase::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoBindToStoryFlow)
	{
		if (UKumaStoryFlowSubsystem* StoryFlowSubsystem = GetStoryFlowSubsystem())
		{
			StoryFlowSubsystem->OnMiniGameRequested.AddDynamic(this, &AKumaMiniGameBase::HandleStoryMiniGameRequested);
			UE_LOG(LogKumaMiniGame, Log, TEXT("[KumaMiniGame] Bound to story request. Actor=%s MiniGameId=%s"), *GetName(), *MiniGameId.ToString());
		}
		else
		{
			UE_LOG(LogKumaMiniGame, Warning, TEXT("[KumaMiniGame] Story subsystem not found while binding. Actor=%s MiniGameId=%s"), *GetName(), *MiniGameId.ToString());
		}
	}
}

void AKumaMiniGameBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseInputRouter();

	if (UKumaStoryFlowSubsystem* StoryFlowSubsystem = GetStoryFlowSubsystem())
	{
		StoryFlowSubsystem->OnMiniGameRequested.RemoveDynamic(this, &AKumaMiniGameBase::HandleStoryMiniGameRequested);
	}

	Super::EndPlay(EndPlayReason);
}

bool AKumaMiniGameBase::StartMiniGame(FName StepId, FName RequestedMiniGameId)
{
	const FName ResolvedMiniGameId = RequestedMiniGameId.IsNone() ? MiniGameId : RequestedMiniGameId;
	if (ResolvedMiniGameId.IsNone())
	{
		UE_LOG(LogKumaMiniGame, Warning, TEXT("[KumaMiniGame] Start ignored because MiniGameId is None. Actor=%s"), *GetName());
		return false;
	}

	if (!DoesMiniGameIdMatch(ResolvedMiniGameId))
	{
		UE_LOG(LogKumaMiniGame, Warning, TEXT("[KumaMiniGame] Start ignored because id does not match. Actor=%s Expected=%s Actual=%s"),
			*GetName(),
			*MiniGameId.ToString(),
			*ResolvedMiniGameId.ToString());
		return false;
	}

	if (IsMiniGameRunning())
	{
		UE_LOG(LogKumaMiniGame, Warning, TEXT("[KumaMiniGame] Start ignored because mini game is already running. MiniGame=%s"), *ResolvedMiniGameId.ToString());
		return false;
	}

	ActiveStepId = StepId;
	ActiveMiniGameId = ResolvedMiniGameId;
	MiniGameState = EKumaMiniGameState::Running;
	SetActorTickEnabled(true);
	UE_LOG(LogKumaMiniGame, Log, TEXT("[KumaMiniGame] Start MiniGame=%s Step=%s Actor=%s"), *ActiveMiniGameId.ToString(), *ActiveStepId.ToString(), *GetName());
	ClaimInputRouter();

	HandleMiniGameStarted();
	OnMiniGameStarted.Broadcast(ActiveStepId, ActiveMiniGameId);
	ReceiveMiniGameStarted(ActiveStepId, ActiveMiniGameId);

	return true;
}

bool AKumaMiniGameBase::FinishMiniGame(bool bSucceeded)
{
	if (!IsMiniGameRunning())
	{
		UE_LOG(LogKumaMiniGame, Warning, TEXT("[KumaMiniGame] Finish ignored because mini game is not running. Actor=%s"), *GetName());
		return false;
	}

	const FName FinishedStepId = ActiveStepId;
	const FName FinishedMiniGameId = ActiveMiniGameId;

	MiniGameState = bSucceeded ? EKumaMiniGameState::Succeeded : EKumaMiniGameState::Failed;
	SetActorTickEnabled(false);
	UE_LOG(LogKumaMiniGame, Log, TEXT("[KumaMiniGame] Finish MiniGame=%s Step=%s Succeeded=%s Actor=%s"),
		*FinishedMiniGameId.ToString(),
		*FinishedStepId.ToString(),
		bSucceeded ? TEXT("true") : TEXT("false"),
		*GetName());
	ReleaseInputRouter();

	HandleMiniGameFinished(bSucceeded);
	OnMiniGameFinished.Broadcast(FinishedStepId, FinishedMiniGameId, bSucceeded);
	ReceiveMiniGameFinished(FinishedStepId, FinishedMiniGameId, bSucceeded);

	bool bStoryAcceptedResult = false;
	if (UKumaStoryFlowSubsystem* StoryFlowSubsystem = GetStoryFlowSubsystem())
	{
		bStoryAcceptedResult = StoryFlowSubsystem->NotifyMiniGameResult(FinishedMiniGameId, bSucceeded);
	}

	ActiveStepId = NAME_None;
	ActiveMiniGameId = NAME_None;

	return bStoryAcceptedResult;
}

bool AKumaMiniGameBase::SucceedMiniGame()
{
	return FinishMiniGame(true);
}

bool AKumaMiniGameBase::FailMiniGame()
{
	return FinishMiniGame(false);
}

void AKumaMiniGameBase::HandleMiniGameStarted()
{
}

void AKumaMiniGameBase::HandleMiniGameFinished(bool bSucceeded)
{
}

UKumaStoryFlowSubsystem* AKumaMiniGameBase::GetStoryFlowSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UKumaStoryFlowSubsystem>();
	}

	return nullptr;
}

UKumaInputRouterComponent* AKumaMiniGameBase::FindInputRouter() const
{
	const UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return nullptr;
	}

	if (APawn* Pawn = PlayerController->GetPawn())
	{
		if (UKumaInputRouterComponent* InputRouter = Pawn->FindComponentByClass<UKumaInputRouterComponent>())
		{
			return InputRouter;
		}
	}

	return PlayerController->FindComponentByClass<UKumaInputRouterComponent>();
}

void AKumaMiniGameBase::ClaimInputRouter()
{
	if (!bClaimInputWhileRunning)
	{
		return;
	}

	UKumaInputRouterComponent* InputRouter = FindInputRouter();
	if (!InputRouter)
	{
		UE_LOG(LogKumaMiniGame, Warning, TEXT("[KumaMiniGame] Input router not found. MiniGame=%s"), *MiniGameId.ToString());
		return;
	}

	InputRouter->PushInputReceiver(this);
	ClaimedInputRouter = InputRouter;
	UE_LOG(LogKumaMiniGame, Log, TEXT("[KumaMiniGame] Claimed input router. MiniGame=%s Actor=%s RouterOwner=%s"), *MiniGameId.ToString(), *GetName(), *GetNameSafe(InputRouter->GetOwner()));
}

void AKumaMiniGameBase::ReleaseInputRouter()
{
	if (UKumaInputRouterComponent* InputRouter = ClaimedInputRouter.Get())
	{
		InputRouter->PopInputReceiver(this);
		UE_LOG(LogKumaMiniGame, Log, TEXT("[KumaMiniGame] Released input router. MiniGame=%s Actor=%s"), *MiniGameId.ToString(), *GetName());
	}

	ClaimedInputRouter.Reset();
}

void AKumaMiniGameBase::HandleStoryMiniGameRequested(FName StepId, FName RequestedMiniGameId)
{
	UE_LOG(LogKumaMiniGame, Log, TEXT("[KumaMiniGame] Story requested MiniGame=%s Step=%s Actor=%s ActorMiniGameId=%s"),
		*RequestedMiniGameId.ToString(),
		*StepId.ToString(),
		*GetName(),
		*MiniGameId.ToString());

	if (DoesMiniGameIdMatch(RequestedMiniGameId))
	{
		StartMiniGame(StepId, RequestedMiniGameId);
	}
}

bool AKumaMiniGameBase::DoesMiniGameIdMatch(FName RequestedMiniGameId) const
{
	return !MiniGameId.IsNone() && RequestedMiniGameId == MiniGameId;
}
