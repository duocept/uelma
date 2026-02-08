// LeaveMeAlone Game by Netologiya. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <Weapon/LMABaseWeapon.h>
#include "LMAWeaponComponent.generated.h"

class ALMABaseWeapon;
class UAnimMontage;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LEAVEMEALONE_API ULMAWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULMAWeaponComponent();

	// Action Mapping: Fire (Pressed) — запускаем автострельбу по таймеру.
	void Fire();

	// Action Mapping: Fire (Released) — останавливаем таймер автострельбы.
	void StopFire();

	// Action Mapping: Reload (Pressed) — ручная перезарядка.
	void Reload();

	// --- Добавил
	UFUNCTION(BlueprintCallable)
	bool GetCurrentWeaponAmmo(FAmmoWeapon& AmmoWeapon) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<ALMABaseWeapon> WeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	UAnimMontage* ReloadMontage;

	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	ALMABaseWeapon* Weapon = nullptr;

	// Флаг: сейчас играет анимация перезарядки.
	bool AnimReloading = false;

	// Флаг: кнопка стрельбы зажата (важно для продолжения стрельбы после автоперезарядки).
	bool bWantsToFire = false;

	FTimerHandle FireTimerHandle;

	void SpawnWeapon();
	void InitAnimNotify();

	// Таймерный выстрел.
	void MakeShot();

	// Внутренняя перезарядка (вызывается и на ручной Reload, и при опустошении магазина).
	void StartReload();

	// Коллбек из AnimNotify.
	void OnNotifyReloadFinished(USkeletalMeshComponent* SkeletalMesh);

	// Делегат оружия: магазин опустел.
	void OnWeaponClipEmpty();

	bool CanReload() const;
};