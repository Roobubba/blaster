// Fill out your copyright notice in the Description page of Project Settings.


#include "Flagon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

AFlagon::AFlagon()
{
    FlagonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagonMesh"));
    SetRootComponent(FlagonMesh);
    GetAreaSphere()->SetupAttachment(FlagonMesh);
    GetPickupWidget()->SetupAttachment(FlagonMesh);
    FlagonMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    FlagonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}