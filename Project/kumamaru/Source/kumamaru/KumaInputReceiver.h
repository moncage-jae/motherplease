// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KumaInputReceiver.generated.h"

UINTERFACE(BlueprintType)
class KUMAMARU_API UKumaInputReceiver : public UInterface
{
	GENERATED_BODY()
};

class KUMAMARU_API IKumaInputReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kuma Input")
	void HandleKumaDirectionInput(FVector2D DirectionValue, float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kuma Input")
	void HandleKumaMouseDragInput(FVector2D DragDelta, float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kuma Input")
	void HandleKumaMouseWheelInput(float WheelValue, float DeltaTime);
};
