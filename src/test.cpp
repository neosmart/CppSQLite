#include "CppSQLite3.h"
#include <iostream>
#include <cassert>

int main() {
    try {
        CppSQLite3DB db;

        // In-memory database
        db.open(":memory:");

        db.execDML("CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT);");
        assert(db.tableExists("test"));

        db.execDML("INSERT INTO test(name) VALUES ('Alice');");
        db.execDML("INSERT INTO test(name) VALUES ('Bob');");

        // Query it back using both Query and Table
        {
            auto q = db.execQuery("SELECT * FROM test ORDER BY id;");
            int cnt = 0;
            while (!q.eof()) {
                int id = q.getIntField("id");
                const char* name = q.getStringField("name");
                std::cout << "Row " << id << ": " << name << std::endl;
                q.nextRow();
                ++cnt;
            }
            assert(cnt == 2);
        }

        {
            auto t = db.getTable("SELECT id, name FROM test;");
            assert(t.numRows() == 2);
            t.setRow(0);
            assert(std::string(t.getStringField(1)) == "Alice");
            t.setRow(1);
            assert(std::string(t.getStringField(1)) == "Bob");
        }

        db.close();
        std::cout << "CppSQLite basic tests passed!" << std::endl;
        return 0;
    } catch(const CppSQLite3Exception& e) {
        std::cerr << "CppSQLite3Exception: " << e.errorMessage() << std::endl;
        return 1;
    }
}
