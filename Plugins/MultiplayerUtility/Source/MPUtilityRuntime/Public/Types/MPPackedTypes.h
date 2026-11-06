#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MPPackedTypes.generated.h"

/**
 * Bandwidth-optimized 16-bit fixed-point replicated vector.
 * Quantizes double-precision FVector into 6 bytes (vs 24 bytes in standard UE5).
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPQuantizedVector
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Quantization")
    int32 X = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Quantization")
    int32 Y = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Quantization")
    int32 Z = 0;

    /** Scale multiplier for units per step (default 0.1 = 1mm resolution, +/- 3276 units range) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Quantization")
    float StepResolution = 0.1f;

    FMPQuantizedVector() = default;
    FMPQuantizedVector(const FVector& InVector, float InStep = 0.1f);

    FVector ToFVector() const;

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FMPQuantizedVector> : public TStructOpsTypeTraitsBase2<FMPQuantizedVector>
{
    enum
    {
        WithNetSerializer = true,
    };
};

/**
 * Bandwidth-optimized quantized rotator.
 * Compresses Pitch, Yaw, Roll into 3 bytes (1 byte per axis = 1.4 degree resolution).
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPQuantizedRotator
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Quantization")
    uint8 PitchByte = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Quantization")
    uint8 YawByte = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Quantization")
    uint8 RollByte = 0;

    FMPQuantizedRotator() = default;
    FMPQuantizedRotator(const FRotator& InRotator);

    FRotator ToFRotator() const;

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FMPQuantizedRotator> : public TStructOpsTypeTraitsBase2<FMPQuantizedRotator>
{
    enum
    {
        WithNetSerializer = true,
    };
};

/**
 * Bit-packed structure encoding up to 8 booleans into a single 1-byte integer.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPPackedBools
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Quantization")
    uint8 PackedMask = 0;

    FMPPackedBools() = default;
    explicit FMPPackedBools(uint8 InMask) : PackedMask(InMask) {}

    bool GetBool(int32 BitIndex) const
    {
        if (BitIndex < 0 || BitIndex > 7) return false;
        return (PackedMask & (1 << BitIndex)) != 0;
    }

    void SetBool(int32 BitIndex, bool bValue)
    {
        if (BitIndex < 0 || BitIndex > 7) return;
        if (bValue)
        {
            PackedMask |= (1 << BitIndex);
        }
        else
        {
            PackedMask &= ~(1 << BitIndex);
        }
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << PackedMask;
        bOutSuccess = true;
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FMPPackedBools> : public TStructOpsTypeTraitsBase2<FMPPackedBools>
{
    enum
    {
        WithNetSerializer = true,
    };
};

/**
 * Blueprint helper nodes for creating and extracting quantized network structs.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPPackedTypesLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta = (DisplayName = "Make Quantized Vector", Keywords = "quantize vector compress pack"))
    static FMPQuantizedVector MakeQuantizedVector(const FVector& InVector, float StepResolution = 0.1f);

    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta = (DisplayName = "Break Quantized Vector", Keywords = "quantize vector decompress unpack"))
    static FVector BreakQuantizedVector(const FMPQuantizedVector& InQuantizedVector);

    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta = (DisplayName = "Make Quantized Rotator", Keywords = "quantize rotator compress pack"))
    static FMPQuantizedRotator MakeQuantizedRotator(const FRotator& InRotator);

    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta = (DisplayName = "Break Quantized Rotator", Keywords = "quantize rotator decompress unpack"))
    static FRotator BreakQuantizedRotator(const FMPQuantizedRotator& InQuantizedRotator);

    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta = (DisplayName = "Make Packed Bools", Keywords = "pack bools bitmask byte"))
    static FMPPackedBools MakePackedBools(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7);

    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta = (DisplayName = "Break Packed Bools", Keywords = "unpack bools bitmask byte"))
    static void BreakPackedBools(const FMPPackedBools& InPacked, bool& b0, bool& b1, bool& b2, bool& b3, bool& b4, bool& b5, bool& b6, bool& b7);

    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta = (DisplayName = "Get Packed Bool Index", Keywords = "get bit bool bitmask"))
    static bool GetPackedBoolByIndex(const FMPPackedBools& InPacked, int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Quantization", meta = (DisplayName = "Set Packed Bool Index", Keywords = "set bit bool bitmask"))
    static void SetPackedBoolByIndex(UPARAM(ref) FMPPackedBools& InPacked, int32 Index, bool bValue);
};
