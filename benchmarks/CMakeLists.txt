# Benchmarking registers the aggregate benchmarks target in standalone builds.
# Production CDCL does not link the Benchmarking framework.
add_executable(benchmark_cdcl_propagation benchmark_cdcl_propagation.cpp)
target_link_libraries(benchmark_cdcl_propagation PRIVATE
    ariadne-cdcl cdcl ariadne-benchmarking)
add_dependencies(benchmarks benchmark_cdcl_propagation)
