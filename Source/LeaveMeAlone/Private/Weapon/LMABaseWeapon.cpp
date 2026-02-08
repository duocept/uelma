// LeaveMeAlone Game by Netologiya. All Rights Reserved.

#include "Weapon/LMABaseWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogWeapon, All, All);

ALMABaseWeapon::ALMABaseWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponComponent = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	SetRootComponent(WeaponComponent);
}

void ALMABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmoWeapon = AmmoWeapon;
}

void ALMABaseWeapon::Fire()
{
	// Если магазин пуст — стрелять нельзя. Перезарядка инициируется делегатом в момент,
	// когда мы "достреляли" магазин (см. DecrementBullets()).
	if (IsCurrentClipEmpty())
	{
		return;
	}

	Shoot();
}

void ALMABaseWeapon::StopFire()
{
	// Базовое оружие ничего не делает.
	// В наследниках можно остановить Niagara/звук/анимацию и т.п.
}

void ALMABaseWeapon::Shoot()
{
	const FTransform SocketTransform = WeaponComponent->GetSocketTransform("Muzzle");
	const FVector TraceStart = SocketTransform.GetLocation();
	const FVector ShootDirection = SocketTransform.GetRotation().GetForwardVector();
	const FVector TraceEnd = TraceStart + ShootDirection * TraceDistance;

	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Black, false, 1.0f, 0, 2.0f);

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);

	if (HitResult.bBlockingHit)
	{
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 5.0f, 24, FColor::Red, false, 1.0f);
	}

	DecrementBullets();
}

void ALMABaseWeapon::DecrementBullets()
{
	CurrentAmmoWeapon.Bullets = FMath::Max(0, CurrentAmmoWeapon.Bullets - 1);
	//UE_LOG(LogWeapon, Display, TEXT("Bullets = %d"), CurrentAmmoWeapon.Bullets);

	// Когда магазин опустел — сообщаем наружу (компонент решит, перезаряжаться ли).
	if (IsCurrentClipEmpty())
	{
		OnClipEmpty.Broadcast();
	}
}

bool ALMABaseWeapon::IsCurrentClipEmpty() const
{
	return CurrentAmmoWeapon.Bullets <= 0;
}

bool ALMABaseWeapon::IsCurrentClipFull() const
{
	return CurrentAmmoWeapon.Bullets >= AmmoWeapon.Bullets;
}

bool ALMABaseWeapon::CanReload() const
{
	// Есть резервные магазины или бесконечные.
	return CurrentAmmoWeapon.Infinite || CurrentAmmoWeapon.Clips > 0;
}

void ALMABaseWeapon::ChangeClip()
{
	// Нельзя перезаряжаться, если магазин и так полный.
	if (IsCurrentClipFull())
	{
		return;
	}

	// Если нет резервных магазинов и не Infinite — нечем перезаряжаться.
	if (!CurrentAmmoWeapon.Infinite)
	{
		if (CurrentAmmoWeapon.Clips <= 0)
		{
			return;
		}
		CurrentAmmoWeapon.Clips--;
	}

	CurrentAmmoWeapon.Bullets = AmmoWeapon.Bullets;
	UE_LOG(LogWeapon, Display, TEXT("ChangeClip -> Bullets=%d, Clips=%d, Infinite=%s"), CurrentAmmoWeapon.Bullets, CurrentAmmoWeapon.Clips,
		CurrentAmmoWeapon.Infinite ? TEXT("true") : TEXT("false"));
}

void ALMABaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}