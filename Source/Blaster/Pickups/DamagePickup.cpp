// Fill out your copyright notice in the Description page of Project Settings.


#include "DamagePickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/BuffComponent.h"

ADamagePickup::ADamagePickup()
{
    bReplicates = true;
}

void ADamagePickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult	)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (BlasterCharacter)
    {
        UBuffComponent* Buff = BlasterCharacter->GetBuffComponent();
        if (Buff)
        {
            Buff->ApplyDamageBuff(DamageBuffMultiplier, BuffTime);
            Destroy();
        }
    }
}