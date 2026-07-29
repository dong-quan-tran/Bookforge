add_executable(benchmark_order_book
  benchmark_order_book.cpp
)

target_link_libraries(benchmark_order_book PRIVATE
  benchmark::benchmark
  benchmark::benchmark_main
  bookforge_core
)

target_compile_features(benchmark_order_book PRIVATE cxx_std_20)

add_executable(benchmark_replay
  benchmark_replay.cpp
  ../src/HyperliquidCsvReader.cpp
  ../src/replay/ReplayRunner.cpp
)

target_include_directories(benchmark_replay PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(benchmark_replay PRIVATE
  benchmark::benchmark
  benchmark::benchmark_main
  bookforge_core
)

target_compile_features(benchmark_replay PRIVATE cxx_std_20)