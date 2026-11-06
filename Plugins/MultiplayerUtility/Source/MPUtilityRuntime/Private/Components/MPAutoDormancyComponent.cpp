#include "Components/MPAutoDormancyComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MPUtilityRuntimeModule.h"

UMPAutoDormancyComponent::UMPAutoDormancyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UMPAutoDormancyComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
    {
        LastRecordedLocation = Owner->GetActorLocation();

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                TimerHandle_Dormancy,
                this,
                &UMPAutoDormancyComponent::CheckIdleAndSleep,
                FMath::Max(1.0f, IdleSecondsBeforeDormancy),
                true
            );
        }
    }
}

void UMPAutoDormancyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimerHandle_Dormancy);
    }
    Super::EndPlay(EndPlayReason);
}

void UMPAutoDormancyComponent::NotifyActivity()
{
    WakeFromDormancy();
}

void UMPAutoDormancyComponent::WakeFromDormancy()
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        return;
    }

    if (Owner->NetDormancy > DORM_Awake)
    {
        Owner->SetNetDormancy(DORM_Awake);
        Owner->FlushNetDormancy();
    }

    LastRecordedLocation = Owner->GetActorLocation();

    // Restart idle check timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TimerHandle_Dormancy,
            this,
            &UMPAutoDormancyComponent::CheckIdleAndSleep,
            FMath::Max(1.0f, IdleSecondsBeforeDormancy),
            true
        );
    }
}

void UMPAutoDormancyComponent::CheckIdleAndSleep()
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        return;
    }

    const FVector CurrentLocation = Owner->GetActorLocation();
    if (FVector::DistSquared(CurrentLocation, LastRecordedLocation) > 1.0)
    {
        // Actor moved; update location and do not sleep yet
        LastRecordedLocation = CurrentLocation;
        return;
    }

    // Actor has been stationary for idle duration; put to sleep
    if (Owner->NetDormancy != TargetDormancy)
    {
        Owner->SetNetDormancy(TargetDormancy);
    }
}

