// Copyright Epic Games, Inc. All Rights Reserved.

#include "Requests/MPNetworkRequestTypes.h"
#include "Requests/MPNetworkRequestComponent.h"

bool UMPNetworkRequestHandle::Cancel()
{
	if (IsDone())
	{
		return false;
	}

	if (OwningComponent.IsValid())
	{
		return OwningComponent->CancelRequest(RequestId);
	}

	CachedState = EMPNetworkRequestState::Cancelled;
	return true;
}

void UMPNetworkRequestHandle::SetOwningComponent(UMPNetworkRequestComponent* InComp)
{
	OwningComponent = InComp;
}
