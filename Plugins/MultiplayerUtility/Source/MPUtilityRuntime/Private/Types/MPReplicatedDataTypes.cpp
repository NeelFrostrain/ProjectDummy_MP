#include "Types/MPReplicatedDataTypes.h"

void FMPReplicatedKeyValue::SetString(const FString& Key, const FString& Val)
{
    for (FMPReplicatedKeyValueItem& Item : Items)
    {
        if (Item.Key == Key)
        {
            Item.StringValue = Val;
            MarkItemDirty(Item);
            return;
        }
    }

    FMPReplicatedKeyValueItem& NewItem = Items.AddDefaulted_GetRef();
    NewItem.Key = Key;
    NewItem.StringValue = Val;
    MarkItemDirty(NewItem);
}

FString FMPReplicatedKeyValue::GetString(const FString& Key, const FString& DefaultVal) const
{
    for (const FMPReplicatedKeyValueItem& Item : Items)
    {
        if (Item.Key == Key)
        {
            return Item.StringValue;
        }
    }
    return DefaultVal;
}

void FMPReplicatedKeyValue::SetFloat(const FString& Key, float Val)
{
    for (FMPReplicatedKeyValueItem& Item : Items)
    {
        if (Item.Key == Key)
        {
            Item.FloatValue = Val;
            MarkItemDirty(Item);
            return;
        }
    }

    FMPReplicatedKeyValueItem& NewItem = Items.AddDefaulted_GetRef();
    NewItem.Key = Key;
    NewItem.FloatValue = Val;
    MarkItemDirty(NewItem);
}

float FMPReplicatedKeyValue::GetFloat(const FString& Key, float DefaultVal) const
{
    for (const FMPReplicatedKeyValueItem& Item : Items)
    {
        if (Item.Key == Key)
        {
            return Item.FloatValue;
        }
    }
    return DefaultVal;
}

void FMPReplicatedKeyValue::SetInt(const FString& Key, int32 Val)
{
    for (FMPReplicatedKeyValueItem& Item : Items)
    {
        if (Item.Key == Key)
        {
            Item.IntValue = Val;
            MarkItemDirty(Item);
            return;
        }
    }

    FMPReplicatedKeyValueItem& NewItem = Items.AddDefaulted_GetRef();
    NewItem.Key = Key;
    NewItem.IntValue = Val;
    MarkItemDirty(NewItem);
}

int32 FMPReplicatedKeyValue::GetInt(const FString& Key, int32 DefaultVal) const
{
    for (const FMPReplicatedKeyValueItem& Item : Items)
    {
        if (Item.Key == Key)
        {
            return Item.IntValue;
        }
    }
    return DefaultVal;
}

bool FMPReplicatedKeyValue::Remove(const FString& Key)
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

void FMPReplicatedKeyValue::Empty()
{
    Items.Empty();
    MarkArrayDirty();
}

