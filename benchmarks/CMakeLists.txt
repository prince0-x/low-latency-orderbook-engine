add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE engine)

# The benchmark reads data/orders.txt relative to where it runs.
# Set the working directory so CTest can invoke it correctly.
set_target_properties(benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
