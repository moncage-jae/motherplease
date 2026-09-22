#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "KumaMiniGame1CameraShake.generated.h"

/** A short, sharp camera shake used when Tomas first manages to move. */
UCLASS()
class KUMAMARU_API UKumaMiniGame1CameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UKumaMiniGame1CameraShake(const FObjectInitializer& ObjectInitializer);
};
