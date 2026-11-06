#include "WorldInventoryItem.h"
#include "InventoryItemData.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AWorldInventoryItem::AWorldInventoryItem()
{
    bReplicates = true;
    SetReplicateMovement(true);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    Mesh->SetSimulatePhysics(true);
    bDestroyOnEmpty = false;
}

void AWorldInventoryItem::BeginPlay()
{
    Super::BeginPlay();
    RefreshVisuals();
}

void AWorldInventoryItem::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshVisuals();
}

void AWorldInventoryItem::InitializeDrop(UInventoryItemData* InItemData, int32 InQuantity)
{
    if (!HasAuthority()) return;
    ItemData = InItemData;
    Quantity = FMath::Max(1, InQuantity);
    RefreshVisuals();
    ForceNetUpdate();
}

int32 AWorldInventoryItem::TakeQuantity(int32 RequestedAmount)
{
    if (!HasAuthority() || RequestedAmount <= 0 || Quantity <= 0) return 0;
    const int32 Taken = FMath::Min(RequestedAmount, Quantity);
    Quantity -= Taken;
    if (Quantity < 0) { Quantity = 0; }
    ForceNetUpdate();
    return Taken;
}

void AWorldInventoryItem::LaunchItem(FVector Velocity)
{
    if (Mesh && Mesh->IsSimulatingPhysics())
    {
        Mesh->AddImpulse(Velocity, NAME_None, true);
    }
}

void AWorldInventoryItem::OnRep_ItemData()
{
    RefreshVisuals();
}

void AWorldInventoryItem::OnRep_Quantity()
{
    RefreshVisuals();
}

void AWorldInventoryItem::RefreshVisuals()
{
    if (Mesh && ItemData)
    {
        if (ItemData->WorldMesh)
        {
            Mesh->SetStaticMesh(ItemData->WorldMesh);
        }
    }
}

void AWorldInventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AWorldInventoryItem, ItemData);
    DOREPLIFETIME(AWorldInventoryItem, Quantity);
}
