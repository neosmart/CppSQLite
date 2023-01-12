#include "CppSQLite3.h"
#include "testhelper.h"

#include <type_traits>

#include <gtest/gtest.h>

TEST(ExecQueryTest, throwsOnSyntaxError) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery( "CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);" );
    EXPECT_THROW_WITH_MSG( db.execQuery("SELCT * FROM myTable"), CppSQLite3InvalidQuery, "near \"SELCT\": syntax error" );
}


TEST(ExecQueryTest, throwsWhenInsertStatementMissesFields) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);");
    EXPECT_THROW_WITH_MSG( db.execQuery("INSERT INTO `myTable` VALUES(\"some text\")"),
                           CustomExceptions::InvalidQuery,
                           "table myTable has 2 columns but 1 values were supplied when compiling statement");
}

TEST( ExecQueryTest, selectOneRow) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery( "CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);" );
    db.execQuery("INSERT INTO myTable VALUES(42, \"some text\")");
    auto result = db.execQuery("SELECT * FROM myTable");
    EXPECT_EQ(42, result.getIntField("ID"));
    EXPECT_STREQ("some text", result.getStringField("INFO"));
    result.nextRow();
    EXPECT_TRUE(result.eof());
}

TEST(ErrorHandlerTest, dbThrowsOnInvalidOpenPath ) {
    CppSQLite3DB db;
    EXPECT_THROW_WITH_MSG(db.open("nonExistentFolder/test.sql"), CppSQLite3Exception,
                          "SQLITE_CANTOPEN[14]: unable to open database file");
}

TEST(ErrorHandlerTest, dbExecQueryThrowsCustomExceptionOnSyntaxError) {
    CppSQLite3DB db;

    db.setErrorHandler(CustomExceptions::throwException);

    db.open(":memory:");
    db.execQuery( "CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);" );

    EXPECT_THROW_WITH_MSG( db.execQuery("SELCT * FROM myTable"),
                           CustomExceptions::InvalidQuery,
                           "near \"SELCT\": syntax error when compiling statement" );
}

TEST( ErrorHandlerTest, dbExecDMLThrowsCustomExceptionOnSyntaxError) {
    CppSQLite3DB db;
    db.setErrorHandler(CustomExceptions::throwException);
    db.open(":memory:");
    EXPECT_THROW_WITH_MSG( db.execDML( "CRETE TABLE `myTable` (`ID` INT);"),
                           CustomExceptions::InvalidQuery,
                           "near \"CRETE\": syntax error when executing DML query" );
}

TEST(ErrorHandlerTest, getTableThrowsCustomExceptionOnSyntaxError ) {
    CppSQLite3DB db;
    db.setErrorHandler(CustomExceptions::throwException);
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    EXPECT_THROW_WITH_MSG(db.getTable("SELECT `something else` FROM `myTable`"), CustomExceptions::InvalidQuery, "no such column: something else when getting table");
}

TEST(ErrorHandlerTest, dbThrowsLogicErrorWhenNotOpened) {
    CppSQLite3DB db;
    db.setErrorHandler(CustomExceptions::throwException);
    EXPECT_THROW_WITH_MSG( db.execQuery( "CREATE TABLE `myTable` (`INFO` TEXT);" ), std::logic_error, "Database not open");
}

TEST(CppSQLite3TableTest, getFieldValue ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    db.execQuery("INSERT INTO myTable VALUES(\"some text\")");
    auto t = db.getTable("SELECT * FROM `myTable`");
    ASSERT_STREQ("some text", t.fieldValue("INFO"));
}

TEST(CppSQLite3TableTest, moveOperatorTransfersResultsHandle ) {
    CppSQLite3DB db;
    db.open(":memory:");
    auto table = db.getTable("CREATE TABLE `myTable` (`INFO` TEXT);");
    ASSERT_EQ(0, table.numRows());
    auto table2 = std::move(table);
    EXPECT_THROW_WITH_MSG(table.numRows(), std::logic_error, "Null Results pointer" );
    ASSERT_EQ(0, table2.numRows());
}

TEST(CppSQLite3TableTest, invalidFieldName ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    auto t = db.getTable("SELECT * FROM `myTable`");
    EXPECT_THROW_WITH_MSG( t.fieldValue("xyz"), std::invalid_argument,  "Invalid field name requested: 'xyz'" );
}

TEST(CppSQLite3QueryTest, moveOperatorTransfersVMHandle ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    CppSQLite3Query query = db.execQuery("SELECT * FROM myTable");
    ASSERT_TRUE(query.eof());
    auto query2 = std::move(query);
    EXPECT_THROW_WITH_MSG(query.eof(), std::logic_error, "Null Virtual Machine pointer" );
    ASSERT_TRUE(query2.eof());
}

TEST(CppSQLite3StatementTest, moveOperatorTransfersVMHandle ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    CppSQLite3Statement stmt = db.compileStatement("SELECT * FROM myTable");
    ASSERT_NO_THROW(stmt.execQuery());
    auto stmt2 = std::move(stmt);
    EXPECT_THROW_WITH_MSG(stmt.execQuery(), std::logic_error, "Null Virtual Machine pointer" );
    ASSERT_NO_THROW(stmt2.execQuery());
}

static_assert(std::is_move_constructible<CppSQLite3Query>::value, "move constructible");
static_assert(std::is_move_assignable<CppSQLite3Query>::value, "move assignable");
static_assert(!std::is_copy_constructible<CppSQLite3Query>::value, "not copy constructible");
static_assert(!std::is_copy_assignable<CppSQLite3Query>::value, "not copy assignable");

static_assert(std::is_move_constructible<CppSQLite3Statement>::value, "move constructible");
static_assert(std::is_move_assignable<CppSQLite3Statement>::value, "move assignable");
static_assert(!std::is_copy_constructible<CppSQLite3Statement>::value, "not copy constructible");
static_assert(!std::is_copy_assignable<CppSQLite3Statement>::value, "not copy assignable");

static_assert(std::is_move_constructible<CppSQLite3Table>::value, "move constructible");
static_assert(std::is_move_assignable<CppSQLite3Table>::value, "move assignable");
static_assert(!std::is_copy_constructible<CppSQLite3Table>::value, "not copy constructible");
static_assert(!std::is_copy_assignable<CppSQLite3Table>::value, "not copy assignable");


