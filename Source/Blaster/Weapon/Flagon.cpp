// Fill out your copyright notice in the Description page of Project Settings.


#include "Flagon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h"

AFlagon::AFlagon()
{
    FlagonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagonMesh"));
    SetRootComponent(FlagonMesh);
    GetAreaSphere()->SetupAttachment(FlagonMesh);
    GetPickupWidget()->SetupAttachment(FlagonMesh);
    FlagonMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    FlagonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void AFlagon::BeginPlay()
{
	Super::BeginPlay();
	InitialTransform = GetActorTransform();
}

void AFlagon::ResetFlagon()
{
	ABlasterCharacter* FlagonBearer = Cast<ABlasterCharacter>(GetOwner());

	if (FlagonBearer)
	{
		FlagonBearer->SetHoldingTheFlagon(false);
		FlagonBearer->SetOverlappingWeapon(nullptr);
		FlagonBearer->FlagonDropped();
	}

	if (!HasAuthority())
	{
		return;
	}

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	FlagonMesh->DetachFromComponent(DetachRules);
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetAreaSphere()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    
	SetWeaponState(EWeaponState::EWS_Initial);

	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
	SetActorTransform(InitialTransform);
}

void AFlagon::Dropped()
{	
	ABlasterCharacter* FlagonBearer = Cast<ABlasterCharacter>(GetOwner());

	if (FlagonBearer)
	{
		FlagonBearer->FlagonDropped();
	}

	if (!HasAuthority())
	{
		return;
	}

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	FlagonMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
    SetWeaponState(EWeaponState::EWS_Dropped);
}

void AFlagon::OnEquipped()
{
    ShowPickupWidget(false);
	if (GetAreaSphere())
	{
		GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (FlagonMesh == nullptr)
	{
		return;
	}

	FlagonMesh->SetSimulatePhysics(false);
	FlagonMesh->SetEnableGravity(false);
	FlagonMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FlagonMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
}

void AFlagon::OnDropped()
{
    if (HasAuthority() && GetAreaSphere())
	{
		GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (FlagonMesh == nullptr)
	{
		return;
	}
	
	FlagonMesh->SetSimulatePhysics(true);
	FlagonMesh->SetEnableGravity(true);
	FlagonMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FlagonMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	FlagonMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
}