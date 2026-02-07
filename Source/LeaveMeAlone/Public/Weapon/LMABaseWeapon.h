// LeaveMeAlone Game by Netologiya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LMABaseWeapon.generated.h"

class USkeletalMeshComponent;

// Делегат: в текущем магазине закончились патроны.
DECLARE_MULTICAST_DELEGATE(FOnClipEmptySignature);

USTRUCT(BlueprintType)
struct FAmmoWeapon
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	int32 Bullets;

	// Количество дополнительных магазинов (резерв). Текущий магазин не учитывается.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	int32 Clips;

	// Бесконечные магазины (резерв не уменьшается).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	bool Infinite;
};

UCLASS()
class LEAVEMEALONE_API ALMABaseWeapon : public AActor
{
	GENERATED_BODY()

public:
	ALMABaseWeapon();

	// Нажатие / автоспуск: один выстрел (вызывается по таймеру из WeaponComponent).
	virtual void Fire();

	// Отпускание кнопки стрельбы (на будущее: остановка FX/звуков).
	virtual void StopFire();

	// Смена магазина (перезарядка) — оставьте public (по заданию).
	void ChangeClip();

	// Делегат: закончились патроны в текущем магазине.
	FOnClipEmptySignature OnClipEmpty;

	// Темп стрельбы (секунды между выстрелами).
	float GetTimeBetweenShots() const { return TimeBetweenShots; }

	// Логические проверки для компонента оружия.
	bool CanReload() const;
	bool IsCurrentClipEmpty() const;
	bool IsCurrentClipFull() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	USkeletalMeshComponent* WeaponComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float TraceDistance = 800.0f;

	// Сколько секунд между выстрелами при зажатой кнопке.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float TimeBetweenShots = 0.12f;

	// Стартовый боекомплект: Bullets — размер магазина, Clips — запас магазинов.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FAmmoWeapon AmmoWeapon{30, 0, true};

	virtual void BeginPlay() override;

	void Shoot();
	void DecrementBullets();

public:
	virtual void Tick(float DeltaTime) override;

private:
	FAmmoWeapon CurrentAmmoWeapon;
};