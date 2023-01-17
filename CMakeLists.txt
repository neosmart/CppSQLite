cmake_minimum_required(VERSION 3.4)
project(CppSQLite3)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared library" OFF)

set(CMAKE_MODULE_PATH ${PROJECT_BINARY_DIR})
find_package(SQLite3 REQUIRED)
find_package(GTest REQUIRED)


add_library(${CMAKE_PROJECT_NAME}
    CppSQLite3.cpp
    CppSQLite3.h)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "cppsqlite3$<$<CONFIG:Debug>:d>"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

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

if(MSVC)
# OutOfMemory test fixture was only tested with msvc and crashes on linux/gcc.
# This doesn't mean the library doesn't work on linux, just that the test setup
# using sqlite3_config is not correct yet.
add_executable(OutOfMemoryTest
    testhelper.h
    outofmemory.test.cpp
)

add_test(NAME OutOfMemoryTest COMMAND OutOfMemoryTest)

target_link_libraries(OutOfMemoryTest
    ${CMAKE_PROJECT_NAME}
    GTest::gtest_main
)

endif(MSVC)
