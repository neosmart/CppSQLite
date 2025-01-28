#include "CppSQLite3.h"
#include "testhelper.h"

#include <filesystem>
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
// Log handler is a function pointer, so we can't provide capturing lambdas in our tests
// Instead provide this singleton. Don't forget to getRecords().clear() after usage!
std::vector<std::string>& getRecords()
{
    static std::vector<std::string> records;
    return records;
}

void removeIfExists(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

} // namespace

TEST(ExecQueryTest, throwsOnSyntaxError)
{
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);");
    EXPECT_THROW_WITH_MSG(db.execQuery("SELCT * FROM myTable"), CppSQLite3Exception,
                          "SQLITE_ERROR[1]: near \"SELCT\": syntax error");
}


TEST(ExecQueryTest, throwsWhenInsertStatementMissesFields)
{
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);");
    EXPECT_THROW_WITH_MSG(db.execQuery("INSERT INTO `myTable` VALUES(\"some text\")"), CustomExceptions::InvalidQuery,
                          "table myTable has 2 columns but 1 values were supplied when compiling statement");
}

TEST(ExecQueryTest, selectOneRow)
{
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);");
    db.execQuery("INSERT INTO myTable VALUES(42, \"some text\")");
    auto result = db.execQuery("SELECT * FROM myTable");
    EXPECT_EQ(42, result.getIntField("ID"));
    EXPECT_STREQ("some text", result.getStringField("INFO"));
    result.nextRow();
    EXPECT_TRUE(result.eof());
}

TEST(DbTest, openNonExistentFileInReadOnly_ShouldThrow)
{
    CppSQLite3DB db;
    EXPECT_THROW_WITH_MSG(db.open("nowhere.sqlite", SQLITE_OPEN_READONLY), CppSQLite3Exception,
                          "SQLITE_CANTOPEN[14]: unable to open database file");
}

TEST(DbTest, writingToReadOnlyDatabaseShouldFail)
{
    CppSQLite3DB db;
    db.open(":memory:", SQLITE_OPEN_READONLY);
    EXPECT_THROW_WITH_MSG(db.execDML("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);"),
                          CppSQLite3Exception, "SQLITE_READONLY[8]: attempt to write a readonly database");
}

TEST(DbTest, tableExists)
{
    CppSQLite3DB db;
    db.open(":memory:");
    ASSERT_FALSE(db.tableExists("myTable"));
    db.execDML("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);");
    ASSERT_TRUE(db.tableExists("myTable"));
}

TEST(ErrorHandlerTest, dbThrowsOnInvalidOpenPath)
{
    CppSQLite3DB db;
    EXPECT_THROW_WITH_MSG(db.open("nonExistentFolder/test.sql"), CppSQLite3Exception,
                          "SQLITE_CANTOPEN[14]: unable to open database file");
}

TEST(ErrorHandlerTest, dbExecQueryThrowsCustomExceptionOnSyntaxError)
{
    CppSQLite3DB db;

    db.setErrorHandler(CustomExceptions::throwException);

    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);");

    EXPECT_THROW_WITH_MSG(db.execQuery("SELCT * FROM myTable"), CustomExceptions::InvalidQuery,
                          "near \"SELCT\": syntax error when compiling statement");
}

TEST(ErrorHandlerTest, dbExecDMLThrowsCustomExceptionOnSyntaxError)
{
    CppSQLite3DB db;
    db.setErrorHandler(CustomExceptions::throwException);
    db.open(":memory:");
    EXPECT_THROW_WITH_MSG(db.execDML("CRETE TABLE `myTable` (`ID` INT);"), CustomExceptions::InvalidQuery,
                          "near \"CRETE\": syntax error when executing DML query");
}

TEST(ErrorHandlerTest, dbThrowsLogicErrorWhenNotOpened)
{
    CppSQLite3DB db;
    db.setErrorHandler(CustomExceptions::throwException);
    EXPECT_THROW_WITH_MSG(db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);"), std::logic_error, "Database not open");
}

TEST(OpenCloseTest, closeAfterExceptionOnOpen)
{
    CppSQLite3DB db;
    ASSERT_ANY_THROW(db.open("notExisting.sqlite", SQLITE_OPEN_READONLY));
    db.close();
}

TEST(OpenCloseTest, closeWithoutOpen)
{
    CppSQLite3DB db;
    db.close();
}

TEST(OpenCloseTest, closeWithoutFinalizingQuery)
{
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);");
    auto query = db.execQuery("SELECT * FROM myTable");
    EXPECT_THROW_WITH_MSG(db.close(), CppSQLite3Exception,
                          "SQLITE_BUSY[5]: unable to close due to unfinalized "
                          "statements or unfinished backups");
}

TEST(OpenCloseTest, destructWithoutFinalizingQuery)
{
    CppSQLite3Query query;
    {
        CppSQLite3DB db;
        db.setLogHandler(
            [](CppSQLite3LogLevel level, std::string_view message)
            {
                ASSERT_EQ(CppSQLite3LogLevel::error, level.code);
                getRecords().emplace_back(message);
            });
        db.open(":memory:");
        db.execQuery("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);");
        query = db.execQuery("SELECT * FROM myTable");
        // error, query survives db
    }
    std::vector<std::string> expected = {"error during ~CppSQLite3DB: SQLITE_BUSY[5]: unable to close due to "
                                         "unfinalized statements or unfinished backups"};
    ASSERT_EQ(expected, getRecords());
    getRecords().clear();
}

TEST(CppSQLite3QueryTest, moveOperatorTransfersVMHandle)
{
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    CppSQLite3Query query = db.execQuery("SELECT * FROM myTable");
    ASSERT_TRUE(query.eof());
    auto query2 = std::move(query);
    EXPECT_THROW_WITH_MSG(query.eof(), std::logic_error, "Null Virtual Machine pointer");
    ASSERT_TRUE(query2.eof());
}

TEST(CppSQLite3StatementTest, moveOperatorTransfersVMHandle)
{
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    CppSQLite3Statement stmt = db.compileStatement("SELECT * FROM myTable");
    ASSERT_NO_THROW(stmt.execQuery());
    auto stmt2 = std::move(stmt);
    EXPECT_THROW_WITH_MSG(stmt.execQuery(), std::logic_error, "Null Virtual Machine pointer");
    ASSERT_NO_THROW(stmt2.execQuery());
}

TEST(CppSQLite3DBTest, openTwice)
{
    CppSQLite3DB db;
    db.open(":memory:");
    EXPECT_THROW_WITH_MSG(db.open(":memory:"), std::logic_error, "Previous db handle was not closed");
    db.close();
    EXPECT_NO_THROW(db.open(":memory:"));
}

TEST(CppSQLite3DBTest, runCheckpoint)
{
    removeIfExists("checkpointTest.sqlite");
    CppSQLite3DB db;
    db.open("checkpointTest.sqlite");
    db.execQuery("PRAGMA journal_mode=wal");
    db.execDML("CREATE TABLE `myTable` (`INFO` TEXT);");
    ASSERT_GT(std::filesystem::file_size("checkpointTest.sqlite-wal"), 0);

    ASSERT_NO_THROW(db.performCheckpoint("", SQLITE_CHECKPOINT_TRUNCATE));
    ASSERT_EQ(std::filesystem::file_size("checkpointTest.sqlite-wal"), 0);
}

TEST(CppSQLite3DBTest, verboseLoggingWithDefaultLogHandler)
{
    CppSQLite3DB db;
    db.open(":memory:");
    db.enableVerboseLogging(true);
    db.execDML("CREATE TABLE `myTable` (`INFO` TEXT);");
    db.execDML("INSERT INTO `myTable` VALUES(\"some text\")");
    db.execQuery("SELECT* FROM `myTable`");
    auto stmt = db.compileStatement("SELECT* FROM `myTable` WHERE INFO = ?");
    stmt.execQuery();
    auto dmlStmt = db.compileStatement("INSERT INTO `myTable` VALUES(?)");
    dmlStmt.bind(1, "some name");
    dmlStmt.execDML();
}

TEST(CppSQLite3DBTest, verboseLoggingWithCustomHandler)
{
    CppSQLite3DB db;
    db.open(":memory:");
    db.enableVerboseLogging(true);

    // std::vector<std::string> records;

    db.setLogHandler(
        [](CppSQLite3LogLevel level, std::string_view message)
        {
            ASSERT_EQ(level.code, CppSQLite3LogLevel::verbose);
            ASSERT_EQ(level.name, "Verbose");
            getRecords().emplace_back(message);
        });
    db.execDML("CREATE TABLE `myTable` (`INFO` TEXT);");
    db.execDML("INSERT INTO `myTable` VALUES(\"some text\")");
    db.execQuery("SELECT* FROM `myTable`");
    auto stmt = db.compileStatement("SELECT* FROM `myTable` WHERE INFO = ?");
    stmt.execQuery();
    auto dmlStmt = db.compileStatement("INSERT INTO `myTable` VALUES(?)");
    dmlStmt.bind(1, "some name");
    dmlStmt.execDML();
    std::vector<std::string> expectedMessages = {"CREATE TABLE `myTable` (`INFO` TEXT);",
                                                 "INSERT INTO `myTable` VALUES(\"some text\")",
                                                 "SELECT* FROM `myTable`", "SELECT* FROM `myTable` WHERE INFO = NULL",
                                                 "INSERT INTO `myTable` VALUES('some name')"};
    ASSERT_EQ(expectedMessages, getRecords());
    getRecords().clear();
}


TEST(StringViewTest, createStringView)
{
    std::string_view test;
    auto stdString = std::string("test");
    CppSQLite3StringView view = stdString;
    ASSERT_STREQ(view.c_str(), "test");
    ASSERT_EQ(stdString.c_str(), view.c_str());
    view = "from C-String";
    ASSERT_STREQ(view.c_str(), "from C-String");
    const char arr[] = "from C-Array";
    view = arr;
    ASSERT_STREQ(view.c_str(), "from C-Array");
}

TEST(StringViewTest, rValueUsage)
{
    auto func = [](CppSQLite3StringView view) { return std::string(view); };
    ASSERT_EQ(func("test"), "test");
    ASSERT_EQ(func(std::string("test")), "test");
}

TEST(StringViewTest, comparisons)
{
    ASSERT_EQ(CppSQLite3StringView("test"), CppSQLite3StringView("test"));
    ASSERT_NE(CppSQLite3StringView("test"), CppSQLite3StringView("tesst"));
    ASSERT_TRUE(CppSQLite3StringView("abc") < CppSQLite3StringView("abd"));
    ASSERT_FALSE(CppSQLite3StringView("abc") < CppSQLite3StringView("abc"));
    ASSERT_FALSE(CppSQLite3StringView("abd") < CppSQLite3StringView("abc"));
}


static_assert(std::is_move_constructible<CppSQLite3Query>::value, "move constructible");
static_assert(std::is_move_assignable<CppSQLite3Query>::value, "move assignable");
static_assert(!std::is_copy_constructible<CppSQLite3Query>::value, "not copy constructible");
static_assert(!std::is_copy_assignable<CppSQLite3Query>::value, "not copy assignable");

static_assert(std::is_move_constructible<CppSQLite3Statement>::value, "move constructible");
static_assert(std::is_move_assignable<CppSQLite3Statement>::value, "move assignable");
static_assert(!std::is_copy_constructible<CppSQLite3Statement>::value, "not copy constructible");
static_assert(!std::is_copy_assignable<CppSQLite3Statement>::value, "not copy assignable");
