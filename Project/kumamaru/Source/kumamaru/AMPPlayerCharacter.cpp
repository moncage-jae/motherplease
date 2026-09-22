// Fill out your copyright notice in the Description page of Project Settings.


#include "AMPPlayerCharacter.h"
#include "ArmControlComponent.h"
#include "KumaInputRouterComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

// Sets default values
AAMPPlayerCharacter::AAMPPlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

		// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Drives the TwoBoneIK arm targets (HandTargetLocation / JointTargetLocation) pushed to the AnimInstance
	ArmControl = CreateDefaultSubobject<UArmControlComponent>(TEXT("ArmControl"));

	InputRouter = CreateDefaultSubobject<UKumaInputRouterComponent>(TEXT("InputRouter"));

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultContextFinder(TEXT("/Game/ThirdPerson/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionFinder(TEXT("/Game/ThirdPerson/Input/Actions/IA_Jump.IA_Jump"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/ThirdPerson/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(TEXT("/Game/ThirdPerson/Input/Actions/IA_Look.IA_Look"));
	DefaultMappingContext = DefaultContextFinder.Object;
	JumpAction = JumpActionFinder.Object;
	MoveAction = MoveActionFinder.Object;
	LookAction = LookActionFinder.Object;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

// Called when the game starts or when spawned
void AAMPPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (FollowCamera)
	{
		LevelStartCameraLocation = FollowCamera->GetComponentLocation();
		bHasLevelStartCameraLocation = true;
	}

	if (InputRouter && ArmControl)
	{
		InputRouter->SetDefaultInputReceiver(ArmControl);
	}
	
}

// Called every frame
void AAMPPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AAMPPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UE_LOG(LogTemplateCharacter, Log, TEXT("[MPInput] Setup pawn=%s controller=%s mapping=%s input=%s"), *GetName(), *GetNameSafe(GetController()), *GetNameSafe(DefaultMappingContext), *GetNameSafe(PlayerInputComponent));
// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
	else
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAMPPlayerCharacter::Move);
		}
	}

	PlayerInputComponent->BindAxis(TEXT("MP_LookYaw"), this, &AAMPPlayerCharacter::HandleMiniGameLookYaw);
	PlayerInputComponent->BindAxis(TEXT("MP_LookPitch"), this, &AAMPPlayerCharacter::HandleMiniGameLookPitch);
}
	void AAMPPlayerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AAMPPlayerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AAMPPlayerCharacter::HandleMiniGameLookYaw(float Value)
{
	static bool bLoggedYaw = false;
	if (!FMath::IsNearlyZero(Value) && !bLoggedYaw)
	{
		bLoggedYaw = true;
		UE_LOG(LogTemplateCharacter, Log, TEXT("[MPInput] MP_LookYaw received. Pawn=%s Value=%f"), *GetName(), Value);
	}
	AddControllerYawInput(Value);
}

void AAMPPlayerCharacter::HandleMiniGameLookPitch(float Value)
{
	static bool bLoggedPitch = false;
	if (!FMath::IsNearlyZero(Value) && !bLoggedPitch)
	{
		bLoggedPitch = true;
		UE_LOG(LogTemplateCharacter, Log, TEXT("[MPInput] MP_LookPitch received. Pawn=%s Value=%f"), *GetName(), Value);
	}
	AddControllerPitchInput(-Value);
}

void AAMPPlayerCharacter::SetMiniGameLookCameraEnabled(bool bEnabled)
{
	if (CameraBoom && FollowCamera)
	{
		// Keep the exact view location from level start, then rotate there in place.
		// This is deliberately not an orbit around the character's root component.
		if (!bHasLevelStartCameraLocation)
		{
			LevelStartCameraLocation = FollowCamera->GetComponentLocation();
			bHasLevelStartCameraLocation = true;
		}
		CameraBoom->SetAbsolute(true, false, false);
		CameraBoom->SetWorldLocation(LevelStartCameraLocation);
		CameraBoom->TargetArmLength = 0.f;
		CameraBoom->bUsePawnControlRotation = true;
	}
	if (FollowCamera)
	{
		FollowCamera->bUsePawnControlRotation = false;
	}
	UE_LOG(LogTemplateCharacter, Log, TEXT("[MPInput] MiniGame camera input=%s boomUsesControlRotation=%s armLength=%.1f startLocation=%s"), bEnabled ? TEXT("enabled") : TEXT("locked"), CameraBoom && CameraBoom->bUsePawnControlRotation ? TEXT("true") : TEXT("false"), CameraBoom ? CameraBoom->TargetArmLength : -1.f, *LevelStartCameraLocation.ToCompactString());
}


