add_executable(tests test_order_book.cpp)
target_link_libraries(tests PRIVATE engine)

enable_testing()
add_test(
    NAME  OrderBookTests
    COMMAND tests
)
