// LeaveMeAlone Game by Netologiya. All Rights Reserved.

#include "Components/LMAWeaponComponent.h"
#include "Animations/LMAReloadFinishedAnimNotify.h"
#include "GameFramework/Character.h"
#include "Weapon/LMABaseWeapon.h"
#include "TimerManager.h"

ULMAWeaponComponent::ULMAWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULMAWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	SpawnWeapon();
	InitAnimNotify();
}

void ULMAWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void ULMAWeaponComponent::SpawnWeapon()
{
	Weapon = GetWorld()->SpawnActor<ALMABaseWeapon>(WeaponClass);
	if (!Weapon)
	{
		return;
	}

	// Подписка на делегат: опустошение магазина.
	Weapon->OnClipEmpty.AddUObject(this, &ULMAWeaponComponent::OnWeaponClipEmpty);

	if (const auto Character = Cast<ACharacter>(GetOwner()))
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, false);
		Weapon->AttachToComponent(Character->GetMesh(), AttachmentRules, "r_Weapon_Socket");
	}
}

void ULMAWeaponComponent::InitAnimNotify()
{
	if (!ReloadMontage)
		return;

	const auto& NotifiesEvents = ReloadMontage->Notifies;
	for (const auto& NotifyEvent : NotifiesEvents)
	{
		if (auto* ReloadFinish = Cast<ULMAReloadFinishedAnimNotify>(NotifyEvent.Notify))
		{
			ReloadFinish->OnNotifyReloadFinished.AddUObject(this, &ULMAWeaponComponent::OnNotifyReloadFinished);
			break;
		}
	}
}

void ULMAWeaponComponent::Fire()
{
	bWantsToFire = true;

	if (!Weapon || AnimReloading)
	{
		return;
	}

	// Если уже стреляем по таймеру — не переустанавливаем.
	if (GetWorld()->GetTimerManager().IsTimerActive(FireTimerHandle))
	{
		return;
	}

	// Первый выстрел сразу.
	MakeShot();

	// Дальше — по таймеру.
	const float Interval = FMath::Max(0.01f, Weapon->GetTimeBetweenShots());
	GetWorld()->GetTimerManager().SetTimer(FireTimerHandle, this, &ULMAWeaponComponent::MakeShot, Interval, true, Interval);
}

void ULMAWeaponComponent::StopFire()
{
	bWantsToFire = false;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
	}

	if (Weapon)
	{
		Weapon->StopFire();
	}
}

void ULMAWeaponComponent::MakeShot()
{
	if (!Weapon || AnimReloading)
	{
		return;
	}

	// Если магазин пуст, то вместо "щёлк-щёлк" инициируем перезарядку (если можно)
	// и останавливаем автострельбу на время анимации.
	if (Weapon->IsCurrentClipEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
		StartReload();
		return;
	}

	Weapon->Fire();
}

void ULMAWeaponComponent::OnWeaponClipEmpty()
{
	// Магазин закончился в момент последнего выстрела.
	// Останавливаем автострельбу и запускаем перезарядку (если есть запас).
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
	}

	StartReload();
}

bool ULMAWeaponComponent::CanReload() const
{
	if (AnimReloading || !Weapon || !ReloadMontage)
	{
		return false;
	}

	// Нельзя перезаряжаться, если магазин полный.
	if (Weapon->IsCurrentClipFull())
	{
		return false;
	}

	// Нельзя перезаряжаться, если нет резервных магазинов (и не Infinite).
	return Weapon->CanReload();
}

void ULMAWeaponComponent::Reload()
{
	// По заданию: Reload-событие остаётся, но вся логика — внутри StartReload().
	StartReload();
}

void ULMAWeaponComponent::StartReload()
{
	if (!CanReload())
	{
		return;
	}

	// Остановить стрельбу на время перезарядки, но bWantsToFire НЕ сбрасываем:
	// если игрок держит кнопку, стрельба возобновится после перезарядки.
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
	}

	AnimReloading = true;

	if (auto* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->PlayAnimMontage(ReloadMontage);
	}
}

void ULMAWeaponComponent::OnNotifyReloadFinished(USkeletalMeshComponent* SkeletalMesh)
{
	const auto Character = Cast<ACharacter>(GetOwner());
	if (!Character || Character->GetMesh() != SkeletalMesh)
	{
		return;
	}

	// На нотифай: фактически меняем магазин.
	if (Weapon)
	{
		Weapon->ChangeClip();
	}

	AnimReloading = false;

	// Если игрок всё ещё держит кнопку стрельбы — продолжаем стрелять.
	if (bWantsToFire)
	{
		Fire();
	}
}