#include "ItemPickup.h"
#include "ItemDefinition.h"
#include "ItemContainerComponent.h"
#include "ItemLibrary.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AItemPickup::AItemPickup()
{
    bReplicates = true;
    SetReplicateMovement(true);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);    // walk through it
    Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // but traces can still hit it
    Mesh->SetSimulatePhysics(false);

    AutoPickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AutoPickupSphere"));
    AutoPickupSphere->SetupAttachment(Mesh);
    AutoPickupSphere->InitSphereRadius(AutoPickupRadius);
    AutoPickupSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    AutoPickupSphere->SetGenerateOverlapEvents(false);
}

void AItemPickup::BeginPlay()
{
    Super::BeginPlay();
    RefreshVisuals();

    if (HasAuthority())
    {
        if (bSimulatePhysics && Mesh->GetStaticMesh())
        {
            Mesh->SetSimulatePhysics(true);
        }

        if (bAutoPickup)
        {
            AutoPickupSphere->SetSphereRadius(AutoPickupRadius);
            AutoPickupSphere->SetGenerateOverlapEvents(true);
            AutoPickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemPickup::OnSphereOverlap);
        }
    }
}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshVisuals();
}

void AItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AItemPickup, Item);
    DOREPLIFETIME(AItemPickup, Count);
}

void AItemPickup::OnRep_Item()
{
    RefreshVisuals();
}

void AItemPickup::RefreshVisuals()
{
    if (Mesh && Item && Item->PickupMesh)
    {
        Mesh->SetStaticMesh(Item->PickupMesh);
    }
}

void AItemPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                  int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || !OtherActor || !Item || Count <= 0) { return; }
    if (!CanBePickedUpBy(OtherActor)) { return; }

    if (UItemContainerComponent* Container = UItemLibrary::FindItemContainer(OtherActor, AutoPickupContainerName))
    {
        Container->PickupActor(this);
    }
}

int32 AItemPickup::Take(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0 || Count <= 0) { return 0; }

    const int32 Taken = FMath::Min(Amount, Count);
    Count -= Taken;
    if (Count <= 0)
    {
        Destroy();
    }
    return Taken;
}

void AItemPickup::Launch(FVector Velocity)
{
    if (Mesh && Mesh->IsSimulatingPhysics())
    {
        Mesh->AddImpulse(Velocity, NAME_None, /*bVelChange*/ true);
    }
}

AItemPickup* AItemPickup::SpawnPickup(UObject* WorldContextObject, UItemDefinition* InItem, int32 InCount, FVector Location, FVector Velocity)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    return SpawnInternal(World, InItem, InCount, Location, Velocity, true, nullptr);
}

AItemPickup* AItemPickup::SpawnInternal(UWorld* World, UItemDefinition* InItem, int32 InCount, FVector Location, FVector Velocity,
                                        bool bAllowAutoPickup, TSubclassOf<AItemPickup> ClassOverride)
{
    if (!World || World->GetNetMode() == NM_Client || !InItem || InCount <= 0) { return nullptr; }

    TSubclassOf<AItemPickup> PickupClass = InItem->PickupClass;
    if (!PickupClass) { PickupClass = ClassOverride; }
    if (!PickupClass) { PickupClass = AItemPickup::StaticClass(); }

    const FTransform Transform(FRotator::ZeroRotator, Location);
    AItemPickup* Pickup = World->SpawnActorDeferred<AItemPickup>(PickupClass, Transform, nullptr, nullptr,
                                                                 ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!Pickup) { return nullptr; }

    Pickup->Item = InItem;
    Pickup->Count = InCount;
    if (!bAllowAutoPickup) { Pickup->bAutoPickup = false; }
    Pickup->FinishSpawning(Transform);

    if (!Velocity.IsNearlyZero())
    {
        Pickup->Launch(Velocity);
    }
    return Pickup;
}
