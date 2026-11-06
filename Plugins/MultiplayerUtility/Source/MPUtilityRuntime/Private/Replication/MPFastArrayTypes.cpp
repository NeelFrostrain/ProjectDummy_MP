// Copyright Epic Games, Inc. All Rights Reserved.

#include "Replication/MPFastArrayTypes.h"

bool FMPReplicatedKeyValueArray::SetValue(FName Key, const FString& Value, double ServerTime)
{
	if (Key.IsNone())
	{
		return false;
	}

	for (FMPReplicatedKeyValuePair& Item : Items)
	{
		if (Item.Key == Key)
		{
			if (Item.Value != Value)
			{
				Item.Value = Value;
				Item.LastModifiedServerTime = ServerTime;
				MarkItemDirty(Item);
				return true;
			}
			return false; // Value unchanged
		}
	}

	// Add new item
	FMPReplicatedKeyValuePair& NewItem = Items.Emplace_GetRef(Key, Value);
	NewItem.LastModifiedServerTime = ServerTime;
	MarkItemDirty(NewItem);
	return true;
}

bool FMPReplicatedKeyValueArray::RemoveKey(FName Key)
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].Key == Key)
		{
			Items.RemoveAt(i);
			MarkArrayDirty();
			return true;
		}
	}
	return false;
}

bool FMPReplicatedKeyValueArray::GetValue(FName Key, FString& OutValue) const
{
	for (const FMPReplicatedKeyValuePair& Item : Items)
	{
		if (Item.Key == Key)
		{
			OutValue = Item.Value;
			return true;
		}
	}
	return false;
}

bool FMPReplicatedKeyValueArray::ContainsKey(FName Key) const
{
	for (const FMPReplicatedKeyValuePair& Item : Items)
	{
		if (Item.Key == Key)
		{
			return true;
		}
	}
	return false;
}

void FMPReplicatedKeyValueArray::Clear()
{
	if (Items.Num() > 0)
	{
		Items.Empty();
		MarkArrayDirty();
	}
}

