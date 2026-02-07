// LeaveMeAlone Game by Netologiya. All RightsReserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LMADefaultCharacter.generated.h"

class UCameraComponent;
class UDecalComponent;
class USpringArmComponent;
class ULMAHealthComponent;
class UCharacterMovementComponent; // Добавлено для удобства доступа к компоненту движения

// Определение перечисления для состояния движения.
// Это позволяет легко отслеживать, чем занимается персонаж (нормальное движение, спринт и т.д.).
UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Normal UMETA(DisplayName = "Normal"),	   // Обычное движение
	Sprinting UMETA(DisplayName = "Sprinting") // Спринт
};

UCLASS()
class LEAVEMEALONE_API ALMADefaultCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Конструктор, вызывается при создании экземпляра персонажа
	ALMADefaultCharacter();

	// Функция для получения компонента здоровья (уже была)
	UFUNCTION()
	ULMAHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	// Компоненты персонажа (уже были)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components|Health")
	ULMAHealthComponent* HealthComponent;

	UPROPERTY()
	UDecalComponent* CurrentCursor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
	UMaterialInterface* CursorMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
	FVector CursorSize = FVector(20.0f, 40.0f, 40.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* DeathMontage;

	// --- Новые переменные для спринта ---

	// Множитель скорости спринта: на сколько увеличится скорость при спринте.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "1.0", ClampMax = "5.0"))
	float SprintSpeedMultiplier = 1.5f; // Персонаж будет двигаться в 1.5 раза быстрее

	// Максимальное значение выносливости.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "1.0"))
	float MaxStamina = 100.0f;

	// Текущее значение выносливости. VisibleAnywhere, чтобы видеть в редакторе, но не редактировать.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	float CurrentStamina = 0.0f; // Инициализируется в конструкторе

	// Скорость траты выносливости в секунду при спринте.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "0.1"))
	float StaminaDrainRate = 10.0f; // 10 единиц выносливости в секунду

	// Скорость восстановления выносливости в секунду, когда не спринтуем.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "0.1"))
	float StaminaRecoveryRate = 5.0f; // 5 единиц выносливости в секунду

	// Текущее состояние движения персонажа.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	EMovementState MovementState = EMovementState::Normal; // По умолчанию обычное движение

	// Скорость ходьбы по умолчанию, чтобы можно было вернуться к ней после спринта.
	float BaseWalkSpeed = 600.0f; // Типичное значение для персонажей в UE

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// Переменные для камеры (уже были)
	float YRotation = -75.0f;
	float ArmLength = 1400.0f;
	float FOV = 55.0f;

	/** Camera zoom settings (editable in UE Editor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom",
		meta = (ClampMin = "300.0", ClampMax = "3000.0", UIMin = "300.0", UIMax = "3000.0", AllowPrivateAccess = "true"))
	float MinZoomArmLength = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom",
		meta = (ClampMin = "300.0", ClampMax = "3000.0", UIMin = "300.0", UIMax = "3000.0", AllowPrivateAccess = "true"))
	float MaxZoomArmLength = 2200.0f;

	/** How much arm length changes per one wheel step (axis value ~ +/-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom",
		meta = (ClampMin = "10.0", ClampMax = "1000.0", UIMin = "10.0", UIMax = "1000.0", AllowPrivateAccess = "true"))
	float ZoomStep = 150.0f;

	/** Smoothness: higher = faster interpolation to target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom",
		meta = (ClampMin = "1.0", ClampMax = "30.0", UIMin = "1.0", UIMax = "30.0", AllowPrivateAccess = "true"))
	float ZoomInterpSpeed = 10.0f;

	/** Desired arm length (we smoothly interpolate to it in Tick) */
	float TargetZoomArmLength = 1400.0f;

	// Методы движения и камеры (уже были)
	void MoveForward(float Value);
	void MoveRight(float Value);
	void CameraZoom(float Value);

	// Методы здоровья (уже были)
	void OnDeath();
	void OnHealthChanged(float NewHealth);

	// Метод поворота (уже был)
	void RotationPlayerOnCursor();

	// --- Новые методы для спринта ---

	// Вызывается при нажатии клавиши "Sprint".
	void StartSprint();
	// Вызывается при отпускании клавиши "Sprint".
	void StopSprint();
	// Обновляет выносливость (тратит или восстанавливает).
	void UpdateStamina(float DeltaTime);
	// Проверяет, может ли персонаж сейчас спринтовать.
	bool CanSprint() const;

	bool IsMoving() const;
};