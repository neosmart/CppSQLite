#pragma once
#include "CppSQLite3.h"
#include <fmt/core.h>
#include <gtest/gtest.h>

#define EXPECT_THROW_WITH_MSG(statement, expected_exception, message)                                                  \
    try                                                                                                                \
    {                                                                                                                  \
        {                                                                                                              \
            statement;                                                                                                 \
        }                                                                                                              \
        FAIL() << "Expected: " #statement " throws an exception of type " << #expected_exception                       \
               << ".\n Actual:  it throws nothing.";                                                                   \
    }                                                                                                                  \
    catch (expected_exception const& e)                                                                                \
    {                                                                                                                  \
        EXPECT_STREQ(message, e.what());                                                                               \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        FAIL() << "Expected: " << #statement << " throws an exception of type " << #expected_exception                 \
               << ".\n Actual:  it throws a different exception.";                                                     \
    }

namespace CustomExceptions
{
class InvalidQuery : public std::logic_error
{
public:
    InvalidQuery(const std::string& msg) : std::logic_error(msg)
    {
    }
};
class SQLiteError : public std::runtime_error
{
public:
    SQLiteError(const std::string& msg) : std::runtime_error(msg)
    {
    }
};

void throwException(int code, int extendedErrorCode, std::string_view msg, std::string_view context)
{
    std::string finalMsg = std::string(msg);
    if (!context.empty())
    {
        finalMsg += fmt::format(" {}", context);
    }
    if (code != SQLITE_ERROR)
    {
        finalMsg += fmt::format(" (Code {})", code);
        if (code != extendedErrorCode)
        {
            finalMsg += fmt::format(" (EECode {})", extendedErrorCode);
        }
    }
    if (code == SQLITE_ERROR)
    {
        throw CustomExceptions::InvalidQuery(finalMsg);
    }
    else
    {
        throw CustomExceptions::SQLiteError(finalMsg);
    }
}
} // namespace CustomExceptions
