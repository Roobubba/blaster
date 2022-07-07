#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
    EWT_Pistol UMETA(DisplayName = "Pistol"),
    EWT_Grenade UMETA(DisplayName = "Grenade"),
    EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
    EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),
    EWT_Shotgun UMETA(DisplayName = "Shotgun"),
    EWT_MAX UMETA(DisplayName = "DefaultMAX")
};