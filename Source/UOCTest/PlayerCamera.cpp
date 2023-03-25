// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCamera.h"

#include "EnhancedInputComponent.h"
#include "Hex.h"
#include "LineTypes.h"
#include "UOCTestGameMode.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APlayerCamera::APlayerCamera() : APawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Floating Movement Component"));
	AddOwnedComponent(FloatingPawnMovement);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 2000.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera Component"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UInputAction> ClickInputAction(TEXT("/Game/TopDown/Input/Actions/IA_SetDestination_Click"));
    if (const UInputAction* ActionObject = ClickInputAction.Object)
	{
		ClickAction = ActionObject;
	}
}

// Called when the game starts or when spawned
void APlayerCamera::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APlayerCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events
		EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Started, this, &APlayerCamera::OnMouseClicked);
	}
}

void APlayerCamera::OnMouseClicked()
{
	AUOCTestGameMode* GameMode = Cast<AUOCTestGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	FVector ClickLocation = GetClickLocation();
	Hex Tile = GameMode->GridManager->WorldToHex(ClickLocation);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("TILE_%d_%d_%d"), Tile.Q, Tile.R, Tile.S));
}

FVector APlayerCamera::GetClickLocation() const
{
	FVector2D MousePosition;
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
	{
		FVector StartLocation = CameraComponent->GetComponentLocation();
		FVector WorldOrigin;
		FVector WorldDirection;
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (UGameplayStatics::DeprojectScreenToWorld(PlayerController, MousePosition, WorldOrigin, WorldDirection))
		{
			FHitResult Hit;
			bool Success = GetWorld()->LineTraceSingleByChannel(
				Hit,
				WorldOrigin,
				StartLocation + WorldDirection * 20000.0f,
				TraceChannel);

			if (Success)
			{
				return Hit.ImpactPoint;
			}
		}
		
	}

	return FVector();
}