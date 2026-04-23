add_library(engine STATIC
    order_book.cpp
)

target_include_directories(engine PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Build the interactive dev/test executable
add_executable(engine_dev main_dev.cpp)
target_link_libraries(engine_dev PRIVATE engine)
