// LeaveMeAlone Game by Netologiya. All RightsReserved.

#include "Player/LMADefaultCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/LMAHealthComponent.h"
#include "Components/LMAWeaponComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h" // Важно! Включаем компонент движения
#include "Engine/Engine.h"							  // Для отладочных сообщений на экране

// Устанавливаем значения по умолчанию
ALMADefaultCharacter::ALMADefaultCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArmComponent->SetupAttachment(GetRootComponent());
	SpringArmComponent->SetUsingAbsoluteRotation(true);
	SpringArmComponent->TargetArmLength = ArmLength;
	TargetZoomArmLength = ArmLength;
	SpringArmComponent->SetRelativeRotation(FRotator(YRotation, 0.0f, 0.0f));
	SpringArmComponent->bDoCollisionTest = false;
	SpringArmComponent->bEnableCameraLag = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->SetFieldOfView(FOV);
	CameraComponent->bUsePawnControlRotation = false;

	HealthComponent = CreateDefaultSubobject<ULMAHealthComponent>("HealthComponent");

	WeaponComponent = CreateDefaultSubobject<ULMAWeaponComponent>("Weapon");

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// --- Инициализация для спринта в конструкторе ---
	// Получаем ссылку на компонент движения персонажа и устанавливаем его начальную скорость.
	// CharacterMovementComponent автоматически создается для ACharacter.
	if (GetCharacterMovement())
	{
		BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed; // Сохраняем базовую скорость
	}
	else
	{
		BaseWalkSpeed = 600.0f; // Запасное значение
	}

	CurrentStamina = MaxStamina; // Начинаем с полной выносливостью
}

// Вызываем когда игра начинается или происходит спаун
void ALMADefaultCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (SpringArmComponent)
	{
		TargetZoomArmLength = SpringArmComponent->TargetArmLength;
	}
	if (CursorMaterial)
	{
		CurrentCursor = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CursorMaterial, CursorSize, FVector(0));
	}

	OnHealthChanged(HealthComponent->GetHealth());
	HealthComponent->OnDeath.AddUObject(this, &ALMADefaultCharacter::OnDeath);
	HealthComponent->OnHealthChanged.AddUObject(this, &ALMADefaultCharacter::OnHealthChanged);

	// Убедимся, что BaseWalkSpeed правильно установлен из CharacterMovementComponent.
	// Это важно, если вы меняете скорость в Blueprint-е.
	if (GetCharacterMovement())
	{
		BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}
}

// Called every frame
void ALMADefaultCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth zoom: interpolate current arm length to desired target (уже было)
	if (SpringArmComponent)
	{
		const float MinLen = FMath::Min(MinZoomArmLength, MaxZoomArmLength);
		const float MaxLen = FMath::Max(MinZoomArmLength, MaxZoomArmLength);
		TargetZoomArmLength = FMath::Clamp(TargetZoomArmLength, MinLen, MaxLen);

		SpringArmComponent->TargetArmLength =
			FMath::FInterpTo(SpringArmComponent->TargetArmLength, TargetZoomArmLength, DeltaTime, ZoomInterpSpeed);
	}

	if (!(HealthComponent->IsDead()))
	{
		RotationPlayerOnCursor();
		UpdateStamina(DeltaTime); // Обновляем выносливость каждый кадр
	}
	else
	{
		// Если персонаж мертв, убедимся, что он не спринтует и его скорость нормальная.
		if (MovementState == EMovementState::Sprinting)
		{
			StopSprint();
		}
	}
}

// Called to bind functionality to input
void ALMADefaultCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ALMADefaultCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ALMADefaultCharacter::MoveRight);
	PlayerInputComponent->BindAxis("CameraZoom", this, &ALMADefaultCharacter::CameraZoom);

	// --- Привязка новых действий для спринта ---
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ALMADefaultCharacter::StartSprint); // При нажатии
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ALMADefaultCharacter::StopSprint); // При отпускании

	// --- Привязка выстрела
	PlayerInputComponent->BindAction("Fire", IE_Pressed, WeaponComponent, &ULMAWeaponComponent::Fire);
	PlayerInputComponent->BindAction("Fire", IE_Released, WeaponComponent, &ULMAWeaponComponent::StopFire);

	// --- Перезарядка
	PlayerInputComponent->BindAction("Reload", IE_Pressed, WeaponComponent, &ULMAWeaponComponent::Reload);
}

void ALMADefaultCharacter::MoveForward(float Value)
{
	// Мы позволяем двигаться вперед, даже если значение Value отрицательное (движение назад).
	// Главное, чтобы персонаж не был мертв.
	if (!HealthComponent->IsDead())
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}
void ALMADefaultCharacter::MoveRight(float Value)
{
	if (!HealthComponent->IsDead())
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ALMADefaultCharacter::CameraZoom(float Value)
{
	if (!SpringArmComponent || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const float MinLen = FMath::Min(MinZoomArmLength, MaxZoomArmLength);
	const float MaxLen = FMath::Max(MinZoomArmLength, MaxZoomArmLength);

	TargetZoomArmLength = FMath::Clamp(TargetZoomArmLength - Value * ZoomStep, MinLen, MaxLen);
}

void ALMADefaultCharacter::OnDeath()
{
	if (bDeathScreenShown)
	{
		return;
	}

	if (CurrentCursor)
	{
		CurrentCursor->DestroyRenderState_Concurrent(); // Деактивируем курсор при смерти
	}

	PlayAnimMontage(DeathMontage); // Проигрываем анимацию смерти

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement(); // Отключаем движение
	}

	SetLifeSpan(5.0f); // Персонаж исчезнет через 5 секунд

	if (Controller)
	{
		Controller->ChangeState(NAME_Spectating); // Переводим контроллер в режим наблюдателя
	}

	StopSprint(); // Убедимся, что спринт прекращен при смерти

	// Показать экран смерти через 2 секунды ---
	GetWorldTimerManager().ClearTimer(DeathMenuTimer);
	GetWorldTimerManager().SetTimer(DeathMenuTimer, this, &ALMADefaultCharacter::ShowDeathScreen, DeathScreenDelay, false);
}

void ALMADefaultCharacter::ShowDeathScreen()
{
	if (bDeathScreenShown)
	{
		return;
	}
	bDeathScreenShown = true;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !DeathScreenClass)
	{
		return;
	}

	// Если вдруг уже был виджет — уберём
	if (DeathScreenWidget)
	{
		DeathScreenWidget->RemoveFromParent();
		DeathScreenWidget = nullptr;
	}

	DeathScreenWidget = CreateWidget<UUserWidget>(PC, DeathScreenClass);
	if (!DeathScreenWidget)
	{
		return;
	}

	DeathScreenWidget->AddToViewport(500);

	//"заморозить" игру или просто отдать ввод в UI
	UGameplayStatics::SetGamePaused(this, true);

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(DeathScreenWidget->TakeWidget());
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;
}

void ALMADefaultCharacter::OnHealthChanged(float NewHealth)
{
	// Отладочное сообщение о здоровье
	//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Health = %f"), NewHealth));
}

void ALMADefaultCharacter::RotationPlayerOnCursor()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && CurrentCursor) // Проверяем, что есть и контроллер, и курсор
	{
		FHitResult ResultHit;
		// Трассировка луча под курсором, чтобы получить WorldLocation
		PC->GetHitResultUnderCursor(ECC_GameTraceChannel1, true, ResultHit);

		// Если курсор попал куда-то в мир
		if (ResultHit.bBlockingHit)
		{
			// Вычисляем угол поворота персонажа, чтобы смотреть на точку попадания курсора
			float FindRotatorResultYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ResultHit.Location).Yaw;
			SetActorRotation(FQuat(FRotator(0.0f, FindRotatorResultYaw, 0.0f)));
			CurrentCursor->SetWorldLocation(ResultHit.Location); // Обновляем положение декали курсора
		}
	}
}

// --- Реализация методов спринта ---

void ALMADefaultCharacter::StopSprint()
{
	// Всегда устанавливаем состояние Normal при вызове StopSprint,
	// это предотвратит проблемы, если мы вызываем StopSprint из CanSprint() или OnDeath()
	// и текущее состояние уже было Normal.
	MovementState = EMovementState::Normal;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Sprinting Stopped"));
}

bool ALMADefaultCharacter::IsMoving() const
{
	// Проверяем, есть ли ненулевая скорость.
	// FVector::ZeroVector обычно означает, что персонаж неподвижен.
	// Small number используется для проверки на "почти ноль", так как float-значения могут быть не точно нулевыми.
	return GetCharacterMovement() && GetCharacterMovement()->Velocity.SizeSquared() > KINDA_SMALL_NUMBER;
}

void ALMADefaultCharacter::StartSprint()
{
	// Проверяем общие условия для спринта (не мертв, есть стамина)
	if (!CanSprint() || HealthComponent->IsDead())
	{
		// Если не можем спринтовать, убедимся, что состояние не "Sprinting"
		// (на случай, если вдруг вызвали StartSprint(), когда уже были на грани)
		if (MovementState == EMovementState::Sprinting)
		{
			StopSprint();
		}
		return;
	}

	// Если персонаж не движется, не начинаем спринт, но и не останавливаем текущее движение (потому что его нет).
	// Стамина не будет расходоваться, потому что MovementState останется Normal.
	if (!IsMoving())
	{
		// GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, TEXT("Can't sprint while stationary."));
		// Важно: если игрок стоит и жмет Shift, мы не переходим в состояние спринта.
		// Но если он отпустит Shift, а потом нажмет, мы не должны вызывать StopSprint, если уже Normal.
		if (MovementState == EMovementState::Sprinting)
		{
			StopSprint(); // Если каким-то образом был в состоянии спринта, когда стоял, прекращаем.
		}
		return;
	}

	// Если уже спринтуем, ничего не делаем
	if (MovementState == EMovementState::Sprinting)
	{
		return;
	}

	// Все проверки пройдены: персонаж движется и может спринтовать.
	MovementState = EMovementState::Sprinting;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SprintSpeedMultiplier;
	}
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Sprinting Started"));
}

void ALMADefaultCharacter::UpdateStamina(float DeltaTime)
{
	// Изменяем условие: тратим стамину только если персонаж в состоянии спринта И он движется.
	if (MovementState == EMovementState::Sprinting && IsMoving())
	{
		CurrentStamina = FMath::FInterpTo(CurrentStamina, 0.0f, DeltaTime, StaminaDrainRate);

		if (CurrentStamina <= 0.0f)
		{
			StopSprint();
		}
		GEngine->AddOnScreenDebugMessage(
			-1, 0.0f, FColor::Blue, FString::Printf(TEXT("Stamina: %.1f / %.1f (Draining)"), CurrentStamina, MaxStamina));
	}
	else // Не спринтуем ИЛИ спринтуем, но стоим на месте - восстанавливаем выносливость
	{
		CurrentStamina = FMath::FInterpTo(CurrentStamina, MaxStamina, DeltaTime, StaminaRecoveryRate);
		GEngine->AddOnScreenDebugMessage(
			-1, 0.0f, FColor::Cyan, FString::Printf(TEXT("Stamina: %.1f / %.1f (Recovering)"), CurrentStamina, MaxStamina));
	}

	CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
}

bool ALMADefaultCharacter::CanSprint() const
{
	// Персонаж может спринтовать, если он не мертв и у него есть выносливость.
	// (Условие движения уже обрабатывается в MovementInput, здесь только проверка для спринта).
	return !HealthComponent->IsDead() && CurrentStamina > 0.0f;
}