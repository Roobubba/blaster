// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Flagon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AFlagon : public AWeapon
{
	GENERATED_BODY()

public:

	AFlagon();

	virtual void Dropped() override;
	void ResetFlagon();

protected:

	virtual void OnEquipped() override;
	virtual void OnDropped() override;
	virtual void BeginPlay() override;

private:

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* FlagonMesh;

	FTransform InitialTransform;

	UPROPERTY(EditAnywhere)
	class AFlagonZone* FlagonZone;

public:

	FORCEINLINE UStaticMeshComponent* GetFlagonMesh() const { return FlagonMesh; }
	//FORCEINLINE FTransform GetInitialTransform() const { return InitialTransform; }
};
