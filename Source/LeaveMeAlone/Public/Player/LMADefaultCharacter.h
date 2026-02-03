// LeaveMeAlone Game by Netologiya. All RightsReserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LMADefaultCharacter.generated.h"

class UCameraComponent;
class UDecalComponent;
class USpringArmComponent;

UCLASS()
class LEAVEMEALONE_API ALMADefaultCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALMADefaultCharacter();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY()
	UDecalComponent* CurrentCursor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
	UMaterialInterface* CursorMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
	FVector CursorSize = FVector(20.0f, 40.0f, 40.0f);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
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

	void MoveForward(float Value);
	void MoveRight(float Value);
	void CameraZoom(float Value);
};
