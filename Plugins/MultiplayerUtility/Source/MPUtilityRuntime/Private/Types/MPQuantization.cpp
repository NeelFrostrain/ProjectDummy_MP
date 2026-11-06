#include "Types/MPQuantization.h"

static int32 GUniqueNetRequestIdCounter = 1000;

float UMPQuantization::QuantizeFloat(float Value, float StepSize)
{
    if (StepSize <= 0.00001f)
    {
        return Value;
    }
    return FMath::RoundToFloat(Value / StepSize) * StepSize;
}

FVector UMPQuantization::QuantizeVector(const FVector& Value, float GridSize)
{
    if (GridSize <= 0.00001f)
    {
        return Value;
    }
    return FVector(
        FMath::RoundToDouble(Value.X / GridSize) * GridSize,
        FMath::RoundToDouble(Value.Y / GridSize) * GridSize,
        FMath::RoundToDouble(Value.Z / GridSize) * GridSize
    );
}

FRotator UMPQuantization::QuantizeRotator(const FRotator& Value, float DegreeStep)
{
    if (DegreeStep <= 0.00001f)
    {
        return Value;
    }
    return FRotator(
        FMath::RoundToDouble(Value.Pitch / DegreeStep) * DegreeStep,
        FMath::RoundToDouble(Value.Yaw / DegreeStep) * DegreeStep,
        FMath::RoundToDouble(Value.Roll / DegreeStep) * DegreeStep
    );
}

void UMPQuantization::CompressNormalizedVectorToBytes(const FVector& InVector, uint8& OutX, uint8& OutY, uint8& OutZ)
{
    const FVector Clamped = InVector.GetClampedToMaxSize(1.0);
    // Maps [-1.0 .. 1.0] -> [0 .. 255]
    OutX = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Clamped.X + 1.0) * 127.5), 0, 255));
    OutY = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Clamped.Y + 1.0) * 127.5), 0, 255));
    OutZ = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Clamped.Z + 1.0) * 127.5), 0, 255));
}

FVector UMPQuantization::DecompressNormalizedVectorFromBytes(uint8 InX, uint8 InY, uint8 InZ)
{
    // Maps [0 .. 255] -> [-1.0 .. 1.0]
    return FVector(
        (static_cast<double>(InX) / 127.5) - 1.0,
        (static_cast<double>(InY) / 127.5) - 1.0,
        (static_cast<double>(InZ) / 127.5) - 1.0
    );
}

bool UMPQuantization::IsValueChangedEnoughFloat(float OldVal, float NewVal, float Threshold)
{
    return FMath::Abs(NewVal - OldVal) >= Threshold;
}

bool UMPQuantization::IsValueChangedEnoughVector(const FVector& OldVal, const FVector& NewVal, float ThresholdDistance)
{
    return FVector::DistSquared(OldVal, NewVal) >= FMath::Square(ThresholdDistance);
}

int32 UMPQuantization::HashPlayerIdToInt(const FString& InPlayerId)
{
    return static_cast<int32>(GetTypeHash(InPlayerId) & 0x7FFFFFFF);
}

int32 UMPQuantization::GenerateUniqueNetRequestId()
{
    return ++GUniqueNetRequestIdCounter;
}

float UMPQuantization::WrapAngleSafe(float AngleDegrees)
{
    return FRotator::NormalizeAxis(AngleDegrees);
}

bool UMPQuantization::IsVectorWithinTolerance(const FVector& A, const FVector& B, float Tolerance)
{
    const float TolSq = FMath::Square(FMath::Max(0.0f, Tolerance));
    return FVector::DistSquared(A, B) <= TolSq;
}

bool UMPQuantization::IsRotatorWithinTolerance(const FRotator& A, const FRotator& B, float ToleranceDegrees)
{
    const float PitchDiff = FMath::Abs(FRotator::NormalizeAxis(A.Pitch - B.Pitch));
    const float YawDiff = FMath::Abs(FRotator::NormalizeAxis(A.Yaw - B.Yaw));
    const float RollDiff = FMath::Abs(FRotator::NormalizeAxis(A.Roll - B.Roll));
    return (PitchDiff <= ToleranceDegrees) && (YawDiff <= ToleranceDegrees) && (RollDiff <= ToleranceDegrees);
}

int32 UMPQuantization::HashNameToNetInt(FName InName)
{
    if (InName.IsNone())
    {
        return 0;
    }
    const FString Str = InName.ToString();
    uint32 Hash = 2166136261u;
    for (int32 i = 0; i < Str.Len(); ++i)
    {
        Hash ^= static_cast<uint32>(Str[i]);
        Hash *= 16777619u;
    }
    return static_cast<int32>(Hash & 0x7FFFFFFF);
}

int32 UMPQuantization::PackTwoFloatsToInt(float ValA, float ValB, float RangeMin, float RangeMax)
{
    const float Range = (RangeMax > RangeMin) ? (RangeMax - RangeMin) : 1.0f;
    const float ClampedA = FMath::Clamp(ValA, RangeMin, RangeMax);
    const float ClampedB = FMath::Clamp(ValB, RangeMin, RangeMax);

    const uint16 QuantA = static_cast<uint16>(FMath::RoundToInt(((ClampedA - RangeMin) / Range) * 65535.0f));
    const uint16 QuantB = static_cast<uint16>(FMath::RoundToInt(((ClampedB - RangeMin) / Range) * 65535.0f));

    return static_cast<int32>((static_cast<uint32>(QuantA) << 16) | static_cast<uint32>(QuantB));
}

void UMPQuantization::UnpackTwoFloatsFromInt(int32 PackedInt, float& OutValA, float& OutValB, float RangeMin, float RangeMax)
{
    const float Range = (RangeMax > RangeMin) ? (RangeMax - RangeMin) : 1.0f;
    const uint32 PackedU = static_cast<uint32>(PackedInt);
    const uint16 QuantA = static_cast<uint16>((PackedU >> 16) & 0xFFFF);
    const uint16 QuantB = static_cast<uint16>(PackedU & 0xFFFF);

    OutValA = RangeMin + (static_cast<float>(QuantA) / 65535.0f) * Range;
    OutValB = RangeMin + (static_cast<float>(QuantB) / 65535.0f) * Range;
}

int32 UMPQuantization::SetBit32(int32 Mask, int32 BitIndex, bool bValue)
{
    if (BitIndex < 0 || BitIndex >= 32) return Mask;
    const uint32 Bit = 1u << BitIndex;
    return bValue ? static_cast<int32>(static_cast<uint32>(Mask) | Bit)
                  : static_cast<int32>(static_cast<uint32>(Mask) & ~Bit);
}

int32 UMPQuantization::ClearBit32(int32 Mask, int32 BitIndex)
{
    if (BitIndex < 0 || BitIndex >= 32) return Mask;
    return static_cast<int32>(static_cast<uint32>(Mask) & ~(1u << BitIndex));
}

bool UMPQuantization::TestBit32(int32 Mask, int32 BitIndex)
{
    if (BitIndex < 0 || BitIndex >= 32) return false;
    return (static_cast<uint32>(Mask) & (1u << BitIndex)) != 0;
}

int64 UMPQuantization::SetBit64(int64 Mask, int32 BitIndex, bool bValue)
{
    if (BitIndex < 0 || BitIndex >= 64) return Mask;
    const uint64 Bit = 1ULL << BitIndex;
    return bValue ? static_cast<int64>(static_cast<uint64>(Mask) | Bit)
                  : static_cast<int64>(static_cast<uint64>(Mask) & ~Bit);
}

int64 UMPQuantization::ClearBit64(int64 Mask, int32 BitIndex)
{
    if (BitIndex < 0 || BitIndex >= 64) return Mask;
    return static_cast<int64>(static_cast<uint64>(Mask) & ~(1ULL << BitIndex));
}

bool UMPQuantization::TestBit64(int64 Mask, int32 BitIndex)
{
    if (BitIndex < 0 || BitIndex >= 64) return false;
    return (static_cast<uint64>(Mask) & (1ULL << BitIndex)) != 0;
}

