#include "CppSQLite3.h"

#include <gtest/gtest.h>

TEST(ExecQueryTest, throwsOnSyntaxError) {
    CppSQLite3DB db;
    db.open(":memory:");
    db.execQuery( "CREATE TABLE `myTable` (`ID` INT NOT NULL UNIQUE,`INFO` TEXT);" );

    bool exceptionThrown = false;
    try {
        db.execQuery("SELCT * FROM myTable");
    }
    catch(const CppSQLite3Exception& e) {
        exceptionThrown = true;
        EXPECT_EQ(1, e.errorCode());
        EXPECT_STREQ("SQLITE_ERROR[1]: near \"SELCT\": syntax error", e.errorMessage());
    }
    ASSERT_TRUE(exceptionThrown);

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
