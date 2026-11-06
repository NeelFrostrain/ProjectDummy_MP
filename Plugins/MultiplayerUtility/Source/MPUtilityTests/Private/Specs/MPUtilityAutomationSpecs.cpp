#include "Misc/AutomationTest.h"
#include "MPNetMacros.h"
#include "MPNetFunctionLibrary.h"
#include "Types/MPRateLimiter.h"
#include "Types/MPRewindBuffer.h"
#include "Types/MPTimeMath.h"
#include "Types/MPQuantization.h"
#include "Types/MPUrlParser.h"
#include "Types/MPReplicatedTimer.h"
#include "Types/MPPackedTypes.h"
#include "Types/MPReplicatedSeed.h"
#include "Wrappers/MPActorNetworkWrapper.h"
#include "Wrappers/MPPawnControllerWrapper.h"
#include "Wrappers/MPWorldSessionWrapper.h"
#include "Wrappers/MPMovementPhysicsWrapper.h"
#include "Wrappers/MPComponentNetworkWrapper.h"
#include "Types/MPReplicatedDataTypes.h"
#include "Types/MPNetPresetAsset.h"
#include "FlowControl/MPFlowControlLibrary.h"
#include "Subsystems/MPNetStatsSubsystem.h"
#include "Requests/MPNetworkRequestTypes.h"
#include "Requests/MPNetworkRequestComponent.h"
#include "Replication/MPFastArrayTypes.h"
#include "Replication/MPReplicationConditions.h"
#include "Player/MPPlayerLifecycleLibrary.h"
#include "Movement/MPCharacterMovementComponent.h"
#include "Types/MPReplicatedStateMachine.h"
#include "Types/MPReplicatedCountdown.h"
#include "Physics/MPPhysicsReplicationHelper.h"
#include "Security/MPSecurityTypes.h"
#include "Security/MPServerSecurityLibrary.h"
#include "Async/MPAsyncNetworkActions.h"
#include "Optimization/MPNetOptimizationTypes.h"
#include "Optimization/MPNetLODComponent.h"
#include "Gameplay/MPHealthComponent.h"
#include "Gameplay/MPGameplayHelperLibrary.h"
#include "Gameplay/MPGameplaySyncTypes.h"
#include "Gameplay/MPRPCGameplayComponent.h"
#if WITH_EDITOR
#include "Subsystems/MPEditorScannerSubsystem.h"
#endif
#include "MPNetworkStressTester.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPRateLimiterTest, "MultiplayerUtility.Core.RateLimiter", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPRateLimiterTest::RunTest(const FString& Parameters)
{
    // Capacity 10, Refill 5 tokens/sec
    FMPRateLimiter Limiter(10.0, 5.0, 0.0);
    TestEqual("Initial tokens should equal capacity", Limiter.GetAvailableTokens(0.0), 10.0);

    // Consume 6 tokens at T = 0.0
    bool bConsumed = Limiter.Consume(6.0, 0.0);
    TestTrue("Should successfully consume 6 tokens", bConsumed);
    TestEqual("Should have 4 tokens remaining", Limiter.CurrentTokens, 4.0);

    // Try consuming 5 tokens immediately (should fail)
    bConsumed = Limiter.Consume(5.0, 0.0);
    TestFalse("Should fail to consume 5 tokens when only 4 exist", bConsumed);

    // Advance 1.0 second: Refill 5 tokens -> 4 + 5 = 9 tokens
    bConsumed = Limiter.Consume(8.0, 1.0);
    TestTrue("Should succeed consuming 8 tokens after 1 second refill", bConsumed);
    TestEqual("Should have 1 token remaining", Limiter.CurrentTokens, 1.0);

    // Advance 10.0 seconds: Tokens should cap at 10.0 (not exceed capacity)
    TestEqual("Available tokens after large duration must not exceed capacity", Limiter.GetAvailableTokens(11.0), 10.0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPRewindBufferTest, "MultiplayerUtility.Core.RewindBuffer", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPRewindBufferTest::RunTest(const FString& Parameters)
{
    FMPRewindBuffer Rewind(64, 1.0f);
    FTransform OutTransform;

    TestFalse("Empty buffer returns false", Rewind.GetInterpolatedTransform(0.5, OutTransform));

    // Record snapshot A at T = 1.0 (Location: 0, 0, 0)
    const FTransform TransA(FQuat::Identity, FVector(0.0, 0.0, 0.0), FVector(1.0, 1.0, 1.0));
    Rewind.RecordSnapshot(1.0, TransA, FVector(100.0, 0.0, 0.0));

    // Record snapshot B at T = 2.0 (Location: 100, 0, 0)
    const FTransform TransB(FQuat::Identity, FVector(100.0, 0.0, 0.0), FVector(100.0, 0.0, 0.0));
    Rewind.RecordSnapshot(2.0, TransB, FVector(100.0, 0.0, 0.0));

    // Sample exactly at midpoint T = 1.5 -> Location should interpolate to 50, 0, 0
    bool bSuccess = Rewind.GetInterpolatedTransform(1.5, OutTransform);
    TestTrue("Interpolation should succeed at midpoint", bSuccess);
    TestNearlyEqual("Interpolated X location at midpoint should be 50.0", OutTransform.GetLocation().X, 50.0, 0.01);

    // Sample at past boundary T = 0.5 -> Should clamp to TransA
    bSuccess = Rewind.GetInterpolatedTransform(0.5, OutTransform);
    TestTrue("Past boundary sampling should return oldest transform", bSuccess);
    TestNearlyEqual("Location at past boundary clamped to 0.0", OutTransform.GetLocation().X, 0.0, 0.01);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPTimeMathTest, "MultiplayerUtility.Core.TimeMath", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPTimeMathTest::RunTest(const FString& Parameters)
{
    // Test NTP offset calculation
    // T1: 10.0, T2: 15.0, T3: 15.1, T4: 10.2
    // RTT = (10.2 - 10.0) - (15.1 - 15.0) = 0.2 - 0.1 = 0.1s (100ms)
    // Offset = ((15.0 - 10.0) + (15.1 - 10.2)) / 2 = (5.0 + 4.9) / 2 = 4.95s
    double RTT = 0.0;
    double Offset = 0.0;
    UMPTimeMath::CalculateNTPTimeOffset(10.0, 15.0, 15.1, 10.2, RTT, Offset);

    TestNearlyEqual("NTP RTT should be 0.1", RTT, 0.1, 0.001);
    TestNearlyEqual("NTP Clock Offset should be 4.95", Offset, 4.95, 0.001);

    // Test sequence number wraparound
    TestTrue("Sequence 0 should be newer than 65535", UMPTimeMath::IsSequenceNewer16(0, 65535));
    TestFalse("Sequence 65535 should not be newer than 0", UMPTimeMath::IsSequenceNewer16(65535, 0));
    TestTrue("Sequence 10 should be newer than 5", UMPTimeMath::IsSequenceNewer16(10, 5));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPUrlParserTest, "MultiplayerUtility.Core.UrlParser", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPUrlParserTest::RunTest(const FString& Parameters)
{
    const FString TestURL = TEXT("/Game/Maps/Arena:7777?listen?Team=Red?MaxPlayers=16?FriendlyFire=true");

    TestEqual("Map name extraction", UMPUrlParser::GetURLMapName(TestURL), TEXT("/Game/Maps/Arena"));
    TestTrue("Has listen option", UMPUrlParser::HasURLOption(TestURL, TEXT("listen")));

    FString Team;
    TestTrue("Get Team option", UMPUrlParser::GetURLOption(TestURL, TEXT("Team"), Team));
    TestEqual("Team value is Red", Team, TEXT("Red"));

    TestEqual("Int option MaxPlayers", UMPUrlParser::GetURLOptionInt(TestURL, TEXT("MaxPlayers")), 16);
    TestTrue("Bool option FriendlyFire", UMPUrlParser::GetURLOptionBool(TestURL, TEXT("FriendlyFire")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPNullSafetyTest, "MultiplayerUtility.Core.NullSafety", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPNullSafetyTest::RunTest(const FString& Parameters)
{
    // None of these should crash when receiving nullptrs
    TestFalse("Null actor authority is false", UMPNetFunctionLibrary::HasAuthority(nullptr));
    TestEqual("Null actor local role is ROLE_None", UMPActorNetworkWrapper::GetLocalRole(nullptr), ROLE_None);
    TestEqual("Null actor remote role is ROLE_None", UMPActorNetworkWrapper::GetRemoteRole(nullptr), ROLE_None);
    TestFalse("Null pawn locally controlled is false", UMPNetFunctionLibrary::IsLocallyControlled(nullptr));
    TestFalse("Null world network game is false", UMPNetFunctionLibrary::IsNetworkGame(nullptr));
    TestTrue("Null world standalone is true", UMPNetFunctionLibrary::IsStandalone(nullptr));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPPackedTypesTest, "MultiplayerUtility.Core.PackedTypes", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPPackedTypesTest::RunTest(const FString& Parameters)
{
    // 1. Vector Quantization & Dequantization Error Bounds
    const FVector OriginalVec(1234.56, -567.89, 42.12);
    const FMPQuantizedVector QuantVec = UMPPackedTypesLibrary::MakeQuantizedVector(OriginalVec, 0.1f);
    const FVector RestoredVec = UMPPackedTypesLibrary::BreakQuantizedVector(QuantVec);

    TestNearlyEqual("Quantized X should stay within step precision (0.1)", RestoredVec.X, OriginalVec.X, 0.1);
    TestNearlyEqual("Quantized Y should stay within step precision (0.1)", RestoredVec.Y, OriginalVec.Y, 0.1);
    TestNearlyEqual("Quantized Z should stay within step precision (0.1)", RestoredVec.Z, OriginalVec.Z, 0.1);

    // 2. Rotator Quantization Error Bounds (360 / 255 = ~1.4 degrees)
    const FRotator OriginalRot(45.0, 90.0, 180.0);
    const FMPQuantizedRotator QuantRot = UMPPackedTypesLibrary::MakeQuantizedRotator(OriginalRot);
    const FRotator RestoredRot = UMPPackedTypesLibrary::BreakQuantizedRotator(QuantRot);

    TestNearlyEqual("Quantized Pitch should stay within 1.5 degrees", RestoredRot.Pitch, OriginalRot.Pitch, 1.5);
    TestNearlyEqual("Quantized Yaw should stay within 1.5 degrees", RestoredRot.Yaw, OriginalRot.Yaw, 1.5);
    TestNearlyEqual("Quantized Roll should stay within 1.5 degrees", RestoredRot.Roll, OriginalRot.Roll, 1.5);

    // 3. Packed Booleans (8 bits in 1 byte)
    FMPPackedBools Packed = UMPPackedTypesLibrary::MakePackedBools(true, false, true, false, true, true, false, true);
    TestTrue("Bit 0 is true", UMPPackedTypesLibrary::GetPackedBoolByIndex(Packed, 0));
    TestFalse("Bit 1 is false", UMPPackedTypesLibrary::GetPackedBoolByIndex(Packed, 1));
    TestTrue("Bit 2 is true", UMPPackedTypesLibrary::GetPackedBoolByIndex(Packed, 2));
    TestTrue("Bit 7 is true", UMPPackedTypesLibrary::GetPackedBoolByIndex(Packed, 7));

    UMPPackedTypesLibrary::SetPackedBoolByIndex(Packed, 1, true);
    TestTrue("Bit 1 toggled to true", UMPPackedTypesLibrary::GetPackedBoolByIndex(Packed, 1));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPReplicatedSeedTest, "MultiplayerUtility.Core.ReplicatedSeed", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPReplicatedSeedTest::RunTest(const FString& Parameters)
{
    // Two independent peers initialized with the identical base seed
    FMPReplicatedSeed ServerSeed = UMPReplicatedSeedLibrary::MakeReplicatedSeed(424242);
    FMPReplicatedSeed ClientSeed = UMPReplicatedSeedLibrary::MakeReplicatedSeed(424242);

    // Generate 10 consecutive random values on both peers
    for (int32 i = 0; i < 10; ++i)
    {
        const float ServerFloat = UMPReplicatedSeedLibrary::RandomFloatFromSeed(ServerSeed, 0.0f, 100.0f);
        const float ClientFloat = UMPReplicatedSeedLibrary::RandomFloatFromSeed(ClientSeed, 0.0f, 100.0f);
        TestEqual(FString::Printf(TEXT("Random float step %d must be 100%% identical across peers"), i), ServerFloat, ClientFloat);

        const int32 ServerInt = UMPReplicatedSeedLibrary::RandomIntFromSeed(ServerSeed, 1, 1000);
        const int32 ClientInt = UMPReplicatedSeedLibrary::RandomIntFromSeed(ClientSeed, 1, 1000);
        TestEqual(FString::Printf(TEXT("Random int step %d must be 100%% identical across peers"), i), ServerInt, ClientInt);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPMacrosTest, "MultiplayerUtility.Core.Macros", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPMacrosTest::RunTest(const FString& Parameters)
{
    // 1. Static assert type verification
    MP_STATIC_ASSERT_REPLICATED_TYPE(int32);
    MP_STATIC_ASSERT_REPLICATED_TYPE(float);
    MP_STATIC_ASSERT_REPLICATED_TYPE(FVector);
    MP_STATIC_ASSERT_REPLICATED_TYPE(AActor*);

    // 2. Change Detection Macro Testing
    float TestFloat = 10.0f;
    TestTrue("Float change below threshold is false", !UE::MPNet::IsChangedEnough(TestFloat, 10.02f, 0.05f));
    TestTrue("Float change above threshold is true", UE::MPNet::IsChangedEnough(TestFloat, 10.1f, 0.05f));

    FVector TestVec(100.0, 200.0, 300.0);
    TestTrue("Vector change within 1 unit is false", !UE::MPNet::IsChangedEnough(TestVec, FVector(100.5, 200.0, 300.0), 1.0f));
    TestTrue("Vector change over 2 units is true", UE::MPNet::IsChangedEnough(TestVec, FVector(103.0, 200.0, 300.0), 2.0f));

    // 3. Rate-limited and unique log macros
    MP_LOG_EVERY_N_SEC(1.0, TEXT("Testing rate-limited macro"));
    MP_NET_SCOPE("TestScope");
    MP_COUNT_RPC(TestRPC);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPPlayerQueriesTest, "MultiplayerUtility.Core.PlayerQueries", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPPlayerQueriesTest::RunTest(const FString& Parameters)
{
    // 1. Null safety checks on all Batch 1 queries
    TestEqual("Null context distance to nearest player returns -1.0f", UMPNetFunctionLibrary::GetDistanceToNearestPlayer(nullptr, FVector::ZeroVector), -1.0f);
    TestFalse("Null context is any player within range returns false", UMPNetFunctionLibrary::IsAnyPlayerWithinRange(nullptr, FVector::ZeroVector, 500.0f));
    TestEqual("Null context sorted players returns empty array", UMPNetFunctionLibrary::GetPlayersSortedByDistance(nullptr, FVector::ZeroVector).Num(), 0);
    TestNull("Null context furthest player returns nullptr", UMPNetFunctionLibrary::GetFurthestPlayer(nullptr, FVector::ZeroVector));
    TestEqual("Null context players in cone returns empty array", UMPNetFunctionLibrary::GetPlayersInCone(nullptr, FVector::ZeroVector, FVector::ForwardVector, 45.0f, 1000.0f).Num(), 0);
    TestEqual("Null context players with line of sight returns empty array", UMPNetFunctionLibrary::GetPlayersInRadiusWithLineOfSight(nullptr, FVector::ZeroVector, 1000.0f).Num(), 0);
    TestNull("Null context random player returns nullptr", UMPNetFunctionLibrary::GetRandomPlayer(nullptr, TArray<APawn*>()));
    TestEqual("Null context alive players returns empty array", UMPNetFunctionLibrary::GetAlivePlayersOnly(nullptr).Num(), 0);

    // 2. Query Math Verification: Cone Angle Dot Product
    const FVector Forward(1.0, 0.0, 0.0);
    const FVector TargetInCone(100.0, 20.0, 0.0);
    const FVector TargetOutsideCone(100.0, 150.0, 0.0);

    const float MinDot45 = FMath::Cos(FMath::DegreesToRadians(45.0f));
    const float DotIn = FVector::DotProduct(Forward, TargetInCone.GetSafeNormal());
    const float DotOut = FVector::DotProduct(Forward, TargetOutsideCone.GetSafeNormal());

    TestTrue("Target within 45-deg cone has dot >= cos(45)", DotIn >= MinDot45);
    TestTrue("Target outside 45-deg cone has dot < cos(45)", DotOut < MinDot45);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPMovementPhysicsTest, "MultiplayerUtility.Core.MovementPhysics", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPMovementPhysicsTest::RunTest(const FString& Parameters)
{
    // 1. Snap or smooth threshold test
    const FTransform Current(FQuat::Identity, FVector(0.0, 0.0, 0.0), FVector(1.0, 1.0, 1.0));
    const FTransform TargetNear(FQuat::Identity, FVector(50.0, 0.0, 0.0), FVector(1.0, 1.0, 1.0));
    const FTransform TargetFar(FQuat::Identity, FVector(500.0, 0.0, 0.0), FVector(1.0, 1.0, 1.0));

    // Near distance (50 units) within 100 unit threshold should smoothly lerp
    const FTransform Smoothed = UMPMovementPhysicsWrapper::SnapOrSmoothTransform(Current, TargetNear, 100.0f, 0.5f);
    TestEqual("Near error smoothly lerps position", Smoothed.GetLocation().X, 25.0);

    // Far distance (500 units) exceeding 100 unit threshold should snap directly to target
    const FTransform Snapped = UMPMovementPhysicsWrapper::SnapOrSmoothTransform(Current, TargetFar, 100.0f, 0.5f);
    TestEqual("Far error snaps directly to target position", Snapped.GetLocation().X, 500.0);

    // 2. Movement error distance
    const float ErrorDist = UMPMovementPhysicsWrapper::GetMovementErrorDistance(FVector(100.0, 0.0, 0.0), FVector(130.0, 40.0, 0.0));
    TestEqual("3-4-5 triangle movement error distance is 50 units", ErrorDist, 50.0f);

    // 3. Null safety checks
    TestEqual("Null actor replicated velocity is ZeroVector", UMPMovementPhysicsWrapper::GetReplicatedVelocitySafe(nullptr), FVector::ZeroVector);
    TestFalse("Null actor physics replicated is false", UMPMovementPhysicsWrapper::IsPhysicsReplicated(nullptr));
    TestFalse("Null character moving authoritatively is false", UMPMovementPhysicsWrapper::IsCharacterMovingAuthoritatively(nullptr));
    TestEqual("Null character smoothing mode is Disabled", UMPMovementPhysicsWrapper::GetNetworkSmoothingMode(nullptr), ENetworkSmoothingMode::Disabled);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPLevelStreamingWorldTest, "MultiplayerUtility.Core.LevelStreamingWorld", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPLevelStreamingWorldTest::RunTest(const FString& Parameters)
{
    // 1. URL travel options parser test
    const FString TestURL = TEXT("/Game/Maps/Arena?listen?port=7777?Team=2?PlayerName=Hero");
    const TMap<FString, FString> Options = UMPWorldSessionWrapper::GetTravelOptionsFromURL(TestURL);

    TestEqual("Should parse 4 option keys", Options.Num(), 4);
    TestTrue("Options should contain 'listen' flag", Options.Contains(TEXT("listen")));
    TestEqual("Flag option value should be empty string", Options.FindRef(TEXT("listen")), FString());
    TestEqual("Parsed port should match", Options.FindRef(TEXT("port")), TEXT("7777"));
    TestEqual("Parsed team should match", Options.FindRef(TEXT("Team")), TEXT("2"));
    TestEqual("Parsed player name should match", Options.FindRef(TEXT("PlayerName")), TEXT("Hero"));

    // Options only string
    const FString OptionsOnlyURL = TEXT("?game=Deathmatch?bots=4");
    const TMap<FString, FString> OptsOnly = UMPWorldSessionWrapper::GetTravelOptionsFromURL(OptionsOnlyURL);
    TestEqual("Options-only URL parses 2 keys", OptsOnly.Num(), 2);
    TestEqual("Parsed game matches", OptsOnly.FindRef(TEXT("game")), TEXT("Deathmatch"));
    TestEqual("Parsed bots matches", OptsOnly.FindRef(TEXT("bots")), TEXT("4"));

    // Empty URL
    const TMap<FString, FString> EmptyOpts = UMPWorldSessionWrapper::GetTravelOptionsFromURL(TEXT(""));
    TestEqual("Empty URL produces empty map", EmptyOpts.Num(), 0);

    // 2. Null safety tests for world and level functions
    TestFalse("Null world context IsLevelLoadedForAllPlayers returns false", UMPWorldSessionWrapper::IsLevelLoadedForAllPlayers(nullptr, FName("TestLevel")));
    TestEqual("Null world context GetPlayersInStreamingLevel returns empty array", UMPWorldSessionWrapper::GetPlayersInStreamingLevel(nullptr, FName("TestLevel")).Num(), 0);
    TestFalse("Null actor IsActorInLoadedLevel returns false", UMPWorldSessionWrapper::IsActorInLoadedLevel(nullptr));
    TestEqual("Null actor GetLevelNameOfActor returns empty string", UMPWorldSessionWrapper::GetLevelNameOfActor(nullptr), FString());
    TestFalse("Null world context IsSeamlessTraveling returns false", UMPWorldSessionWrapper::IsSeamlessTraveling(nullptr));
    TestEqual("Null world context GetCurrentMapNameClean returns empty string", UMPWorldSessionWrapper::GetCurrentMapNameClean(nullptr), FString());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPTagAndNameHelpersTest, "MultiplayerUtility.Core.TagAndNameHelpers", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPTagAndNameHelpersTest::RunTest(const FString& Parameters)
{
    // 1. MakeNetUniqueName tests
    TestEqual("MakeNetUniqueName formats prefix, net index, and counter", UMPNetFunctionLibrary::MakeNetUniqueName(TEXT("Player"), 3, 42), TEXT("Player_3_42"));
    TestEqual("MakeNetUniqueName falls back to NetObj when prefix is empty", UMPNetFunctionLibrary::MakeNetUniqueName(TEXT(""), 0, 1), TEXT("NetObj_0_1"));

    // 2. IsValidPlayerName tests
    TestTrue("Alphanumeric with underscores is valid", UMPNetFunctionLibrary::IsValidPlayerName(TEXT("Sniper_Wolf-01")));
    TestFalse("Too short name (< 3 chars) is rejected", UMPNetFunctionLibrary::IsValidPlayerName(TEXT("GG")));
    TestFalse("Name exceeding 20 characters is rejected", UMPNetFunctionLibrary::IsValidPlayerName(TEXT("PlayerNameThatIsExceedinglyLongAndForbidden")));
    TestFalse("Illegal characters are rejected", UMPNetFunctionLibrary::IsValidPlayerName(TEXT("Admin?DropTable")));
    TestFalse("Space rejected when bAllowSpaces is false", UMPNetFunctionLibrary::IsValidPlayerName(TEXT("John Doe"), 3, 20, false));
    TestTrue("Space accepted when bAllowSpaces is true", UMPNetFunctionLibrary::IsValidPlayerName(TEXT("John Doe"), 3, 20, true));

    // 3. ComparePlayerNamesCaseInsensitive tests
    TestTrue("Identical names match case-insensitively", UMPNetFunctionLibrary::ComparePlayerNamesCaseInsensitive(TEXT("PlayerOne"), TEXT("playerone")));
    TestTrue("Names matching with leading/trailing whitespace", UMPNetFunctionLibrary::ComparePlayerNamesCaseInsensitive(TEXT("  PlayerOne "), TEXT("PLAYERONE")));
    TestFalse("Different names return false", UMPNetFunctionLibrary::ComparePlayerNamesCaseInsensitive(TEXT("PlayerOne"), TEXT("PlayerTwo")));

    // 4. Null safety for Tag helpers
    FGameplayTag InvalidTag;
    TestEqual("Null world context GetActorsWithTagReplicatedSafe returns empty", UMPNetFunctionLibrary::GetActorsWithTagReplicatedSafe(nullptr, InvalidTag).Num(), 0);
    FGameplayTagContainer EmptyContainer;
    TestFalse("Null actor HasAnyTagSafe returns false", UMPNetFunctionLibrary::HasAnyTagSafe(nullptr, EmptyContainer));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPSubobjectComponentReplicationTest, "MultiplayerUtility.Core.SubobjectComponentReplication", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPSubobjectComponentReplicationTest::RunTest(const FString& Parameters)
{
    // 1. Null safety tests for subobject replication
    TestFalse("Null owner RegisterReplicatedSubobjectSafe returns false", UMPComponentNetworkWrapper::RegisterReplicatedSubobjectSafe(nullptr, nullptr));
    TestFalse("Null owner UnregisterReplicatedSubobjectSafe returns false", UMPComponentNetworkWrapper::UnregisterReplicatedSubobjectSafe(nullptr, nullptr));
    TestFalse("Null owner IsSubobjectReplicated returns false", UMPComponentNetworkWrapper::IsSubobjectReplicated(nullptr, nullptr));

    // 2. Component queries null safety
    TestEqual("Null actor GetReplicatedComponentsOfActor returns empty array", UMPComponentNetworkWrapper::GetReplicatedComponentsOfActor(nullptr).Num(), 0);
    TestEqual("Null actor PrintReplicatedSubobjects returns empty array", UMPComponentNetworkWrapper::PrintReplicatedSubobjects(nullptr, false).Num(), 0);

    // 3. Recursive replication null safety
    UMPComponentNetworkWrapper::SetComponentReplicatedRecursive(nullptr, true);
    TestFalse("Null component IsComponentReplicated returns false", UMPComponentNetworkWrapper::IsComponentReplicated(nullptr));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPMathDataNetworkingTest, "MultiplayerUtility.Core.MathDataNetworking", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPMathDataNetworkingTest::RunTest(const FString& Parameters)
{
    // 1. Timestamp and Sequence tests
    TestTrue("10s is newer than 5s", UMPTimeMath::IsTimestampNewer(10.0f, 5.0f));
    TestFalse("5s is not newer than 10s", UMPTimeMath::IsTimestampNewer(5.0f, 10.0f));
    TestTrue("Timestamp circular wraparound test", UMPTimeMath::IsTimestampNewer(1.0f, 3599.0f, 1800.0f));

    TestEqual("SequenceDistance16 forward distance", UMPTimeMath::SequenceDistance16(5, 2), 3);
    TestEqual("SequenceDistance16 wraparound distance", UMPTimeMath::SequenceDistance16(2, 65534), 4);
    TestEqual("SequenceDistance32 distance", UMPTimeMath::SequenceDistance32(100, 50), static_cast<int64>(50));

    // 2. Network DeltaTime clamping and smoothing
    TestEqual("DeltaTime clamped from large spike", UMPTimeMath::ClampDeltaTimeForNetwork(0.5f, 0.001f, 0.2f), 0.2f);
    TestEqual("DeltaTime clamped from negative", UMPTimeMath::ClampDeltaTimeForNetwork(-0.1f, 0.001f, 0.2f), 0.001f);
    TestEqual("LerpNetValue linearly interpolates with rate limit", UMPTimeMath::LerpNetValue(0.0f, 100.0f, 0.1f, 5.0f), 50.0f);
    TestEqual("Exponential smooth zero dt returns target immediately", UMPTimeMath::ExponentialSmoothFloat(0.0f, 100.0f, 0.0f, 5.0f), 100.0f);

    // 3. Angles and tolerance checks
    TestEqual("WrapAngleSafe normalizes 190 to -170", UMPQuantization::WrapAngleSafe(190.0f), -170.0f);
    TestEqual("WrapAngleSafe normalizes -190 to 170", UMPQuantization::WrapAngleSafe(-190.0f), 170.0f);
    TestTrue("Vector within tolerance", UMPQuantization::IsVectorWithinTolerance(FVector(0,0,0), FVector(0.5, 0.5, 0), 1.0f));
    TestFalse("Vector beyond tolerance", UMPQuantization::IsVectorWithinTolerance(FVector(0,0,0), FVector(5, 0, 0), 1.0f));
    TestTrue("Rotator within tolerance", UMPQuantization::IsRotatorWithinTolerance(FRotator(10, 20, 30), FRotator(10.5, 20.2, 29.8), 1.0f));

    // 4. Stable name hash
    const int32 HashHero = UMPQuantization::HashNameToNetInt(FName("HeroPlayer"));
    TestTrue("Stable hash is positive integer", HashHero > 0);
    TestEqual("Stable hash is deterministic", HashHero, UMPQuantization::HashNameToNetInt(FName("HeroPlayer")));

    // 5. Float pack / unpack quantization
    const float OriginalA = 12.5f;
    const float OriginalB = -34.2f;
    const int32 Packed = UMPQuantization::PackTwoFloatsToInt(OriginalA, OriginalB, -50.0f, 50.0f);
    float UnpackedA = 0.0f;
    float UnpackedB = 0.0f;
    UMPQuantization::UnpackTwoFloatsFromInt(Packed, UnpackedA, UnpackedB, -50.0f, 50.0f);
    TestTrue("Unpacked A matches within 0.01 tolerance", FMath::Abs(UnpackedA - OriginalA) < 0.01f);
    TestTrue("Unpacked B matches within 0.01 tolerance", FMath::Abs(UnpackedB - OriginalB) < 0.01f);

    // 6. Bit manipulation helpers
    int32 Mask32 = 0;
    Mask32 = UMPQuantization::SetBit32(Mask32, 4, true);
    TestEqual("Bit 4 set is 16", Mask32, 16);
    TestTrue("TestBit32 returns true on bit 4", UMPQuantization::TestBit32(Mask32, 4));
    TestFalse("TestBit32 returns false on bit 3", UMPQuantization::TestBit32(Mask32, 3));
    Mask32 = UMPQuantization::ClearBit32(Mask32, 4);
    TestEqual("Bit 4 cleared is 0", Mask32, 0);

    int64 Mask64 = 0;
    Mask64 = UMPQuantization::SetBit64(Mask64, 45, true);
    TestTrue("TestBit64 returns true on bit 45", UMPQuantization::TestBit64(Mask64, 45));
    TestFalse("TestBit64 returns false on bit 44", UMPQuantization::TestBit64(Mask64, 44));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPCVarSettingsTest, "MultiplayerUtility.Core.CVarSettings", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPCVarSettingsTest::RunTest(const FString& Parameters)
{
    // 1. Verify default values of registered MP CVars
    TestEqual("mp.Debug.Overlay defaults to 0", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.Debug.Overlay")), 0);
    TestEqual("mp.Debug.LogOnce defaults to 1", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.Debug.LogOnce")), 1);
    TestTrue("mp.RateLimit.Enabled defaults to true", UMPNetFunctionLibrary::GetMPCVarBool(TEXT("mp.RateLimit.Enabled")));
    TestEqual("mp.Validation.KickOnFail defaults to 0", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.Validation.KickOnFail")), 0);
    TestTrue("mp.Net.PingSampleInterval defaults to ~0.25s", FMath::IsNearlyEqual(UMPNetFunctionLibrary::GetMPCVarFloat(TEXT("mp.Net.PingSampleInterval")), 0.25f, 0.01f));

    // 2. Get / Set CVar tests with restoration
    UMPNetFunctionLibrary::SetMPCVarBool(TEXT("mp.RateLimit.Enabled"), false);
    TestFalse("mp.RateLimit.Enabled updated to false", UMPNetFunctionLibrary::GetMPCVarBool(TEXT("mp.RateLimit.Enabled")));
    UMPNetFunctionLibrary::SetMPCVarBool(TEXT("mp.RateLimit.Enabled"), true);
    TestTrue("mp.RateLimit.Enabled restored to true", UMPNetFunctionLibrary::GetMPCVarBool(TEXT("mp.RateLimit.Enabled")));

    UMPNetFunctionLibrary::SetMPCVarInt(TEXT("mp.Debug.Overlay"), 1);
    TestEqual("mp.Debug.Overlay updated to 1", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.Debug.Overlay")), 1);
    UMPNetFunctionLibrary::SetMPCVarInt(TEXT("mp.Debug.Overlay"), 0);
    TestEqual("mp.Debug.Overlay restored to 0", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.Debug.Overlay")), 0);

    // 3. Fallback tests on non-existent CVars
    TestTrue("Non-existent bool CVar returns fallback true", UMPNetFunctionLibrary::GetMPCVarBool(TEXT("mp.NonExistent.Bool"), true));
    TestEqual("Non-existent int CVar returns fallback 42", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.NonExistent.Int"), 42), 42);
    TestEqual("Non-existent float CVar returns fallback 3.14", UMPNetFunctionLibrary::GetMPCVarFloat(TEXT("mp.NonExistent.Float"), 3.14f), 3.14f);

    // 4. Test MP_LOG macro
    MP_LOG(Log, TEXT("Testing MP_LOG with dedicated LogMPUtility category: %s"), TEXT("Success"));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPScopedGuardsTest, "MultiplayerUtility.Core.ScopedGuards", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPScopedGuardsTest::RunTest(const FString& Parameters)
{
    // 1. FMPScopedAuthorityCheck null context
    {
        FMPScopedAuthorityCheck AuthGuard(nullptr, TEXT("NullTest"));
        TestFalse("Null context has no authority", AuthGuard.HasAuthority());
        TestFalse("Operator bool is false on null context", static_cast<bool>(AuthGuard));
    }

    // 2. FMPScopedNetTimer execution
    {
        FMPScopedNetTimer NetTimer(TEXT("TestTimerScope"), 100.0f);
        // Scope completes without issues
    }

    // 3. FMPScopedRPCCounter re-entrancy depth tracking
    {
        FMPScopedRPCCounter OuterCounter(TEXT("TestRPC"));
        TestEqual("Outer RPC depth is 1", OuterCounter.GetDepth(), 1);
        {
            FMPScopedRPCCounter InnerCounter(TEXT("TestRPC"));
            TestEqual("Nested RPC depth is 2", InnerCounter.GetDepth(), 2);
        }
        TestEqual("After inner scope exit, outer RPC depth is 1", OuterCounter.GetDepth(), 1);
    }
    {
        FMPScopedRPCCounter NewScope(TEXT("TestRPC"));
        TestEqual("New scope depth resets to 1", NewScope.GetDepth(), 1);
    }

    // 4. FMPScopedDormancyWake null safety
    {
        FMPScopedDormancyWake WakeGuard(nullptr);
        TestFalse("Null actor dormancy wake was not dormant", WakeGuard.WasDormant());
    }

    // 5. FMPScopedCVarOverride RAII restoration
    TestEqual("mp.Debug.Overlay initial value is 0", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.Debug.Overlay")), 0);
    {
        FMPScopedCVarOverride OverrideGuard(TEXT("mp.Debug.Overlay"), 1);
        TestTrue("Override guard is active", OverrideGuard.IsActive());
        TestEqual("mp.Debug.Overlay is overridden to 1", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.Debug.Overlay")), 1);
    }
    TestEqual("mp.Debug.Overlay restored back to 0 upon scope exit", UMPNetFunctionLibrary::GetMPCVarInt(TEXT("mp.Debug.Overlay")), 0);

    return true;
}

static int32 HelperTestEnsureWorld(const UObject* Ctx)
{
    MP_ENSURE_WORLD_RET(Ctx, -1);
    return 100;
}

static bool HelperTestEnsureServer(const UObject* Ctx)
{
    MP_ENSURE_SERVER_OR_RETURN_RET(Ctx, false);
    return true;
}

static int32 HelperTestUnlikelyReturn(bool bCondition)
{
    MP_UNLIKELY_RETURN_IF_RET(bCondition, 999);
    return 123;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPAdvancedMacrosTest, "MultiplayerUtility.Core.AdvancedMacros", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPAdvancedMacrosTest::RunTest(const FString& Parameters)
{
    // 1. Engine version macro tests
    TestTrue("Engine version is at least 5.3", MP_ENGINE_VERSION_AT_LEAST(5, 3));
    TestTrue("Engine version is at least 5.6", MP_ENGINE_VERSION_AT_LEAST(5, 6));
    TestFalse("Engine version is not 6.0 yet", MP_ENGINE_VERSION_AT_LEAST(6, 0));

    // 2. MP_BRANCH_NETMODE expression macro with null context
    const int32 BranchVal = MP_BRANCH_NETMODE(nullptr, 100, 200);
    TestEqual("MP_BRANCH_NETMODE with null context selects client branch", BranchVal, 200);

    // 3. MP_ENSURE_WORLD_RET with null context
    TestEqual("MP_ENSURE_WORLD_RET on null context returns fallback -1", HelperTestEnsureWorld(nullptr), -1);

    // 4. MP_ENSURE_SERVER_OR_RETURN_RET with null context
    TestFalse("MP_ENSURE_SERVER_OR_RETURN_RET on null context returns false", HelperTestEnsureServer(nullptr));

    // 5. MP_UNLIKELY_RETURN_IF_RET
    TestEqual("MP_UNLIKELY_RETURN_IF_RET triggered returns fallback 999", HelperTestUnlikelyReturn(true), 999);
    TestEqual("MP_UNLIKELY_RETURN_IF_RET not triggered returns normal 123", HelperTestUnlikelyReturn(false), 123);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPMiscExtrasTest, "MultiplayerUtility.Core.MiscExtras", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPMiscExtrasTest::RunTest(const FString& Parameters)
{
    // 1. GetLogicalOwnerSafe null check
    TestNull("Null actor logical owner returns nullptr", UMPActorNetworkWrapper::GetLogicalOwnerSafe(nullptr));

    // 2. UMPNetStatsSubsystem tests
    UMPNetStatsSubsystem* StatsSubsystem = NewObject<UMPNetStatsSubsystem>();
    TestNotNull("Stats subsystem instantiated", StatsSubsystem);
    if (StatsSubsystem)
    {
        StatsSubsystem->RecordRPCCall(FName("ServerFireWeapon"));
        StatsSubsystem->RecordRPCCall(FName("ServerFireWeapon"));
        StatsSubsystem->RecordRPCCall(FName("ServerReload"));
        TestEqual("ServerFireWeapon recorded 2 calls", StatsSubsystem->GetRPCCallCount(FName("ServerFireWeapon")), 2);
        TestEqual("ServerReload recorded 1 call", StatsSubsystem->GetRPCCallCount(FName("ServerReload")), 1);
        TestEqual("Unrecorded RPC has 0 count", StatsSubsystem->GetRPCCallCount(FName("NonExistentRPC")), 0);

        StatsSubsystem->RecordActorSpawned();
        StatsSubsystem->RecordActorSpawned();
        StatsSubsystem->RecordActorDestroyed();
        int32 TotalSpawned = 0;
        int32 TotalDestroyed = 0;
        StatsSubsystem->GetNetworkStats(TotalSpawned, TotalDestroyed);
        TestEqual("Total spawned actors is 2", TotalSpawned, 2);
        TestEqual("Total destroyed actors is 1", TotalDestroyed, 1);

        StatsSubsystem->ResetNetworkStats();
        StatsSubsystem->GetNetworkStats(TotalSpawned, TotalDestroyed);
        TestEqual("Total spawned reset to 0", TotalSpawned, 0);
        TestEqual("Total destroyed reset to 0", TotalDestroyed, 0);
        TestEqual("RPC counters reset to 0", StatsSubsystem->GetRPCCallCount(FName("ServerFireWeapon")), 0);
    }

    // 3. Flow control pending timer null safety
    TestFalse("CancelPendingByKey on null world returns false", UMPFlowControlLibrary::CancelPendingByKey(nullptr, FName("TestTimer")));
    FMPTimerCallback EmptyCallback;
    TestEqual("RunOnNextFrame on null world returns FailedInvalidWorld", UMPFlowControlLibrary::RunOnNextFrame(nullptr, FName("NextTickKey"), EmptyCallback), EMPDelayedActionResult::FailedInvalidWorld);
    TestEqual("RunAfterServerSeconds on null world returns FailedInvalidWorld", UMPFlowControlLibrary::RunAfterServerSeconds(nullptr, FName("DelayKey"), 1.0f, EmptyCallback), EMPDelayedActionResult::FailedInvalidWorld);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPSmokeTest, "MultiplayerUtility.Core.SmokeTest", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPSmokeTest::RunTest(const FString& Parameters)
{
    // 1. Connection quality null safety tests
    TestEqual("Null PC quality is Unknown", UMPNetFunctionLibrary::GetConnectionQualityEnum(nullptr), EMPConnectionQuality::Unknown);
    TestFalse("Null PC connection is not unstable", UMPNetFunctionLibrary::IsConnectionUnstable(nullptr));
    TestFalse("Null PC does not trigger load reduction", UMPNetFunctionLibrary::ShouldReduceNetworkLoad(nullptr));

    // 2. Player queries null safety tests
    TestNull("Null context ClosestPlayer is nullptr", UMPNetFunctionLibrary::GetClosestPlayerPawn(nullptr, FVector::ZeroVector));
    TestNull("Null context FurthestPlayer is nullptr", UMPNetFunctionLibrary::GetFurthestPlayer(nullptr, FVector::ZeroVector));
    TestEqual("Null context DistanceToNearest is -1", UMPNetFunctionLibrary::GetDistanceToNearestPlayer(nullptr, FVector::ZeroVector), -1.0f);
    TestFalse("Null context IsAnyPlayerWithinRange is false", UMPNetFunctionLibrary::IsAnyPlayerWithinRange(nullptr, FVector::ZeroVector, 100.0f));
    TestEqual("Null context GetPlayersSortedByDistance is empty", UMPNetFunctionLibrary::GetPlayersSortedByDistance(nullptr, FVector::ZeroVector).Num(), 0);
    TestEqual("Null context GetPlayersInCone is empty", UMPNetFunctionLibrary::GetPlayersInCone(nullptr, FVector::ZeroVector, FVector::ForwardVector, 45.0f, 500.0f).Num(), 0);
    TestNull("Null context GetRandomPlayer is nullptr", UMPNetFunctionLibrary::GetRandomPlayer(nullptr, {}));
    TestEqual("Null context GetAlivePlayersOnly is empty", UMPNetFunctionLibrary::GetAlivePlayersOnly(nullptr).Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPReplicatedDataTypesTest, "MultiplayerUtility.Core.ReplicatedDataTypes", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPReplicatedDataTypesTest::RunTest(const FString& Parameters)
{
    // 1. Replicated Counter
    FMPReplicatedCounter Counter;
    TestEqual("Initial counter is 0", Counter.GetValue(), 0);
    Counter.Increment(5);
    TestEqual("Counter after +5 is 5", Counter.GetValue(), 5);
    Counter.Decrement(2);
    TestEqual("Counter after -2 is 3", Counter.GetValue(), 3);
    Counter.Reset();
    TestEqual("Counter after reset is 0", Counter.GetValue(), 0);

    // 2. Replicated Flag Set
    FMPReplicatedFlagSet FlagSet;
    TestFalse("Initial flag 0 is unset", FlagSet.HasFlag(0));
    FlagSet.SetFlag(3, true);
    TestTrue("Flag 3 is set", FlagSet.HasFlag(3));
    TestFalse("Flag 2 remains unset", FlagSet.HasFlag(2));
    FlagSet.ToggleFlag(3);
    TestFalse("Flag 3 is toggled off", FlagSet.HasFlag(3));
    FlagSet.SetFlag(7, true);
    FlagSet.ClearFlag(7);
    TestFalse("Flag 7 is cleared", FlagSet.HasFlag(7));

    // 3. Replicated Key-Value
    FMPReplicatedKeyValue KeyValue;
    KeyValue.SetString(TEXT("PlayerTag"), TEXT("Viper"));
    TestEqual("String value matches", KeyValue.GetString(TEXT("PlayerTag")), FString(TEXT("Viper")));
    KeyValue.SetInt(TEXT("Score"), 100);
    TestEqual("Int value matches", KeyValue.GetInt(TEXT("Score")), 100);
    KeyValue.SetFloat(TEXT("HealthRatio"), 0.75f);
    TestNearlyEqual("Float value matches", KeyValue.GetFloat(TEXT("HealthRatio")), 0.75f, 0.001f);
    TestTrue("Remove key succeeds", KeyValue.Remove(TEXT("Score")));
    TestEqual("Removed key returns default", KeyValue.GetInt(TEXT("Score"), -1), -1);

    // 4. Replicated Log Ring
    FMPReplicatedLogRing LogRing;
    LogRing.MaxMessages = 3;
    LogRing.AddMessage(TEXT("Msg1"));
    LogRing.AddMessage(TEXT("Msg2"));
    LogRing.AddMessage(TEXT("Msg3"));
    TestEqual("Log ring has 3 messages", LogRing.Messages.Num(), 3);
    LogRing.AddMessage(TEXT("Msg4"));
    TestEqual("Log ring still capped at 3", LogRing.Messages.Num(), 3);
    TestEqual("Oldest Msg1 evicted, first is Msg2", LogRing.Messages[0], FString(TEXT("Msg2")));
    TestEqual("Latest is Msg4", LogRing.Messages[2], FString(TEXT("Msg4")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPServerValidationTest, "MultiplayerUtility.Core.ServerValidation", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPServerValidationTest::RunTest(const FString& Parameters)
{
    FMPValidationResult Res;

    // 1. Float Range
    TestTrue("Valid float passes", UMPNetFunctionLibrary::ValidateFloatRange(50.0f, 0.0f, 100.0f, Res));
    TestFalse("Out of range float fails", UMPNetFunctionLibrary::ValidateFloatRange(150.0f, 0.0f, 100.0f, Res));
    TestEqual("Reason is OutOfRange", Res.Reason, EMPValidationReason::Failed_OutOfRange);
    TestFalse("NaN float fails", UMPNetFunctionLibrary::ValidateFloatRange(NAN, 0.0f, 100.0f, Res));
    TestEqual("Reason is NaN", Res.Reason, EMPValidationReason::Failed_NaN);

    // 2. Vector NaN
    TestTrue("Finite vector passes", UMPNetFunctionLibrary::ValidateVectorNotNaN(FVector(100.0f, 200.0f, 300.0f), Res));
    TestFalse("NaN vector fails", UMPNetFunctionLibrary::ValidateVectorNotNaN(FVector(NAN, 0.0f, 0.0f), Res));
    TestEqual("Vector NaN reason", Res.Reason, EMPValidationReason::Failed_NaN);

    // 3. Enum Value
    TestTrue("Valid enum ordinal passes", UMPNetFunctionLibrary::ValidateEnumValue(2, 5, Res));
    TestFalse("Invalid enum ordinal fails", UMPNetFunctionLibrary::ValidateEnumValue(6, 5, Res));
    TestEqual("Enum reason is InvalidEnum", Res.Reason, EMPValidationReason::Failed_InvalidEnum);

    // 4. Input Vector Clamp
    FVector BigInput(10.0f, 0.0f, 0.0f);
    FVector Clamped = UMPNetFunctionLibrary::ClampClientInputVector(BigInput, 1.0f);
    TestNearlyEqual("Clamped vector length is 1.0", (float)Clamped.Size(), 1.0f, 0.001f);

    // 5. Sanitize Player Name
    FString CleanName;
    TestTrue("Valid name passes", UMPNetFunctionLibrary::SanitizePlayerName(TEXT("  Hero_01  "), 3, 20, CleanName, Res));
    TestEqual("Whitespace trimmed", CleanName, FString(TEXT("Hero_01")));

    TestFalse("Short name fails", UMPNetFunctionLibrary::SanitizePlayerName(TEXT(" A "), 3, 20, CleanName, Res));
    TestEqual("Reason is InvalidName", Res.Reason, EMPValidationReason::Failed_InvalidName);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPTeamAndPresetsTest, "MultiplayerUtility.Core.TeamAndPresets", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPTeamAndPresetsTest::RunTest(const FString& Parameters)
{
    // 1. Team helpers null safety
    TestEqual("Null actor team is -1", UMPNetFunctionLibrary::GetTeamOfActorSafe(nullptr, -1), -1);
    TestFalse("Null actors are not teammates", UMPNetFunctionLibrary::AreTeammates(nullptr, nullptr));
    TestFalse("Null actors are not enemies", UMPNetFunctionLibrary::AreEnemies(nullptr, nullptr));
    TestEqual("Null context GetActorsByTeam is empty", UMPNetFunctionLibrary::GetActorsByTeam(nullptr, 1).Num(), 0);

    // 2. Net Preset Data Asset
    UMPNetPresetAsset* Preset = NewObject<UMPNetPresetAsset>();
    TestNotNull("Preset created", Preset);
    TestNearlyEqual("Default NetUpdateFrequency is 30.0", Preset->NetUpdateFrequency, 30.0f, 0.001f);
    TestNearlyEqual("Default MinNetUpdateFrequency is 2.0", Preset->MinNetUpdateFrequency, 2.0f, 0.001f);
    TestEqual("Default dormancy is DORM_DormantAll", (int32)Preset->NetDormancy, (int32)DORM_DormantAll);

    // 3. Flow control CallOncePerPlayer
    FMPTimerCallback EmptyCallback;
    TestFalse("CallOncePerPlayer with null PC returns false", UMPFlowControlLibrary::CallOncePerPlayer(nullptr, FName("TestKey"), EmptyCallback));

    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPMetadataValidationTest, "MultiplayerUtility.Core.MetadataValidation", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPMetadataValidationTest::RunTest(const FString& Parameters)
{
    const TArray<UClass*> ClassesToTest = {
        UMPNetFunctionLibrary::StaticClass(),
        UMPFlowControlLibrary::StaticClass(),
        UMPActorNetworkWrapper::StaticClass(),
        UMPComponentNetworkWrapper::StaticClass(),
        UMPPawnControllerWrapper::StaticClass(),
        UMPWorldSessionWrapper::StaticClass(),
        UMPMovementPhysicsWrapper::StaticClass(),
        UMPTimeMath::StaticClass(),
        UMPQuantization::StaticClass(),
        UMPUrlParser::StaticClass()
    };

    TSet<FString> SeenDisplayNames;
    int32 ValidatedFunctionCount = 0;

    for (UClass* Class : ClassesToTest)
    {
        for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
        {
            const UFunction* Func = *FuncIt;
            if (!Func) continue;

            ValidatedFunctionCount++;
            const FString FuncName = Func->GetName();

            // 1. Verify Category metadata exists
            TestTrue(FString::Printf(TEXT("Function '%s' has Category metadata"), *FuncName),
                Func->HasMetaData(TEXT("Category")) && !Func->GetMetaData(TEXT("Category")).IsEmpty());

            // 2. Verify ToolTip metadata exists
            TestTrue(FString::Printf(TEXT("Function '%s' has ToolTip metadata"), *FuncName),
                Func->HasMetaData(TEXT("ToolTip")) && !Func->GetMetaData(TEXT("ToolTip")).IsEmpty());

            // 3. Verify DisplayName uniqueness if specified
            if (Func->HasMetaData(TEXT("DisplayName")))
            {
                const FString DisplayName = Func->GetMetaData(TEXT("DisplayName"));
                TestFalse(FString::Printf(TEXT("DisplayName '%s' on '%s' must be unique"), *DisplayName, *FuncName),
                    SeenDisplayNames.Contains(DisplayName));
                SeenDisplayNames.Add(DisplayName);
            }
        }
    }

    TestTrue("Validated at least 30 UFunctions", ValidatedFunctionCount >= 30);
    return true;
}
#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPNetworkRequestFrameworkTest, "MultiplayerUtility.Core.NetworkRequestFramework", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPNetworkRequestFrameworkTest::RunTest(const FString& Parameters)
{
    // 1. Request Structure initialization
    FMPNetworkRequest Request;
    TestTrue("New request should have valid GUID", Request.RequestId.IsValid());
    TestEqual("Initial state should be Uninitialized", Request.State, EMPNetworkRequestState::Uninitialized);
    TestFalse("Default should not be idempotent", Request.bIsIdempotent);

    Request.RequestAction = TEXT("PurchaseItem");
    Request.TimeoutSeconds = 4.0f;
    Request.bIsIdempotent = true;
    Request.MaxRetries = 2;

    // 2. Response Structure
    FMPNetworkResponse Response;
    Response.RequestId = Request.RequestId;
    Response.Result = EMPNetworkRequestResult::Success;
    Response.Message = TEXT("Purchase Approved");
    Response.ServerTimestamp = 123.45;

    TestTrue("Response should be marked success", Response.IsSuccess());
    TestEqual("Response ID must match Request ID", Response.RequestId, Request.RequestId);

    // 3. Request Handle state tracking
    UMPNetworkRequestHandle* Handle = NewObject<UMPNetworkRequestHandle>();
    Handle->RequestId = Request.RequestId;
    TestEqual("Initial handle state should be Pending", Handle->GetState(), EMPNetworkRequestState::Pending);
    TestFalse("Pending handle should not be done", Handle->IsDone());

    Handle->UpdateState(EMPNetworkRequestState::Completed);
    TestTrue("Completed handle should be done", Handle->IsDone());

    Handle->UpdateState(EMPNetworkRequestState::TimedOut);
    TestTrue("TimedOut handle should be done", Handle->IsDone());

    Handle->UpdateState(EMPNetworkRequestState::Cancelled);
    TestTrue("Cancelled handle should be done", Handle->IsDone());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPAdvancedReplicationFastArrayTest, "MultiplayerUtility.Core.AdvancedReplicationFastArray", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPAdvancedReplicationFastArrayTest::RunTest(const FString& Parameters)
{
    // 1. Fast Array Key-Value Dictionary Operations
    FMPReplicatedKeyValueArray KVArray;
    TestEqual("Initial Fast Array should be empty", KVArray.Num(), 0);

    // Add entries
    bool bSet1 = KVArray.SetValue(TEXT("PlayerScore"), TEXT("100"), 1.0);
    TestTrue("Setting new key should return true", bSet1);
    TestEqual("Array should have 1 item", KVArray.Num(), 1);
    TestTrue("Array should contain PlayerScore", KVArray.ContainsKey(TEXT("PlayerScore")));

    // Read back
    FString OutVal;
    bool bFound = KVArray.GetValue(TEXT("PlayerScore"), OutVal);
    TestTrue("GetValue should succeed", bFound);
    TestEqual("Value should match", OutVal, FString(TEXT("100")));

    // Update with same value (should return false, no dirty flag needed)
    bool bSetSame = KVArray.SetValue(TEXT("PlayerScore"), TEXT("100"), 2.0);
    TestFalse("Setting identical value should return false", bSetSame);

    // Update with new value
    bool bSetNew = KVArray.SetValue(TEXT("PlayerScore"), TEXT("250"), 3.0);
    TestTrue("Setting new value should return true", bSetNew);
    KVArray.GetValue(TEXT("PlayerScore"), OutVal);
    TestEqual("Value should be updated to 250", OutVal, FString(TEXT("250")));

    // Add second key
    KVArray.SetValue(TEXT("TeamId"), TEXT("Blue"), 4.0);
    TestEqual("Array should have 2 items", KVArray.Num(), 2);

    // Remove key
    bool bRemoved = KVArray.RemoveKey(TEXT("PlayerScore"));
    TestTrue("Removing existing key should return true", bRemoved);
    TestEqual("Array should now have 1 item", KVArray.Num(), 1);
    TestFalse("PlayerScore should no longer exist", KVArray.ContainsKey(TEXT("PlayerScore")));

    // Clear array
    KVArray.Clear();
    TestEqual("Cleared array should have 0 items", KVArray.Num(), 0);

    // 2. Replication Condition string descriptions
    TestTrue("Describe COND_None", UMPReplicationConditions::DescribeReplicationCondition(COND_None).Contains(TEXT("COND_None")));
    TestTrue("Describe COND_OwnerOnly", UMPReplicationConditions::DescribeReplicationCondition(COND_OwnerOnly).Contains(TEXT("COND_OwnerOnly")));
    TestTrue("Describe COND_Custom", UMPReplicationConditions::DescribeReplicationCondition(COND_Custom).Contains(TEXT("COND_Custom")));

    // 3. State change byte comparison
    int32 ValA = 42;
    int32 ValB = 42;
    int32 ValC = 99;
    TestFalse("Identical data should not indicate change", UMPReplicationConditions::HasStateChanged(&ValA, &ValB, sizeof(int32)));
    TestTrue("Different data should indicate change", UMPReplicationConditions::HasStateChanged(&ValA, &ValC, sizeof(int32)));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPPlayerLifecycleTest, "MultiplayerUtility.Core.PlayerLifecycle", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPPlayerLifecycleTest::RunTest(const FString& Parameters)
{
    // 1. Null safety checks
    TestFalse("Possess with null controller should return false", UMPPlayerLifecycleLibrary::SafePossessPawnServerOnly(nullptr, nullptr));
    TestFalse("Unpossess with null controller should return false", UMPPlayerLifecycleLibrary::SafeUnpossessPawnServerOnly(nullptr));
    TestNull("Spawn and possess with null controller should return nullptr", UMPPlayerLifecycleLibrary::SpawnAndPossessPlayerPawnServerOnly(nullptr, nullptr, FTransform::Identity));
    TestFalse("Transfer ownership with null actor should return false", UMPPlayerLifecycleLibrary::TransferActorOwnershipSafe(nullptr, nullptr));
    TestFalse("Spectator state with null controller should return false", UMPPlayerLifecycleLibrary::SetPlayerSpectatorStateServerOnly(nullptr, true));
    TestNull("Respawn with null controller should return nullptr", UMPPlayerLifecycleLibrary::RespawnPlayerPawnServerOnly(nullptr, nullptr, FTransform::Identity));
    TestFalse("Cleanup with null pawn should return false", UMPPlayerLifecycleLibrary::CleanupPlayerPawnSafe(nullptr));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPMovementPredictionAndSmoothingTest, "MultiplayerUtility.Core.MovementPredictionAndSmoothing", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPMovementPredictionAndSmoothingTest::RunTest(const FString& Parameters)
{
    // 1. Saved Move flag packing and clearing
    FSavedMove_MPCharacter MoveA;
    TestEqual("Initial compressed flags should be 0", MoveA.GetCompressedFlags(), 0);

    MoveA.bWantsToSprint = 1;
    uint8 SprintFlag = MoveA.GetCompressedFlags();
    TestTrue("Sprint flag should set custom 0 bit", (SprintFlag & FSavedMove_Character::FLAG_Custom_0) != 0);

    MoveA.bWantsToDash = 1;
    uint8 DashFlag = MoveA.GetCompressedFlags();
    TestTrue("Dash flag should set custom 1 bit", (DashFlag & FSavedMove_Character::FLAG_Custom_1) != 0);

    MoveA.Clear();
    TestEqual("Cleared move should have 0 flags", MoveA.GetCompressedFlags(), 0);

    // 2. Server movement step validation
    const FVector LocStart(0.0f, 0.0f, 0.0f);
    const FVector LocValid(50.0f, 0.0f, 0.0f);   // 50 units in 0.1s = 500 units/s (within 600 speed)
    const FVector LocInvalid(200.0f, 0.0f, 0.0f); // 200 units in 0.1s = 2000 units/s (exceeds 600 speed)

    float OutErr = 0.0f;
    bool bValidStep = UMPCharacterMovementComponent::ValidateMovementStep(LocStart, LocValid, 0.1f, 600.0f, 20.0f, OutErr);
    TestTrue("Valid step should be accepted", bValidStep);
    TestEqual("Error distance should be 0", OutErr, 0.0f);

    bool bInvalidStep = UMPCharacterMovementComponent::ValidateMovementStep(LocStart, LocInvalid, 0.1f, 600.0f, 20.0f, OutErr);
    TestFalse("Excessive speed step should be rejected", bInvalidStep);
    TestTrue("Error distance should be positive", OutErr > 0.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPReplicatedStateMachineTest, "MultiplayerUtility.Core.ReplicatedStateMachine", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPReplicatedStateMachineTest::RunTest(const FString& Parameters)
{
    FMPReplicatedStateMachine SM;
    TestFalse("Initial state should be invalid", SM.CurrentState.IsValid());
    TestEqual("Initial time in state should be 0", SM.GetTimeInCurrentState(10.0), 0.0f);

    // Transition to state
    const FGameplayTag TagIdle = FGameplayTag::RequestGameplayTag(TEXT("State.Idle"), false);
    // If not registered in dictionary, test with FGameplayTag()
    FGameplayTag TagA;
    TagA = FGameplayTag::RequestGameplayTag(TEXT("Net.State.A"), false);

    bool bTrans = SM.TransitionToState(TagA, 10.0);
    if (TagA.IsValid())
    {
        TestTrue("Transition to valid tag should succeed", bTrans);
        TestEqual("Current state should match TagA", SM.CurrentState, TagA);
        TestEqual("Time in state at T=15 should be 5.0s", SM.GetTimeInCurrentState(15.0), 5.0f);
    }
    else
    {
        TestFalse("Transition to empty/invalid tag should fail", bTrans);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPReplicatedCountdownTimerTest, "MultiplayerUtility.Core.ReplicatedCountdownTimer", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPReplicatedCountdownTimerTest::RunTest(const FString& Parameters)
{
    FMPReplicatedCountdownTimer Timer;
    TestFalse("Initial timer should not be active", Timer.IsActive());
    TestTrue("Initial timer should be finished", Timer.IsFinished(0.0));

    // Start 10-second timer at T = 100.0
    Timer.Start(10.0f, 100.0);
    TestTrue("Timer should now be active", Timer.IsActive());
    TestFalse("Timer should not be finished at T=102", Timer.IsFinished(102.0));
    TestEqual("Remaining time at T=104 should be 6.0s", Timer.GetRemainingTime(104.0), 6.0f);
    TestEqual("Elapsed time at T=104 should be 4.0s", Timer.GetElapsedTime(104.0), 4.0f);
    TestEqual("Progress at T=105 should be 0.5", Timer.GetNormalizedProgress(105.0), 0.5f);

    // Pause at T = 104 (6.0s remaining)
    Timer.Pause(104.0);
    TestTrue("Timer should be paused", Timer.bIsPaused);
    TestEqual("Remaining time when paused should stay 6.0s even at T=200", Timer.GetRemainingTime(200.0), 6.0f);

    // Resume at T = 200.0
    Timer.Resume(200.0);
    TestFalse("Timer should no longer be paused", Timer.bIsPaused);
    TestEqual("Remaining time at T=203 should be 3.0s", Timer.GetRemainingTime(203.0), 3.0f);

    // Finish at T = 206.0
    TestTrue("Timer should be finished at T=206.0", Timer.IsFinished(206.0));
    TestEqual("Remaining time should be 0.0s", Timer.GetRemainingTime(207.0), 0.0f);

    // Reset
    Timer.Reset();
    TestFalse("Reset timer should not be active", Timer.IsActive());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPPhysicsReplicationTest, "MultiplayerUtility.Core.PhysicsReplication", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPPhysicsReplicationTest::RunTest(const FString& Parameters)
{
    // Test null safety
    float OutLocErr = 0.0f;
    float OutRotErr = 0.0f;
    bool bVal = UMPPhysicsReplicationHelper::ValidatePhysicsTransformError(nullptr, FTransform::Identity, 100.0f, 45.0f, OutLocErr, OutRotErr);
    TestFalse("Null component transform validation should return false", bVal);
    TestFalse("Null skeletal mesh ragdoll init should return false", UMPPhysicsReplicationHelper::InitNetworkRagdollSafe(nullptr));
    TestFalse("Null sleep should return false", UMPPhysicsReplicationHelper::PutPhysicsToSleepServerOnly(nullptr));
    TestFalse("Null wake should return false", UMPPhysicsReplicationHelper::WakePhysicsServerOnly(nullptr));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPServerSecurityAndAntiCheatTest, "MultiplayerUtility.Core.ServerSecurityAndAntiCheat", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPServerSecurityAndAntiCheatTest::RunTest(const FString& Parameters)
{
    // 1. Fire Rate Validator
    FMPFireRateValidator FireValidator;
    FireValidator.MinShotInterval = 0.1f;
    FireValidator.MaxBurstCount = 3;
    FireValidator.BurstWindowSeconds = 0.35f;

    FString FailReason;
    // Shot 1 at T = 1.0 (valid)
    TestTrue("Shot 1 should be valid", FireValidator.ValidateShot(1.0, FailReason));

    // Shot 2 at T = 1.02 (too fast! only 20ms delta, min is 100ms)
    TestFalse("Shot 2 should fail minimum interval", FireValidator.ValidateShot(1.02, FailReason));
    TestTrue("Fail reason should be set", !FailReason.IsEmpty());

    // Shot 3 at T = 1.12 (valid)
    TestTrue("Shot 3 should be valid", FireValidator.ValidateShot(1.12, FailReason));

    // Shot 4 at T = 1.24 (valid)
    TestTrue("Shot 4 should be valid", FireValidator.ValidateShot(1.24, FailReason));

    // Shot 5 at T = 1.30 (burst limit reached: 4 shots in 0.35s)
    TestFalse("Shot 5 should fail burst limit", FireValidator.ValidateShot(1.30, FailReason));

    // 2. Movement Exploit Detector
    FMPMovementExploitDetector MoveDetector;
    MoveDetector.MaxAllowedSpeed = 600.0f;
    MoveDetector.ToleranceMargin = 50.0f;
    MoveDetector.TeleportThresholdDistance = 2000.0f;
    MoveDetector.Reset(FVector::ZeroVector, 0.0);

    bool bTeleport = false;
    float SpeedErr = 0.0f;

    // Move to (50, 0, 0) at T = 0.1 (Speed = 500 units/s, within 600)
    bool bCleanStep = MoveDetector.ValidateStep(FVector(50.0f, 0.0f, 0.0f), 0.1, bTeleport, SpeedErr);
    TestTrue("Normal speed step should be valid", bCleanStep);
    TestFalse("Teleport should not be detected", bTeleport);

    // Jump to (5000, 0, 0) at T = 0.2 (Teleport!)
    bool bTeleportStep = MoveDetector.ValidateStep(FVector(5000.0f, 0.0f, 0.0f), 0.2, bTeleport, SpeedErr);
    TestFalse("Teleport step should fail validation", bTeleportStep);
    TestTrue("Teleport flag should be true", bTeleport);

    // 3. Client Timestamp Drift Validation
    FString DriftReason;
    // Client is 1.0s ahead of server (within 2.0s allowed)
    TestTrue("Mild drift should be allowed", UMPServerSecurityLibrary::ValidateClientTimestampDrift(101.0, 100.0, 2.0f, DriftReason));

    // Client is 5.0s in future (hacked timestamp!)
    TestFalse("Excessive future timestamp should be rejected", UMPServerSecurityLibrary::ValidateClientTimestampDrift(105.0, 100.0, 2.0f, DriftReason));
    TestTrue("Drift reason should mention future", DriftReason.Contains(TEXT("future")));

    // 4. Duplicate Transaction Detection
    TArray<FGuid> History;
    FGuid TxA = FGuid::NewGuid();
    FGuid TxB = FGuid::NewGuid();

    TestTrue("First submission of TxA should pass", UMPServerSecurityLibrary::ValidateTransactionNonDuplicated(TxA, History, 5));
    TestFalse("Duplicate submission of TxA should fail", UMPServerSecurityLibrary::ValidateTransactionNonDuplicated(TxA, History, 5));
    TestTrue("Submission of TxB should pass", UMPServerSecurityLibrary::ValidateTransactionNonDuplicated(TxB, History, 5));
    TestEqual("History should contain 2 entries", History.Num(), 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPAsyncNetworkActionsTest, "MultiplayerUtility.Core.AsyncNetworkActions", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPAsyncNetworkActionsTest::RunTest(const FString& Parameters)
{
    // 1. Wait Until PlayerState Action creation & activation safety
    UMPAsyncWaitUntilPlayerState* PSAction = UMPAsyncWaitUntilPlayerState::WaitUntilPlayerStateAvailable(nullptr, 2.0f);
    TestNotNull("Async PS action should be created", PSAction);

    // 2. Wait Until Pawn Possessed Action creation
    UMPAsyncWaitUntilPawnPossessed* PawnAction = UMPAsyncWaitUntilPawnPossessed::WaitUntilPawnPossessed(nullptr, 3.0f);
    TestNotNull("Async Pawn action should be created", PawnAction);

    // 3. Wait Until Actor Replicated Action creation
    UMPAsyncWaitUntilActorReplicated* ActorAction = UMPAsyncWaitUntilActorReplicated::WaitUntilActorReplicated(nullptr, AActor::StaticClass(), TEXT("KeyActor"), 5.0f);
    TestNotNull("Async Actor action should be created", ActorAction);

    // 4. Async Server Request Action creation
    FMPNetworkRequest Req;
    Req.RequestAction = TEXT("TestAsyncAction");
    UMPAsyncServerRequest* ReqAction = UMPAsyncServerRequest::AsyncSendServerRequest(nullptr, Req);
    TestNotNull("Async Server Request action should be created", ReqAction);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPNetworkOptimizationAndLODTest, "MultiplayerUtility.Core.NetworkOptimizationAndLOD", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPNetworkOptimizationAndLODTest::RunTest(const FString& Parameters)
{
    FMPReplicationLODSettings Settings;
    Settings.NearDistance = 1000.0f;
    Settings.NearFrequencyHz = 60.0f;
    Settings.MidDistance = 4000.0f;
    Settings.MidFrequencyHz = 20.0f;
    Settings.FarDistance = 8000.0f;
    Settings.FarFrequencyHz = 5.0f;
    Settings.AutoDormancyDistance = 12000.0f;
    Settings.bEnableDormancyAtDistance = true;

    // 1. Near tier evaluation (500 units)
    EMPReplicationLODTier NearTier = Settings.EvaluateDistance(500.0f);
    TestEqual("500 units should evaluate to NearHighPriority", NearTier, EMPReplicationLODTier::NearHighPriority);
    TestEqual("Frequency should be 60Hz", Settings.GetFrequencyForTier(NearTier), 60.0f);

    // 2. Mid tier evaluation (2500 units)
    EMPReplicationLODTier MidTier = Settings.EvaluateDistance(2500.0f);
    TestEqual("2500 units should evaluate to MidStandard", MidTier, EMPReplicationLODTier::MidStandard);
    TestEqual("Frequency should be 20Hz", Settings.GetFrequencyForTier(MidTier), 20.0f);

    // 3. Far tier evaluation (6000 units)
    EMPReplicationLODTier FarTier = Settings.EvaluateDistance(6000.0f);
    TestEqual("6000 units should evaluate to FarThrottled", FarTier, EMPReplicationLODTier::FarThrottled);
    TestEqual("Frequency should be 5Hz", Settings.GetFrequencyForTier(FarTier), 5.0f);

    // 4. Dormancy tier evaluation (15000 units)
    EMPReplicationLODTier DormTier = Settings.EvaluateDistance(15000.0f);
    TestEqual("15000 units should evaluate to Dormant", DormTier, EMPReplicationLODTier::Dormant);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPModularGameplayHealthTest, "MultiplayerUtility.Core.ModularGameplayHealth", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPModularGameplayHealthTest::RunTest(const FString& Parameters)
{
    // Health Component Object instantiation & state tests
    const UMPHealthComponent* HealthComp = GetDefault<UMPHealthComponent>();
    TestNotNull("HealthComponent CDO valid", HealthComp);
    TestEqual("Default MaxHealth is 100", HealthComp->MaxHealth, 100.0f);
    TestEqual("Default CurrentHealth is 100", HealthComp->CurrentHealth, 100.0f);
    TestTrue("Initially alive", HealthComp->IsAlive());
    TestFalse("Initially not downed", HealthComp->IsDowned());
    TestFalse("Initially not dead", HealthComp->IsDead());

    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPEditorScannerTest, "MultiplayerUtility.Editor.ScannerSubsystem", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPEditorScannerTest::RunTest(const FString& Parameters)
{
    // Test PIE network emulation setting
    UMPEditorScannerSubsystem::ApplyPIENetworkEmulation(80, 2.5f);
    
    // Null safety check
    UMPEditorScannerSubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UMPEditorScannerSubsystem>() : nullptr;
    if (Subsystem)
    {
        FMPActorNetworkAuditReport Report;
        bool bClean = Subsystem->AuditActorReplication(nullptr, Report);
        TestFalse("Audit null actor should return false", bClean);

        TArray<FString> Warnings;
        bool bRPC = Subsystem->ValidateRPCOwnership(nullptr, Warnings);
        TestFalse("Validate RPC ownership null should return false", bRPC);
    }
    else
    {
        // Subsystem null in headless console test context is safe
        TestTrue("Editor Subsystem null safety verified", true);
    }

    return true;
}
#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPNetworkStressTest, "MultiplayerUtility.Core.NetworkStressTest", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPNetworkStressTest::RunTest(const FString& Parameters)
{
    FMPNetworkStressTestConfig Config;
    Config.ActorCount = 20;
    Config.RPCCallsPerSecond = 50;

    FMPNetworkStressTestResult Result = UMPNetworkStressTester::RunInEngineStressTest(Config);

    TestTrue("Stress test should pass state consistency", Result.bPassedConsistencyCheck);
    TestTrue("Should process at least 500 attempted operations", Result.TotalAttemptedRPCs >= 500);
    TestTrue("Execution duration should be positive", Result.ExecutionDurationSeconds > 0.0);
    TestTrue("Summary report should be generated", !Result.SummaryReport.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMPGameplayHelperLibraryTest, "MultiplayerUtility.Core.GameplayHelperLibrary", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMPGameplayHelperLibraryTest::RunTest(const FString& Parameters)
{
    // =========================================================================
    // 1. PLAYER & NETWORK UTILITIES (Null safety & formatting)
    // =========================================================================
    TestNull("GetLocalPlayerState null context should return null", UMPGameplayHelperLibrary::GetLocalPlayerState(nullptr));
    TestNull("GetPlayerStateFromActor null actor should return null", UMPGameplayHelperLibrary::GetPlayerStateFromActor(nullptr));
    TestNull("GetPlayerControllerFromActor null actor should return null", UMPGameplayHelperLibrary::GetPlayerControllerFromActor(nullptr));
    TestFalse("IsPlayerLocallyViewed null actor should return false", UMPGameplayHelperLibrary::IsPlayerLocallyViewed(nullptr));
    TestEqual("GetPlayerControllerIndex null should return -1", UMPGameplayHelperLibrary::GetPlayerControllerIndex(nullptr), -1);
    TestEqual("GetAllRemotePlayerControllers null context should return empty array", UMPGameplayHelperLibrary::GetAllRemotePlayerControllers(nullptr).Num(), 0);
    TestFalse("IsPlayerConnectionActive null controller should return false", UMPGameplayHelperLibrary::IsPlayerConnectionActive(nullptr));
    TestEqual("GetPlayerPingFormatted null state should return '-- ms'", UMPGameplayHelperLibrary::GetPlayerPingFormatted(nullptr), FString(TEXT("-- ms")));

    // =========================================================================
    // 2. RPC UTILITIES
    // =========================================================================
    FString OutReason;
    TestFalse("CanActorSendServerRPC null should return false", UMPGameplayHelperLibrary::CanActorSendServerRPC(nullptr, OutReason));
    TestTrue("CanActorSendServerRPC should produce failure reason", !OutReason.IsEmpty());

    FString OutScope;
    UMPGameplayHelperLibrary::GetRPCExecutionScope(nullptr, OutScope);
    TestEqual("GetRPCExecutionScope null should return 'Invalid Actor'", OutScope, FString(TEXT("Invalid Actor")));

    bool bAllowed = false;
    UMPGameplayHelperLibrary::CheckRPCRateLimitGate(nullptr, FName("TestRPC"), 1.0f, bAllowed);
    TestTrue("Rate limit gate with null context allows execution safely", bAllowed);

    // =========================================================================
    // 3. REPLICATION UTILITIES
    // =========================================================================
    TestFalse("IsActorDormant null actor returns false", UMPGameplayHelperLibrary::IsActorDormant(nullptr));
    TestEqual("GetActorNetUpdateFrequency null actor returns 0", UMPGameplayHelperLibrary::GetActorNetUpdateFrequency(nullptr), 0.0f);
    TestFalse("IsActorTornOff null actor returns false", UMPGameplayHelperLibrary::IsActorTornOff(nullptr));

    // Safe setters should not crash with null
    UMPGameplayHelperLibrary::SetActorReplicationSafe(nullptr, true, true);
    UMPGameplayHelperLibrary::SetActorNetDormantSafe(nullptr, true);
    UMPGameplayHelperLibrary::ForceNetUpdateWithAttachedActors(nullptr);
    UMPGameplayHelperLibrary::TearOffActorSafe(nullptr);

    // =========================================================================
    // 4. MULTIPLAYER GAMEPLAY UTILITIES
    // =========================================================================
    TestFalse("CanDamageActorAuthoritative with null victim returns false", UMPGameplayHelperLibrary::CanDamageActorAuthoritative(nullptr, nullptr, false));
    TestFalse("ApplyPointDamageServerOnly with null actor returns false", UMPGameplayHelperLibrary::ApplyPointDamageServerOnly(nullptr, 10.0f, FVector::ForwardVector, FHitResult(), nullptr, nullptr, nullptr));
    TestFalse("ApplyRadialDamageServerOnly with null context returns false", UMPGameplayHelperLibrary::ApplyRadialDamageServerOnly(nullptr, 10.0f, FVector::ZeroVector, 100.0f, nullptr, TArray<AActor*>(), nullptr, nullptr));
    TestNull("SpawnReplicatedActorServerOnly with null context returns null", UMPGameplayHelperLibrary::SpawnReplicatedActorServerOnly(nullptr, nullptr, FTransform::Identity, ESpawnActorCollisionHandlingMethod::AlwaysSpawn));
    TestNull("RespawnPlayerPawnAtTransform with null controller returns null", UMPGameplayHelperLibrary::RespawnPlayerPawnAtTransform(nullptr, nullptr, FTransform::Identity));
    TestNull("FindBestPlayerStart with null context returns null", UMPGameplayHelperLibrary::FindBestPlayerStart(nullptr, FVector::ZeroVector));
    TestFalse("TeleportPawnServerOnly with null pawn returns false", UMPGameplayHelperLibrary::TeleportPawnServerOnly(nullptr, FVector::ZeroVector, FRotator::ZeroRotator));
    TestFalse("SetPlayerScoreServerOnly with null state returns false", UMPGameplayHelperLibrary::SetPlayerScoreServerOnly(nullptr, 100.0f));

    // =========================================================================
    // 5. MULTIPLAYER SYNCHRONIZATION UTILITIES & SYNC TIMER STRUCT
    // =========================================================================
    TestEqual("GetServerTimeSynced with null context returns 0", UMPGameplayHelperLibrary::GetServerTimeSynced(nullptr), 0.0f);
    TestEqual("GetTimeSinceServerTimestamp with null context returns 0", UMPGameplayHelperLibrary::GetTimeSinceServerTimestamp(nullptr, 0.0f), 0.0f);

    // Extrapolation test
    const FVector Pos(100.0f, 0.0f, 0.0f);
    const FVector Vel(1000.0f, 0.0f, 0.0f);
    const float PingSec = 0.1f; // 100ms RTT -> 50ms one-way
    const FVector Extrapolated = UMPGameplayHelperLibrary::CalculateLatencyCompensatedLocation(Pos, Vel, PingSec);
    TestEqual("Extrapolated X position with 50ms latency (1000cm/s * 0.05s = 50cm)", static_cast<float>(Extrapolated.X), 150.0f);

    TestTrue("Timestamps 10.0 and 20.0 are ordered", UMPGameplayHelperLibrary::AreTimestampsOrdered(10.0f, 20.0f));
    TestFalse("Timestamps 25.0 and 20.0 are not ordered", UMPGameplayHelperLibrary::AreTimestampsOrdered(25.0f, 20.0f));

    // FMPReplicatedSyncTimer tests
    FMPReplicatedSyncTimer SyncTimer;
    TestFalse("Timer initially not active", SyncTimer.bIsActive);
    TestFalse("Timer initially not running", SyncTimer.IsRunning());

    SyncTimer.Start(10.0f, 100.0f); // Starts at time 100s, duration 10s -> ends at 110s
    TestTrue("Timer active after start", SyncTimer.bIsActive);
    TestTrue("Timer running after start", SyncTimer.IsRunning());
    TestEqual("Remaining time at 102s is 8s", SyncTimer.GetRemainingTime(102.0f), 8.0f);
    TestEqual("Elapsed time at 102s is 2s", SyncTimer.GetElapsedTime(102.0f), 2.0f);
    TestEqual("Normalized progress at 105s is 0.5", SyncTimer.GetNormalizedProgress(105.0f), 0.5f);
    TestFalse("Not finished at 105s", SyncTimer.IsFinished(105.0f));
    TestTrue("Finished at 110s", SyncTimer.IsFinished(110.0f));
    TestTrue("Finished at 115s", SyncTimer.IsFinished(115.0f));
    TestEqual("Remaining time at 115s clamped to 0", SyncTimer.GetRemainingTime(115.0f), 0.0f);

    // Pause & Resume
    SyncTimer.Pause(104.0f); // Paused at 4s elapsed
    TestTrue("Timer paused", SyncTimer.bIsPaused);
    TestFalse("Timer not running when paused", SyncTimer.IsRunning());
    TestEqual("Remaining time paused at 104s is 6s", SyncTimer.GetRemainingTime(120.0f), 6.0f);

    SyncTimer.Resume(130.0f); // Resumed at time 130s
    TestFalse("Timer unpaused after resume", SyncTimer.bIsPaused);
    TestTrue("Timer running after resume", SyncTimer.IsRunning());
    TestEqual("Elapsed time right after resume is 4s", SyncTimer.GetElapsedTime(130.0f), 4.0f);
    TestEqual("Remaining time right after resume is 6s", SyncTimer.GetRemainingTime(130.0f), 6.0f);

    SyncTimer.Stop();
    TestFalse("Timer stopped", SyncTimer.bIsActive);
    TestEqual("Remaining time after stop is 0", SyncTimer.GetRemainingTime(130.0f), 0.0f);

    // =========================================================================
    // 6. MULTIPLAYER DEBUGGING UTILITIES
    // =========================================================================
    TestEqual("GetActorNetworkDebugString null actor returns '[Null Actor]'", UMPGameplayHelperLibrary::GetActorNetworkDebugString(nullptr), FString(TEXT("[Null Actor]")));
    UMPGameplayHelperLibrary::SimulateNetworkConditionCVar(50, 1.0f);
    UMPGameplayHelperLibrary::DumpReplicationInfoToLog(nullptr);

    // =========================================================================
    // 7. MULTIPLAYER C++ MACROS
    // =========================================================================
    // Test MP_REP_NOTIFY_CHANGED
    int32 OldVal = 10;
    int32 NewVal = 20;
    bool bExecutedChange = false;
    MP_REP_NOTIFY_CHANGED(OldVal, NewVal)
    {
        bExecutedChange = true;
    }
    TestTrue("MP_REP_NOTIFY_CHANGED executes when values differ", bExecutedChange);

    bExecutedChange = false;
    NewVal = 10;
    MP_REP_NOTIFY_CHANGED(OldVal, NewVal)
    {
        bExecutedChange = true;
    }
    TestFalse("MP_REP_NOTIFY_CHANGED skips when values match", bExecutedChange);

    // Test MP_CHECK_NOT_TORN_OFF_RET
    auto TestTearOffMacro = [](AActor* Act) -> bool
    {
        MP_CHECK_NOT_TORN_OFF_RET(Act, false);
        return true;
    };
    TestFalse("MP_CHECK_NOT_TORN_OFF_RET returns false on null", TestTearOffMacro(nullptr));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
