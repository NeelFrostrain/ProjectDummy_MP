#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MPQuantization.generated.h"

UCLASS()
class MPUTILITYRUNTIME_API UMPQuantization : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Quantizes a float value to the specified step size (e.g. 0.1 for 1 decimal place).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization")
    static float QuantizeFloat(float Value, float StepSize = 0.1f);

    /**
     * Quantizes an FVector to the specified spatial step grid in units.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization")
    static FVector QuantizeVector(const FVector& Value, float GridSize = 1.0f);

    /**
     * Quantizes an FRotator to discrete angle steps (e.g. 0.5 degrees).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization")
    static FRotator QuantizeRotator(const FRotator& Value, float DegreeStep = 0.5f);

    /**
     * Compresses a normalized vector (-1..1) into 3 signed 8-bit bytes (3 bytes total).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization")
    static void CompressNormalizedVectorToBytes(const FVector& InVector, uint8& OutX, uint8& OutY, uint8& OutZ);

    /**
     * Decompresses 3 bytes back into a normalized vector.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization")
    static FVector DecompressNormalizedVectorFromBytes(uint8 InX, uint8 InY, uint8 InZ);

    /**
     * Returns true only if the difference between OldVal and NewVal exceeds the threshold.
     * Prevents unnecessary network replication when values fluctuate minimally.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization")
    static bool IsValueChangedEnoughFloat(float OldVal, float NewVal, float Threshold = 0.05f);

    /**
     * Returns true only if the distance between OldVal and NewVal exceeds ThresholdDistance.
     * Prevents redundant network replication when 3D positions fluctuate minimally.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization")
    static bool IsValueChangedEnoughVector(const FVector& OldVal, const FVector& NewVal, float ThresholdDistance = 2.0f);

    /**
     * Generates a deterministic integer hash from a player network string.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization")
    static int32 HashPlayerIdToInt(const FString& InPlayerId);

    /**
     * Generates an incremental unique 32-bit transaction request ID.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Quantization")
    static int32 GenerateUniqueNetRequestId();

    /**
     * Safely wraps an angle in degrees into [-180.0, 180.0).
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Math", meta=(DisplayName="Wrap Angle Safe", Keywords="wrap angle normalize 180 rotator degrees safe"))
    static float WrapAngleSafe(float AngleDegrees);

    /**
     * Compares two vectors with a Euclidean distance tolerance.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Math", meta=(DisplayName="Is Vector Within Tolerance", Keywords="vector compare tolerance distance equal"))
    static bool IsVectorWithinTolerance(const FVector& A, const FVector& B, float Tolerance = 1.0f);

    /**
     * Compares two rotators with an angular tolerance in degrees per axis (Pitch, Yaw, Roll).
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Math", meta=(DisplayName="Is Rotator Within Tolerance", Keywords="rotator compare tolerance angle equal degrees"))
    static bool IsRotatorWithinTolerance(const FRotator& A, const FRotator& B, float ToleranceDegrees = 1.0f);

    /**
     * Generates a stable, deterministic 31-bit positive integer hash from an FName using FNV-1a.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta=(DisplayName="Hash Name To Net Int", Keywords="hash name int fnv stable deterministic"))
    static int32 HashNameToNetInt(FName InName);

    /**
     * Quantizes and packs two 16-bit float values into a single 32-bit integer.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta=(DisplayName="Pack Two Floats To Int", Keywords="pack 2 floats int 32 bit compression quantize"))
    static int32 PackTwoFloatsToInt(float ValA, float ValB, float RangeMin = -100.0f, float RangeMax = 100.0f);

    /**
     * Unpacks two 16-bit float values from a single 32-bit integer.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Quantization", meta=(DisplayName="Unpack Two Floats From Int", Keywords="unpack 2 floats int 32 bit decompression dequantize"))
    static void UnpackTwoFloatsFromInt(int32 PackedInt, float& OutValA, float& OutValB, float RangeMin = -100.0f, float RangeMax = 100.0f);

    /**
     * Sets or clears a specific bit (0-31) in a 32-bit integer mask.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Bits", meta=(DisplayName="Set Bit (32-bit)", Keywords="set bit mask 32 boolean flag"))
    static int32 SetBit32(int32 Mask, int32 BitIndex, bool bValue = true);

    /**
     * Clears a specific bit (0-31) in a 32-bit integer mask.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Bits", meta=(DisplayName="Clear Bit (32-bit)", Keywords="clear bit mask 32 zero"))
    static int32 ClearBit32(int32 Mask, int32 BitIndex);

    /**
     * Tests if a specific bit (0-31) is set in a 32-bit integer mask.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Bits", meta=(DisplayName="Test Bit (32-bit)", Keywords="test bit mask 32 check boolean"))
    static bool TestBit32(int32 Mask, int32 BitIndex);

    /**
     * Sets or clears a specific bit (0-63) in a 64-bit integer mask.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Bits", meta=(DisplayName="Set Bit (64-bit)", Keywords="set bit mask 64 boolean flag"))
    static int64 SetBit64(int64 Mask, int32 BitIndex, bool bValue = true);

    /**
     * Clears a specific bit (0-63) in a 64-bit integer mask.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Bits", meta=(DisplayName="Clear Bit (64-bit)", Keywords="clear bit mask 64 zero"))
    static int64 ClearBit64(int64 Mask, int32 BitIndex);

    /**
     * Tests if a specific bit (0-63) is set in a 64-bit integer mask.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Bits", meta=(DisplayName="Test Bit (64-bit)", Keywords="test bit mask 64 check boolean"))
    static bool TestBit64(int64 Mask, int32 BitIndex);
};

