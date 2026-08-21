cmake_minimum_required(VERSION 3.20)
project(Bookforge VERSION 0.1.0 LANGUAGES CXX)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG v2.13.6
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest pybind11)

include(CTest)
include(GoogleTest)

set(BOOKFORGE_PY_PACKAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/python/bookforge_py)
file(MAKE_DIRECTORY ${BOOKFORGE_PY_PACKAGE_DIR})

option(BOOKFORGE_ENABLE_BENCHMARKS "Build Bookforge benchmarks" ON)

add_library(bookforge_core
  src/core/price_level.cpp
  src/core/order_book.cpp
  src/core/matching_engine.cpp
  src/replay/LatencyHistogram.cpp
  src/replay/ReplayClock.cpp
  src/snapshot/SnapshotBuilder.cpp
  src/snapshot/SnapshotSerializer.cpp
  src/snapshot/SnapshotComparator.cpp
  src/snapshot/SnapshotDeserializer.cpp
  src/snapshot/SnapshotBinarySerializer.cpp
  src/snapshot/SnapshotBinaryDeserializer.cpp
  src/features/FeatureBuilder.cpp
  src/features/FeatureCsvWriter.cpp
  src/features/OfiFeatureBuilder.cpp
  src/features/RollingFeatureBuilder.cpp
)

target_include_directories(bookforge_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

function(bookforge_add_test target)
  add_executable(${target} ${ARGN})
  target_link_libraries(${target} PRIVATE
    bookforge_core
    GTest::gtest_main
  )
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  gtest_discover_tests(${target})
endfunction()

bookforge_add_test(test_order_book
  tests/cpp/test_order_book.cpp
)

bookforge_add_test(test_matching_engine
  tests/cpp/test_matching_engine.cpp
)

bookforge_add_test(test_hyperliquid_matching_engine_adapter
  tests/cpp/test_hyperliquid_matching_engine_adapter.cpp
)

bookforge_add_test(test_hyperliquid_csv_reader
  tests/cpp/test_hyperliquid_csv_reader.cpp
  src/HyperliquidCsvReader.cpp
)

bookforge_add_test(test_replay_runner
  tests/cpp/test_replay_runner.cpp
  src/replay/ReplayRunner.cpp
)

bookforge_add_test(test_replay_regression
  tests/cpp/test_replay_regression.cpp
  src/HyperliquidCsvReader.cpp
  src/replay/ReplayRunner.cpp
)
target_compile_definitions(test_replay_regression PRIVATE
  BOOKFORGE_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)

bookforge_add_test(test_replay_adapter
  tests/cpp/test_replay_adapter.cpp
  src/replay/ReplayRunner.cpp
)

bookforge_add_test(test_replay_injected_orders
  tests/cpp/test_replay_injected_orders.cpp
  src/replay/ReplayRunner.cpp
)

bookforge_add_test(test_replay_clock
  tests/cpp/test_replay_clock.cpp
)

bookforge_add_test(test_latency_histogram
  tests/cpp/test_latency_histogram.cpp
)

bookforge_add_test(test_strategy_experiment
  tests/cpp/test_strategy_experiment.cpp
  src/replay/StrategyExperiment.cpp
)

bookforge_add_test(test_strategy_experiment_csv_writer
  tests/cpp/test_strategy_experiment_csv_writer.cpp
  src/replay/StrategyExperimentCsvWriter.cpp
)

bookforge_add_test(test_strategy_experiment_runner
  tests/cpp/test_strategy_experiment_runner.cpp
  src/replay/StrategyExperimentRunner.cpp
  src/replay/StrategyExperiment.cpp
  src/replay/StrategyExperimentAdapter.cpp
  src/replay/ReplayRunner.cpp
)

bookforge_add_test(test_strategy_experiment_comparison
  tests/cpp/test_strategy_experiment_comparison.cpp
  src/replay/StrategyExperimentComparison.cpp
  src/replay/StrategyExperimentRunner.cpp
  src/replay/StrategyExperiment.cpp
  src/replay/StrategyExperimentAdapter.cpp
  src/replay/StrategyExperimentCsvWriter.cpp
  src/replay/ReplayRunner.cpp
)

bookforge_add_test(test_matching_engine_integration
  tests/cpp/test_matching_engine_integration.cpp
)

bookforge_add_test(test_snapshot
  tests/cpp/test_snapshot.cpp
)

bookforge_add_test(test_feature_builder
  tests/cpp/test_feature_builder.cpp
)

bookforge_add_test(test_strategy_experiment_adapter
  tests/cpp/test_strategy_experiment_adapter.cpp
  src/replay/StrategyExperimentAdapter.cpp
  src/replay/StrategyExperiment.cpp
)

add_executable(hyperliquid_replay_main
  src/hyperliquid_replay_main.cpp
  src/HyperliquidCsvReader.cpp
  src/replay/ReplayRunner.cpp
  src/replay/StrategyExperiment.cpp
  src/replay/StrategyExperimentCsvWriter.cpp
  src/replay/StrategyExperimentRunner.cpp
  src/replay/StrategyExperimentAdapter.cpp
)
target_include_directories(hyperliquid_replay_main PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(hyperliquid_replay_main PRIVATE
  bookforge_core
)

add_executable(strategy_experiment_main
  src/strategy_experiment_main.cpp
  src/HyperliquidCsvReader.cpp
  src/replay/ReplayRunner.cpp
  src/replay/StrategyExperiment.cpp
  src/replay/StrategyExperimentCsvWriter.cpp
  src/replay/StrategyExperimentRunner.cpp
  src/replay/StrategyExperimentAdapter.cpp
)
target_include_directories(strategy_experiment_main PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(strategy_experiment_main PRIVATE
  bookforge_core
)

add_executable(feature_export_main
  src/feature_export_main.cpp
  src/HyperliquidCsvReader.cpp
)
target_include_directories(feature_export_main PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(feature_export_main PRIVATE
  bookforge_core
)

pybind11_add_module(bookforge_py
  src/python/bookforge_py.cpp
)
target_include_directories(bookforge_py PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(bookforge_py PRIVATE
  bookforge_core
)
target_compile_features(bookforge_py PRIVATE cxx_std_20)

set_target_properties(bookforge_py PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${BOOKFORGE_PY_PACKAGE_DIR}
  RUNTIME_OUTPUT_DIRECTORY ${BOOKFORGE_PY_PACKAGE_DIR}
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python_artifacts
)

if(WIN32)
  set_target_properties(bookforge_py PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BOOKFORGE_PY_PACKAGE_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BOOKFORGE_PY_PACKAGE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BOOKFORGE_PY_PACKAGE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BOOKFORGE_PY_PACKAGE_DIR}
  )
endif()

if(BOOKFORGE_ENABLE_BENCHMARKS)
  FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
  )
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googlebenchmark)

  set(BOOKFORGE_BENCH_REPLAY_FIXTURE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hyperliquid_replay_fixture_large.csv"
  )
  cmake_path(NORMAL_PATH BOOKFORGE_BENCH_REPLAY_FIXTURE)

  add_subdirectory(bench)

  target_compile_definitions(benchmark_replay PRIVATE
    BOOKFORGE_BENCH_FIXTURE_FILE="${BOOKFORGE_BENCH_REPLAY_FIXTURE}"
  )
endif()