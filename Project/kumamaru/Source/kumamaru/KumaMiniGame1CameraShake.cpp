#include "KumaMiniGame1CameraShake.h"

#include "Shakes/WaveOscillatorCameraShakePattern.h"

UKumaMiniGame1CameraShake::UKumaMiniGame1CameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Camera shake classes construct a class default object first. The generic
	// ChangeRootShakePattern helper uses NewObject with no explicit name, which
	// is invalid while that default object is being created. Create a named
	// default subobject instead so every shake instance has a stable pattern.
	UWaveOscillatorCameraShakePattern* Pattern = ObjectInitializer.CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(this, TEXT("MiniGame1WavePattern"));
	SetRootShakePattern(Pattern);
	Pattern->Duration = 0.55f;
	Pattern->BlendInTime = 0.03f;
	Pattern->BlendOutTime = 0.18f;
	Pattern->Pitch.Amplitude = 1.5f;
	Pattern->Pitch.Frequency = 20.f;
	Pattern->Yaw.Amplitude = 1.0f;
	Pattern->Yaw.Frequency = 24.f;
	Pattern->X.Amplitude = 1.4f;
	Pattern->X.Frequency = 22.f;
	Pattern->Y.Amplitude = 1.0f;
	Pattern->Y.Frequency = 18.f;
	Pattern->Pitch.InitialOffsetType = EInitialWaveOscillatorOffsetType::Zero;
	Pattern->Yaw.InitialOffsetType = EInitialWaveOscillatorOffsetType::Zero;
	Pattern->X.InitialOffsetType = EInitialWaveOscillatorOffsetType::Zero;
	Pattern->Y.InitialOffsetType = EInitialWaveOscillatorOffsetType::Zero;
}
