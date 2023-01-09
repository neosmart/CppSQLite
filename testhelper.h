#pragma once
#include "CppSQLite3.h"
#include <gtest/gtest.h>

#define EXPECT_THROW_CPPSQLITE( statement, message )                                                \
    try {                                                                                                              \
        { statement; }                                                                                             \
        FAIL() << "Expected: " #statement                                                                                \
        " throws an exception of type CppSQLite3Exception.\n Actual:  it throws nothing.";                       \
    } catch( CppSQLite3Exception const& e ) {                                                                               \
        EXPECT_STREQ( message, e.errorMessage() );                                                                             \
    } catch( ... ) {                                                                                                   \
    FAIL() << "Expected: " << #statement <<                                                                               \
    " throws an exception of type CppSQLite3Exception.\n Actual:  it throws a different exception.";             \
    }

#define EXPECT_THROW_WITH_MSG( statement, expected_exception, message )                                                \
    try {                                                                                                              \
        { statement; }                                                                                             \
        FAIL() << "Expected: " #statement                                                                                \
        " throws an exception of type " << #expected_exception << ".\n Actual:  it throws nothing.";                       \
    } catch( expected_exception const& e ) {                                                                               \
        EXPECT_STREQ( message, e.what() );                                                                             \
    } catch( ... ) {                                                                                                   \
    FAIL() << "Expected: " << #statement <<                                                                               \
    " throws an exception of type " << #expected_exception << ".\n Actual:  it throws a different exception.";             \
    }

namespace CustomExceptions {
class InvalidQuery : public std::logic_error {
public:
    InvalidQuery(const std::string& msg) : std::logic_error(msg) {

    }
};
class SQLiteError : public std::runtime_error {
public:
    SQLiteError(const std::string& msg) : std::runtime_error(msg) {

    }
};

void throwException(int code, const char* msg, const char* context) {
    std::string finalMsg = msg;
    if(strlen(context) > 0) {
        finalMsg += std::string(" ") + context;
    }
    if(code != 1) {
        finalMsg += " (Code " + std::to_string(code) + ")";
    }

    if(code == SQLITE_ERROR) {
        throw CustomExceptions::InvalidQuery(finalMsg);
    }
    else {
        throw CustomExceptions::SQLiteError(finalMsg);
    }
}
}
