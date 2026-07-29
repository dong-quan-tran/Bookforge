function(bookforge_add_benchmark target)
  add_executable(${target} ${ARGN})
  target_link_libraries(${target} PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bookforge_core
  )
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
  )
  target_compile_features(${target} PRIVATE cxx_std_20)
endfunction()

bookforge_add_benchmark(benchmark_order_book
  benchmark_order_book.cpp
)

bookforge_add_benchmark(benchmark_replay
  benchmark_replay.cpp
  ../src/HyperliquidCsvReader.cpp
  ../src/replay/ReplayRunner.cpp
)