#include "CppSQLite3.h"
#include "testhelper.h"
#include <gtest/gtest.h>

namespace {


class MockAllocator {
public:
bool provideMemory = true;
std::map<void*, size_t> allocations;

};

MockAllocator mockAllocator;

int mockInit(void *pAppData){ return SQLITE_OK; }
void mockShutdown(void *pAppData){
    mockAllocator.allocations.clear();
}
void *mockMalloc(int n) {
    if(!mockAllocator.provideMemory) {
        return nullptr;
    }
    auto p = malloc(n);
    mockAllocator.allocations[p] = n;
    return p;
}
void mockFree(void *p) {
    mockAllocator.allocations.erase(p);
    free(p);
}

void *mockRealloc(void *p, int n) {
    if(!mockAllocator.provideMemory) {
        return nullptr;
    }
    mockAllocator.allocations.erase(p);
    auto newLocation= realloc(p, n);
    mockAllocator.allocations[newLocation] = n;
    return newLocation;
}

int mockMemSize(void *p)             { return mockAllocator.allocations.at(p);}
int mockMemRoundup(int n)            {return n;}


/**
 * @brief The OutOfMemoryTest tests the CppSQLite3 wrapper by installing the MockAllocator that can
 * be asked any time during the test to not provide memory any longer and therefore run into code paths
 * that would be otherwise hard to cover in an automated tests.
 *
 * These paths might also be executed in other error conditions for which it is difficult to automate
 * tests, e.g. SQLITE_IOERR (Code 10).
 */
class OutOfMemoryTest : public ::testing::Test {
public:
    static void SetUpTestSuite() {
        sqlite3_mem_methods mem {
            mockMalloc,
                    mockFree,
                    mockRealloc,
                    mockMemSize,
                    mockMemRoundup,
                    mockInit,
                    mockShutdown,
                    0
        };
        sqlite3_config(SQLITE_CONFIG_MALLOC, mem);
    }

    void TearDown() {
        mockAllocator.provideMemory = true;
    }
};

class CustomError : public std::exception {};

}


TEST_F(OutOfMemoryTest, BufferThrowsMemoryError) {
    CppSQLite3Buffer buffer;
    std::string someString(20, '*');
    mockAllocator.provideMemory = false;
    ASSERT_THROW(buffer.format(someString.c_str()), SQLite3MemoryException);
}

TEST_F(OutOfMemoryTest, dbTableExistsThrowsMemoryError) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    mockAllocator.provideMemory = false;
    ASSERT_THROW(db.tableExists("xyz"), SQLite3MemoryException);
}

TEST_F(OutOfMemoryTest, QueryObjectCallsCustomErrorHandler) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    db.execQuery("INSERT INTO `myTable` VALUES(\"some text\")");
    auto insertLongValue = "INSERT INTO `myTable` VALUES(\"" + std::string(5000, '*') + "\")";
    db.execQuery(insertLongValue.c_str());
    auto query = db.execQuery("SELECT * FROM `myTable`");
    mockAllocator.provideMemory = false;
    EXPECT_THROW_WITH_MSG(query.nextRow(), CustomExceptions::SQLiteError, "out of memory when getting next row (Code 7)");
}

TEST_F(OutOfMemoryTest, QueryObjectFromCompiledStatementCallsCustomErrorHandler) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    db.execQuery("INSERT INTO `myTable` VALUES(\"some text\")");
    auto insertLongValue = "INSERT INTO `myTable` VALUES(\"" + std::string(2000, '*') + "\")";
    db.execQuery(insertLongValue.c_str());
    auto statement = db.compileStatement("SELECT * FROM `myTable`");
    auto query = statement.execQuery();
    mockAllocator.provideMemory = false;
    EXPECT_THROW_WITH_MSG(query.nextRow(), CustomExceptions::SQLiteError, "out of memory when getting next row (Code 7)");
}

TEST_F(OutOfMemoryTest, execDMLFromDBCallsCustomErrorHandler ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    std::string longString(2000, '*');
    std::string stmt = "insert into `myTable` values ('" + longString + "');";
    mockAllocator.provideMemory = false;

    EXPECT_THROW_WITH_MSG(db.execDML(stmt.c_str()), CustomExceptions::SQLiteError, "out of memory when executing DML query (Code 7)");
}

TEST_F(OutOfMemoryTest, execDMLFromCompiledstatementCallsCustomErrorHandler ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    CppSQLite3Statement stmt = db.compileStatement("insert into `myTable` values (?);");
    std::string longString(2000, '*');
    stmt.bind(1, longString.c_str());
    mockAllocator.provideMemory = false;
    EXPECT_THROW_WITH_MSG(stmt.execDML(), CustomExceptions::SQLiteError, "out of memory when executing DML statement (Code 7)");
}

TEST_F(OutOfMemoryTest, execQueryFromCompiledstatementCallsCustomErrorHandler ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execDML("CREATE TABLE `myTable` (`INFO` TEXT);");
    auto insertLongValue = "INSERT INTO `myTable` VALUES(\"" + std::string(2000, '*') + "\")";
    db.execDML(insertLongValue.c_str());
    CppSQLite3Statement stmt = db.compileStatement("SELECT * FROM `myTable`;");
    mockAllocator.provideMemory = false;
    EXPECT_THROW_WITH_MSG(stmt.execQuery(), CustomExceptions::SQLiteError, "out of memory when evaluating query (Code 7)");
}

TEST_F(OutOfMemoryTest, bindCallsCustomErrorHandler ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`INFO` TEXT);");
    CppSQLite3Statement stmt = db.compileStatement("insert into `myTable` values (?);");
    mockAllocator.provideMemory = false;
    std::string longString(2000, '*');
    EXPECT_THROW_WITH_MSG(stmt.bind(1, longString.c_str()), CustomExceptions::SQLiteError, "out of memory when binding string param (Code 7)");
}

TEST_F(OutOfMemoryTest, errorCodeWhenCompilationFails ) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.setErrorHandler(CustomExceptions::throwException);
    db.execQuery("CREATE TABLE `myTable` (`someId` INT, `INFO` TEXT);");
    db.execQuery("CREATE TABLE `otherTable` (`someId` INT, `INFO` TEXT);");
    db.execQuery("insert into `myTable` values (1, 'abcde\');");
    db.execQuery("insert into `otherTable` values (1, 'fghij\');");
    mockAllocator.provideMemory = false;
    EXPECT_THROW_WITH_MSG(db.execQuery("SELECT * FROM `myTable` JOIN `otherTable` ON myTable.someId == otherTable.someId"),
                          CustomExceptions::SQLiteError, "out of memory when compiling statement (Code 7)" );
}
