cmake_minimum_required(VERSION 3.4)
project(CppSQLite)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${PROJECT_BINARY_DIR})
find_package(SQLite3 REQUIRED)
find_package(GTest REQUIRED)


add_library(${CMAKE_PROJECT_NAME}
    CppSQLite3.cpp
    CppSQLite3.h)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PUBLIC SQLite::SQLite3
)

enable_testing()
add_executable(cppSqliteTest
    testhelper.h
    cppsqlite.test.cpp
)

add_test(NAME cppSqliteTest COMMAND cppSqliteTest)

target_link_libraries(cppSqliteTest
    ${CMAKE_PROJECT_NAME}
    GTest::gtest_main
)

add_executable(OutOfMemoryTest
    testhelper.h
    outofmemory.test.cpp
)
add_test(NAME OutOfMemoryTest COMMAND OutOfMemoryTest)

target_link_libraries(OutOfMemoryTest
    ${CMAKE_PROJECT_NAME}
    GTest::gtest_main
)
