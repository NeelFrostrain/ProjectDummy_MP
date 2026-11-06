#include "Types/MPPackedTypes.h"

FMPQuantizedVector::FMPQuantizedVector(const FVector& InVector, float InStep)
    : StepResolution(FMath::Max(0.001f, InStep))
{
    X = static_cast<int16>(FMath::Clamp(FMath::RoundToInt(InVector.X / StepResolution), -32768, 32767));
    Y = static_cast<int16>(FMath::Clamp(FMath::RoundToInt(InVector.Y / StepResolution), -32768, 32767));
    Z = static_cast<int16>(FMath::Clamp(FMath::RoundToInt(InVector.Z / StepResolution), -32768, 32767));
}

FVector FMPQuantizedVector::ToFVector() const
{
    return FVector(
        static_cast<double>(X) * StepResolution,
        static_cast<double>(Y) * StepResolution,
        static_cast<double>(Z) * StepResolution
    );
}

bool FMPQuantizedVector::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    int16 PackedX = static_cast<int16>(FMath::Clamp(X, -32768, 32767));
    int16 PackedY = static_cast<int16>(FMath::Clamp(Y, -32768, 32767));
    int16 PackedZ = static_cast<int16>(FMath::Clamp(Z, -32768, 32767));

    Ar << PackedX;
    Ar << PackedY;
    Ar << PackedZ;
    Ar << StepResolution;

    if (Ar.IsLoading())
    {
        X = PackedX;
        Y = PackedY;
        Z = PackedZ;
    }

    bOutSuccess = true;
    return true;
}

FMPQuantizedRotator::FMPQuantizedRotator(const FRotator& InRotator)
{
    PitchByte = static_cast<uint8>(FMath::RoundToInt(FRotator::ClampAxis(InRotator.Pitch) * (255.0 / 360.0)) & 0xFF);
    YawByte   = static_cast<uint8>(FMath::RoundToInt(FRotator::ClampAxis(InRotator.Yaw) * (255.0 / 360.0)) & 0xFF);
    RollByte  = static_cast<uint8>(FMath::RoundToInt(FRotator::ClampAxis(InRotator.Roll) * (255.0 / 360.0)) & 0xFF);
}

FRotator FMPQuantizedRotator::ToFRotator() const
{
    return FRotator(
        static_cast<double>(PitchByte) * (360.0 / 255.0),
        static_cast<double>(YawByte)   * (360.0 / 255.0),
        static_cast<double>(RollByte)  * (360.0 / 255.0)
    );
}

bool FMPQuantizedRotator::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    Ar << PitchByte;
    Ar << YawByte;
    Ar << RollByte;
    bOutSuccess = true;
    return true;
}

// =============================================================================
// LIBRARY IMPLEMENTATION
// =============================================================================

FMPQuantizedVector UMPPackedTypesLibrary::MakeQuantizedVector(const FVector& InVector, float StepResolution)
{
    return FMPQuantizedVector(InVector, StepResolution);
}

FVector UMPPackedTypesLibrary::BreakQuantizedVector(const FMPQuantizedVector& InQuantizedVector)
{
    return InQuantizedVector.ToFVector();
}

FMPQuantizedRotator UMPPackedTypesLibrary::MakeQuantizedRotator(const FRotator& InRotator)
{
    return FMPQuantizedRotator(InRotator);
}

FRotator UMPPackedTypesLibrary::BreakQuantizedRotator(const FMPQuantizedRotator& InQuantizedRotator)
{
    return InQuantizedRotator.ToFRotator();
}

FMPPackedBools UMPPackedTypesLibrary::MakePackedBools(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7)
{
    uint8 Mask = 0;
    if (b0) Mask |= (1 << 0);
    if (b1) Mask |= (1 << 1);
    if (b2) Mask |= (1 << 2);
    if (b3) Mask |= (1 << 3);
    if (b4) Mask |= (1 << 4);
    if (b5) Mask |= (1 << 5);
    if (b6) Mask |= (1 << 6);
    if (b7) Mask |= (1 << 7);
    return FMPPackedBools(Mask);
}

void UMPPackedTypesLibrary::BreakPackedBools(const FMPPackedBools& InPacked, bool& b0, bool& b1, bool& b2, bool& b3, bool& b4, bool& b5, bool& b6, bool& b7)
{
    b0 = InPacked.GetBool(0);
    b1 = InPacked.GetBool(1);
    b2 = InPacked.GetBool(2);
    b3 = InPacked.GetBool(3);
    b4 = InPacked.GetBool(4);
    b5 = InPacked.GetBool(5);
    b6 = InPacked.GetBool(6);
    b7 = InPacked.GetBool(7);
}

bool UMPPackedTypesLibrary::GetPackedBoolByIndex(const FMPPackedBools& InPacked, int32 Index)
{
    return InPacked.GetBool(Index);
}

void UMPPackedTypesLibrary::SetPackedBoolByIndex(FMPPackedBools& InPacked, int32 Index, bool bValue)
{
    InPacked.SetBool(Index, bValue);
}
