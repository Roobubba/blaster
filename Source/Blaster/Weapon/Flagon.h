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

private:

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* FlagonMesh;
};
