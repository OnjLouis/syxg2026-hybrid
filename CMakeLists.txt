cmake_minimum_required(VERSION 3.25)

project(SYXG2026Hybrid VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32 OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "S-YXG2026 Hybrid currently requires a 32-bit Windows target")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(SYXG2026Hybrid SHARED
    Source/MidiRouter.cpp
    Source/MidiRouter.h
    Source/NativeSgClient.cpp
    Source/NativeSgClient.h
    Source/NativeVlClient.cpp
    Source/NativeVlClient.h
    Source/NativeVlProtocol.h
    Source/OrderedSetupHistory.h
    Source/SgRouting.cpp
    Source/SgRouting.h
    Source/StreamingRateAdapter.cpp
    Source/StreamingRateAdapter.h
    Source/VlPartRouter.cpp
    Source/VlPartRouter.h
    Source/XgEffectsBridge.cpp
    Source/XgEffectsBridge.h
    Source/XgPartModes.cpp
    Source/XgPartModes.h
    Source/XgVariationRouting.cpp
    Source/XgVariationRouting.h
    Source/XglEngine.cpp
    Source/XglEngine.h
    Source/XglVoiceMap.cpp
    Source/XglVoiceMap.h
    Source/HybridVst.cpp
    Source/Vst2Abi.h
    Source/exports.def
)
set_target_properties(SYXG2026Hybrid PROPERTIES
    OUTPUT_NAME "syxg2026-hybrid"
    PREFIX ""
)
target_compile_definitions(SYXG2026Hybrid PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_options(SYXG2026Hybrid PRIVATE -static -static-libgcc -static-libstdc++)

add_executable(SYXG2026VlWorker
    NativeWorker/VlNativeWorker.cpp
    Source/LeImageLoader.cpp
    Source/LeImageLoader.h
    Source/NativeVlEngine.cpp
    Source/NativeVlEngine.h
    Source/NativeVlProtocol.h
)
set_target_properties(SYXG2026VlWorker PROPERTIES
    OUTPUT_NAME "syxg2026-vl-worker"
)
target_compile_definitions(SYXG2026VlWorker PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_options(SYXG2026VlWorker PRIVATE
    -municode -static -static-libgcc "-static-libstdc++"
)

add_executable(SYXG2026SgWorker
    NativeWorker/SgNativeWorker.cpp
    Source/LeImageLoader.cpp
    Source/LeImageLoader.h
    Source/NativeSgEngine.cpp
    Source/NativeSgEngine.h
    Source/NativeVlProtocol.h
)
set_target_properties(SYXG2026SgWorker PROPERTIES
    OUTPUT_NAME "syxg2026-sg-worker"
)
target_compile_definitions(SYXG2026SgWorker PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_options(SYXG2026SgWorker PRIVATE
    -municode -static -static-libgcc "-static-libstdc++"
)

add_executable(HybridHostProbe
    Tests/HybridHostProbe.cpp
    Source/Vst2Abi.h
)
target_compile_definitions(HybridHostProbe PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_options(HybridHostProbe PRIVATE -static -static-libgcc -static-libstdc++)

add_executable(HybridXgForwardingProbe
    Tests/HybridXgForwardingProbe.cpp
    Source/Vst2Abi.h
)
target_compile_definitions(HybridXgForwardingProbe PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX
)
target_link_options(HybridXgForwardingProbe PRIVATE
    -static -static-libgcc -static-libstdc++
)

add_executable(HybridDualInstanceProbe
    Tests/HybridDualInstanceProbe.cpp
    Source/Vst2Abi.h
)
target_compile_definitions(HybridDualInstanceProbe PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX
)
target_link_options(HybridDualInstanceProbe PRIVATE
    -static -static-libgcc -static-libstdc++
)

enable_testing()
add_executable(MidiRouterTests
    Tests/MidiRouterTests.cpp
    Source/MidiRouter.cpp
    Source/MidiRouter.h
)
target_include_directories(MidiRouterTests PRIVATE Source)
target_link_options(MidiRouterTests PRIVATE -static -static-libgcc -static-libstdc++)
add_test(NAME MidiRouterTests COMMAND MidiRouterTests)

add_executable(LeImageLoaderTests
    Tests/LeImageLoaderTests.cpp
    Source/LeImageLoader.cpp
    Source/LeImageLoader.h
)
target_include_directories(LeImageLoaderTests PRIVATE Source)
target_link_options(LeImageLoaderTests PRIVATE -static -static-libgcc -static-libstdc++)
add_test(NAME LeImageLoaderTests COMMAND LeImageLoaderTests)

add_executable(VlPartRouterTests
    Tests/VlPartRouterTests.cpp
    Source/VlPartRouter.cpp
    Source/VlPartRouter.h
)
target_include_directories(VlPartRouterTests PRIVATE Source)
target_link_options(VlPartRouterTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME VlPartRouterTests COMMAND VlPartRouterTests)

add_executable(SgRoutingTests
    Tests/SgRoutingTests.cpp
    Source/SgRouting.cpp
    Source/SgRouting.h
)
target_include_directories(SgRoutingTests PRIVATE Source)
target_link_options(SgRoutingTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME SgRoutingTests COMMAND SgRoutingTests)

add_executable(NativeEventTimelineTests
    Tests/NativeEventTimelineTests.cpp
    Source/NativeEventTimeline.h
)
target_include_directories(NativeEventTimelineTests PRIVATE Source)
target_link_options(NativeEventTimelineTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME NativeEventTimelineTests COMMAND NativeEventTimelineTests)

add_executable(OrderedMidiHistoryTests
    Tests/OrderedSetupHistoryTests.cpp
    Source/OrderedSetupHistory.h
)
target_include_directories(OrderedMidiHistoryTests PRIVATE Source)
target_link_options(OrderedMidiHistoryTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME OrderedMidiHistoryTests COMMAND OrderedMidiHistoryTests)

add_executable(MidiChannelSnapshotTests
    Tests/MidiChannelSnapshotTests.cpp
    Source/MidiChannelSnapshot.h
)
target_include_directories(MidiChannelSnapshotTests PRIVATE Source)
target_link_options(MidiChannelSnapshotTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME MidiChannelSnapshotTests COMMAND MidiChannelSnapshotTests)

add_executable(VlVoiceAllocatorTests
    Tests/VlVoiceAllocatorTests.cpp
    Source/VlVoiceAllocator.h
)
target_include_directories(VlVoiceAllocatorTests PRIVATE Source)
target_link_options(VlVoiceAllocatorTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME VlVoiceAllocatorTests COMMAND VlVoiceAllocatorTests)

add_executable(StreamingRateAdapterTests
    Tests/StreamingRateAdapterTests.cpp
    Source/StreamingRateAdapter.cpp
    Source/StreamingRateAdapter.h
)
target_include_directories(StreamingRateAdapterTests PRIVATE Source)
target_link_options(StreamingRateAdapterTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME StreamingRateAdapterTests COMMAND StreamingRateAdapterTests)

add_executable(XglVoiceMapTests
    Tests/XglVoiceMapTests.cpp
    Source/XglVoiceMap.cpp
    Source/XglVoiceMap.h
)
target_include_directories(XglVoiceMapTests PRIVATE Source)
target_link_options(XglVoiceMapTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME XglVoiceMapTests COMMAND XglVoiceMapTests)

add_executable(XgPartModesTests
    Tests/XgPartModesTests.cpp
    Source/XgPartModes.cpp
    Source/XgPartModes.h
)
target_include_directories(XgPartModesTests PRIVATE Source)
target_link_options(XgPartModesTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME XgPartModesTests COMMAND XgPartModesTests)

add_executable(XgVariationRoutingTests
    Tests/XgVariationRoutingTests.cpp
    Source/XgVariationRouting.cpp
    Source/XgVariationRouting.h
)
target_include_directories(XgVariationRoutingTests PRIVATE Source)
target_link_options(XgVariationRoutingTests PRIVATE
    -static -static-libgcc -static-libstdc++
)
add_test(NAME XgVariationRoutingTests COMMAND XgVariationRoutingTests)
