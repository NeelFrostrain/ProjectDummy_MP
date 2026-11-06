#include "Types/MPUrlParser.h"
#include "Kismet/KismetSystemLibrary.h"

bool UMPUrlParser::GetURLOption(const FString& InURL, const FString& Key, FString& OutValue)
{
    OutValue.Empty();
    if (Key.IsEmpty())
    {
        return false;
    }

    TArray<FString> Tokens;
    InURL.ParseIntoArray(Tokens, TEXT("?"), true);

    const FString Prefix = Key + TEXT("=");

    for (const FString& Token : Tokens)
    {
        if (Token.StartsWith(Prefix, ESearchCase::IgnoreCase))
        {
            OutValue = Token.RightChop(Prefix.Len());
            return true;
        }
    }

    return false;
}

int32 UMPUrlParser::GetURLOptionInt(const FString& InURL, const FString& Key, int32 DefaultValue)
{
    FString ValueStr;
    if (GetURLOption(InURL, Key, ValueStr))
    {
        if (ValueStr.IsNumeric())
        {
            return FCString::Atoi(*ValueStr);
        }
    }
    return DefaultValue;
}

float UMPUrlParser::GetURLOptionFloat(const FString& InURL, const FString& Key, float DefaultValue)
{
    FString ValueStr;
    if (GetURLOption(InURL, Key, ValueStr))
    {
        return FCString::Atof(*ValueStr);
    }
    return DefaultValue;
}

bool UMPUrlParser::GetURLOptionBool(const FString& InURL, const FString& Key, bool DefaultValue)
{
    FString ValueStr;
    if (GetURLOption(InURL, Key, ValueStr))
    {
        if (ValueStr.Equals(TEXT("1")) || ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("yes"), ESearchCase::IgnoreCase))
        {
            return true;
        }
        if (ValueStr.Equals(TEXT("0")) || ValueStr.Equals(TEXT("false"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("no"), ESearchCase::IgnoreCase))
        {
            return false;
        }
    }
    else if (HasURLOption(InURL, Key))
    {
        return true;
    }

    return DefaultValue;
}

bool UMPUrlParser::HasURLOption(const FString& InURL, const FString& Key)
{
    if (Key.IsEmpty())
    {
        return false;
    }

    TArray<FString> Tokens;
    InURL.ParseIntoArray(Tokens, TEXT("?"), true);

    const FString Prefix = Key + TEXT("=");

    for (const FString& Token : Tokens)
    {
        if (Token.Equals(Key, ESearchCase::IgnoreCase) || Token.StartsWith(Prefix, ESearchCase::IgnoreCase))
        {
            return true;
        }
    }

    return false;
}

FString UMPUrlParser::GetURLMapName(const FString& InURL)
{
    FString MapPart = InURL;
    int32 QuestionIdx = INDEX_NONE;
    if (MapPart.FindChar(TEXT('?'), QuestionIdx))
    {
        MapPart = MapPart.Left(QuestionIdx);
    }

    int32 ColonIdx = INDEX_NONE;
    if (MapPart.FindChar(TEXT(':'), ColonIdx))
    {
        MapPart = MapPart.Left(ColonIdx);
    }

    return MapPart;
}

FString UMPUrlParser::SetURLOption(const FString& InURL, const FString& Key, const FString& Value)
{
    if (Key.IsEmpty())
    {
        return InURL;
    }

    FString CleanURL = RemoveURLOption(InURL, Key);
    return FString::Printf(TEXT("%s?%s=%s"), *CleanURL, *Key, *Value);
}

FString UMPUrlParser::RemoveURLOption(const FString& InURL, const FString& Key)
{
    if (Key.IsEmpty())
    {
        return InURL;
    }

    TArray<FString> Tokens;
    InURL.ParseIntoArray(Tokens, TEXT("?"), true);

    const FString Prefix = Key + TEXT("=");
    TArray<FString> RetainedTokens;

    for (const FString& Token : Tokens)
    {
        if (!Token.Equals(Key, ESearchCase::IgnoreCase) && !Token.StartsWith(Prefix, ESearchCase::IgnoreCase))
        {
            RetainedTokens.Add(Token);
        }
    }

    if (RetainedTokens.Num() == 0)
    {
        return FString();
    }

    FString Result = RetainedTokens[0];
    for (int32 i = 1; i < RetainedTokens.Num(); ++i)
    {
        Result += TEXT("?") + RetainedTokens[i];
    }

    return Result;
}

FString UMPUrlParser::BuildTravelURL(const FString& MapName, const TMap<FString, FString>& Options, bool bListen, int32 Port)
{
    FString Result = MapName.TrimStartAndEnd();

    if (Port > 0)
    {
        Result += FString::Printf(TEXT(":%d"), Port);
    }

    if (bListen)
    {
        Result += TEXT("?listen");
    }

    for (const auto& Pair : Options)
    {
        if (!Pair.Key.IsEmpty())
        {
            Result += FString::Printf(TEXT("?%s=%s"), *Pair.Key, *Pair.Value);
        }
    }

    return Result;
}

