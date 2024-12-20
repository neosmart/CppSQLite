cmake_minimum_required(VERSION 3.16)
project(CppSQLite3)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared library" OFF)

set(CMAKE_MODULE_PATH ${PROJECT_BINARY_DIR})
find_package(SQLite3 REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)


add_library(${CMAKE_PROJECT_NAME} CppSQLite3.h CppSQLite3.cpp)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "cppsqlite3$<$<CONFIG:Debug>:d>"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PUBLIC SQLite::SQLite3
    PRIVATE fmt::fmt
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

endif()

set(CPACK_PACKAGE_VENDOR "Bruker Daltonics GmbH")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "c++ wrapper for sqlite library")
string(TIMESTAMP TODAY "%Y%m%d")
set(CPACK_PACKAGE_VERSION "${TODAY}")
set(CPACK_PACKAGE_FILE_NAME ${CMAKE_PROJECT_NAME}-fork-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CPACK_PACKAGE_VERSION})
install(TARGETS ${CMAKE_PROJECT_NAME}
        #EXPORT ${CMAKE_PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include)
        
install(FILES CppSQLite3.h TYPE INCLUDE)
if(MSVC AND BUILD_SHARED_LIBS)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
endif()
install(FILES LICENSE DESTINATION .)

set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /dist
    /.*build.*
    /\\\\.DS_Store
)

include(CPack)

