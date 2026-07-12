// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KumaInputRouterComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaDirectionInputRoutedSignature, FVector2D, DirectionValue, float, DeltaTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaMouseDragInputRoutedSignature, FVector2D, DragDelta, float, DeltaTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKumaMouseWheelInputRoutedSignature, float, WheelValue, float, DeltaTime);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KUMAMARU_API UKumaInputRouterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKumaInputRouterComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Kuma Input")
	void SetDefaultInputReceiver(UObject* NewDefaultReceiver);

	UFUNCTION(BlueprintCallable, Category = "Kuma Input")
	void PushInputReceiver(UObject* NewInputReceiver);

	UFUNCTION(BlueprintCallable, Category = "Kuma Input")
	void PopInputReceiver(UObject* ExpectedInputReceiver);

	UFUNCTION(BlueprintCallable, Category = "Kuma Input")
	void ClearInputReceiverStack();

	UFUNCTION(BlueprintPure, Category = "Kuma Input")
	UObject* GetCurrentInputReceiver() const;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Input|Events")
	FKumaDirectionInputRoutedSignature OnDirectionInputRouted;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Input|Events")
	FKumaMouseDragInputRoutedSignature OnMouseDragInputRouted;

	UPROPERTY(BlueprintAssignable, Category = "Kuma Input|Events")
	FKumaMouseWheelInputRoutedSignature OnMouseWheelInputRouted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Input|Direction")
	bool bReadDirectionKeys = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Input|Mouse")
	bool bReadMouseDrag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Input|Mouse")
	bool bReadMouseWheel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Input|Mouse")
	FKey MouseDragButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Input|Mouse", meta = (ClampMin = "0.01"))
	float MouseDragInputScale = 14.285714f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Input|Debug")
	bool bDebugLogMouseDragInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kuma Input|Debug", meta = (ClampMin = "0.05"))
	float MouseDragDebugLogInterval = 0.25f;

private:
	TWeakObjectPtr<UObject> DefaultInputReceiver;
	TArray<TWeakObjectPtr<UObject>> InputReceiverStack;
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	bool bWasDragging = false;
	float MouseDragDebugElapsed = 0.f;

	APlayerController* GetPlayerController();
	void RouteDirectionInput(float DeltaTime);
	void RouteMouseDragInput(float DeltaTime);
	void RouteMouseWheelInput(float DeltaTime);
	void RouteToCurrentReceiverDirection(FVector2D DirectionValue, float DeltaTime);
	void RouteToCurrentReceiverMouseDrag(FVector2D DragDelta, float DeltaTime);
	void RouteToCurrentReceiverMouseWheel(float WheelValue, float DeltaTime);
	bool CanRouteToReceiver(const UObject* Receiver) const;
};
