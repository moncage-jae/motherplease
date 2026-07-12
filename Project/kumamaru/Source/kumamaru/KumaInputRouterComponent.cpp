// Fill out your copyright notice in the Description page of Project Settings.

#include "KumaInputRouterComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KumaInputReceiver.h"

DEFINE_LOG_CATEGORY_STATIC(LogKumaInputRouter, Log, All);

UKumaInputRouterComponent::UKumaInputRouterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	MouseDragButton = EKeys::LeftMouseButton;
}

void UKumaInputRouterComponent::BeginPlay()
{
	Super::BeginPlay();
	GetPlayerController();
}

void UKumaInputRouterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearInputReceiverStack();
	DefaultInputReceiver.Reset();
	CachedPlayerController.Reset();

	Super::EndPlay(EndPlayReason);
}

void UKumaInputRouterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetCurrentInputReceiver())
	{
		return;
	}

	RouteDirectionInput(DeltaTime);
	RouteMouseDragInput(DeltaTime);
	RouteMouseWheelInput(DeltaTime);
}

void UKumaInputRouterComponent::SetDefaultInputReceiver(UObject* NewDefaultReceiver)
{
	if (NewDefaultReceiver && !CanRouteToReceiver(NewDefaultReceiver))
	{
		UE_LOG(LogKumaInputRouter, Warning, TEXT("[KumaInput] Default receiver does not implement KumaInputReceiver. Receiver=%s"), *GetNameSafe(NewDefaultReceiver));
		return;
	}

	DefaultInputReceiver = NewDefaultReceiver;
}

void UKumaInputRouterComponent::PushInputReceiver(UObject* NewInputReceiver)
{
	if (!NewInputReceiver || !CanRouteToReceiver(NewInputReceiver))
	{
		UE_LOG(LogKumaInputRouter, Warning, TEXT("[KumaInput] Push ignored because receiver is invalid. Receiver=%s"), *GetNameSafe(NewInputReceiver));
		return;
	}

	InputReceiverStack.Add(NewInputReceiver);
	bWasDragging = false;
}

void UKumaInputRouterComponent::PopInputReceiver(UObject* ExpectedInputReceiver)
{
	for (int32 Index = InputReceiverStack.Num() - 1; Index >= 0; --Index)
	{
		UObject* Receiver = InputReceiverStack[Index].Get();
		if (!Receiver)
		{
			InputReceiverStack.RemoveAt(Index);
			continue;
		}

		if (!ExpectedInputReceiver || Receiver == ExpectedInputReceiver)
		{
			InputReceiverStack.RemoveAt(Index);
			break;
		}
	}

	bWasDragging = false;
}

void UKumaInputRouterComponent::ClearInputReceiverStack()
{
	InputReceiverStack.Reset();
	bWasDragging = false;
}

UObject* UKumaInputRouterComponent::GetCurrentInputReceiver() const
{
	for (int32 Index = InputReceiverStack.Num() - 1; Index >= 0; --Index)
	{
		if (UObject* Receiver = InputReceiverStack[Index].Get())
		{
			return Receiver;
		}
	}

	return DefaultInputReceiver.Get();
}

APlayerController* UKumaInputRouterComponent::GetPlayerController()
{
	if (CachedPlayerController.IsValid())
	{
		return CachedPlayerController.Get();
	}

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		CachedPlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	}
	else
	{
		CachedPlayerController = Cast<APlayerController>(GetOwner());
	}

	return CachedPlayerController.Get();
}

void UKumaInputRouterComponent::RouteDirectionInput(float DeltaTime)
{
	if (!bReadDirectionKeys)
	{
		return;
	}

	APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		return;
	}

	FVector2D DirectionValue = FVector2D::ZeroVector;
	if (PlayerController->IsInputKeyDown(EKeys::Up))    { DirectionValue.Y += 1.f; }
	if (PlayerController->IsInputKeyDown(EKeys::Down))  { DirectionValue.Y -= 1.f; }
	if (PlayerController->IsInputKeyDown(EKeys::Right)) { DirectionValue.X += 1.f; }
	if (PlayerController->IsInputKeyDown(EKeys::Left))  { DirectionValue.X -= 1.f; }

	if (!DirectionValue.IsNearlyZero())
	{
		RouteToCurrentReceiverDirection(DirectionValue.GetSafeNormal(), DeltaTime);
	}
}

void UKumaInputRouterComponent::RouteMouseDragInput(float DeltaTime)
{
	if (!bReadMouseDrag)
	{
		return;
	}

	APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		bWasDragging = false;
		return;
	}

	if (!PlayerController->IsInputKeyDown(MouseDragButton))
	{
		if (bWasDragging && bDebugLogMouseDragInput)
		{
			UE_LOG(LogKumaInputRouter, Log, TEXT("[KumaInput] Mouse drag stopped."));
		}

		bWasDragging = false;
		MouseDragDebugElapsed = 0.f;
		return;
	}

	if (!bWasDragging)
	{
		bWasDragging = true;
		MouseDragDebugElapsed = 0.f;
		if (bDebugLogMouseDragInput)
		{
			UE_LOG(LogKumaInputRouter, Log, TEXT("[KumaInput] Mouse drag started. Receiver=%s"), *GetNameSafe(GetCurrentInputReceiver()));
		}
		return;
	}

	float MouseDeltaX = 0.f;
	float MouseDeltaY = 0.f;
	PlayerController->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);

	const FVector2D DragDelta(MouseDeltaX * MouseDragInputScale, MouseDeltaY * MouseDragInputScale);
	MouseDragDebugElapsed += DeltaTime;
	if (bDebugLogMouseDragInput && MouseDragDebugElapsed >= MouseDragDebugLogInterval)
	{
		UE_LOG(LogKumaInputRouter, Log, TEXT("[KumaInput] Mouse drag delta=%s Receiver=%s"),
			*DragDelta.ToString(),
			*GetNameSafe(GetCurrentInputReceiver()));
		MouseDragDebugElapsed = 0.f;
	}

	if (!DragDelta.IsNearlyZero())
	{
		RouteToCurrentReceiverMouseDrag(DragDelta, DeltaTime);
	}
}

void UKumaInputRouterComponent::RouteMouseWheelInput(float DeltaTime)
{
	if (!bReadMouseWheel)
	{
		return;
	}

	APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		return;
	}

	float WheelValue = PlayerController->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (FMath::IsNearlyZero(WheelValue))
	{
		if (PlayerController->WasInputKeyJustPressed(EKeys::MouseScrollUp))
		{
			WheelValue += 1.f;
		}

		if (PlayerController->WasInputKeyJustPressed(EKeys::MouseScrollDown))
		{
			WheelValue -= 1.f;
		}
	}

	if (!FMath::IsNearlyZero(WheelValue))
	{
		RouteToCurrentReceiverMouseWheel(WheelValue, DeltaTime);
	}
}

void UKumaInputRouterComponent::RouteToCurrentReceiverDirection(FVector2D DirectionValue, float DeltaTime)
{
	OnDirectionInputRouted.Broadcast(DirectionValue, DeltaTime);

	UObject* Receiver = GetCurrentInputReceiver();
	if (CanRouteToReceiver(Receiver))
	{
		IKumaInputReceiver::Execute_HandleKumaDirectionInput(Receiver, DirectionValue, DeltaTime);
	}
}

void UKumaInputRouterComponent::RouteToCurrentReceiverMouseDrag(FVector2D DragDelta, float DeltaTime)
{
	OnMouseDragInputRouted.Broadcast(DragDelta, DeltaTime);

	UObject* Receiver = GetCurrentInputReceiver();
	if (CanRouteToReceiver(Receiver))
	{
		IKumaInputReceiver::Execute_HandleKumaMouseDragInput(Receiver, DragDelta, DeltaTime);
	}
}

void UKumaInputRouterComponent::RouteToCurrentReceiverMouseWheel(float WheelValue, float DeltaTime)
{
	OnMouseWheelInputRouted.Broadcast(WheelValue, DeltaTime);

	UObject* Receiver = GetCurrentInputReceiver();
	if (CanRouteToReceiver(Receiver))
	{
		IKumaInputReceiver::Execute_HandleKumaMouseWheelInput(Receiver, WheelValue, DeltaTime);
	}
}

bool UKumaInputRouterComponent::CanRouteToReceiver(const UObject* Receiver) const
{
	return Receiver && Receiver->GetClass()->ImplementsInterface(UKumaInputReceiver::StaticClass());
}
