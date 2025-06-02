/*
 * CppSQLite
 * Developed by Rob Groves <rob.groves@btinternet.com>
 * Maintained by NeoSmart Technologies <http://neosmart.net/>
 * See LICENSE file for copyright and license info
*/

#ifndef CppSQLite3_H
#define CppSQLite3_H

#include <sqlite3.h>
#include <cstdio>
#include <cstring>
#include <exception>

#define CPPSQLITE_ERROR 1000

namespace detail
{
    /**
     * RAII class for managing memory allocated by sqlite
    */
    class SQLite3Memory
    {
    public:

        // Default constructor
        SQLite3Memory();
        // Constructor that allocates memory of a given size
        SQLite3Memory(int nBufferLen);
        // Constructor that formats a string with sqlite memory allocation
        SQLite3Memory(const char* szFormat, va_list list);
        // Destructor
        ~SQLite3Memory();

        // Copy constructor
        SQLite3Memory(SQLite3Memory const& other);
        // Copy assignment
        SQLite3Memory& operator=(SQLite3Memory const& lhs);

        // Move constructor
        SQLite3Memory(SQLite3Memory&& other);
        // Move assignment
        SQLite3Memory& operator=(SQLite3Memory&& lhs);

        // Swap operation
        void swap(SQLite3Memory& other);

        int getLength() const { return mnBufferLen; }

        void* getBuffer() const { return mpBuf; }

        void clear();

    private:

        int mnBufferLen;
        void* mpBuf;
    };
}


class CppSQLite3Exception : public std::exception
{
public:

    CppSQLite3Exception(const int nErrCode,
                    const char* szErrMess,
                    bool bDeleteMsg=true);

    CppSQLite3Exception(const CppSQLite3Exception&  e);

    virtual ~CppSQLite3Exception();

    int errorCode() const { return mnErrCode; }

    const char* errorMessage() const { return mpszErrMess; }

    const char* what() const noexcept override { return mpszErrMess; }

    static const char* errorCodeAsString(int nErrCode);

private:

    int mnErrCode;
    char* mpszErrMess;
};


class CppSQLite3Buffer
{
public:
    const char* format(const char* szFormat, ...);

    operator const char*() { return static_cast<char const*>(mBuf.getBuffer()); }

    void clear();

private:

    detail::SQLite3Memory mBuf;
};


class CppSQLite3Binary
{
public:

    CppSQLite3Binary();

    ~CppSQLite3Binary();

    void setBinary(const unsigned char* pBuf, int nLen);
    void setEncoded(const unsigned char* pBuf);

    const unsigned char* getEncoded();
    const unsigned char* getBinary();

    int getBinaryLength();

    unsigned char* allocBuffer(int nLen);

    void clear();

private:

    unsigned char* mpBuf;
    int mnBinaryLen;
    int mnBufferLen;
    int mnEncodedLen;
    bool mbEncoded;
};


class CppSQLite3Query
{
public:

    CppSQLite3Query();

    CppSQLite3Query(const CppSQLite3Query& rQuery);

    CppSQLite3Query(sqlite3* pDB,
                sqlite3_stmt* pVM,
                bool bEof,
                bool bOwnVM=true);

    CppSQLite3Query& operator=(const CppSQLite3Query& rQuery);

    virtual ~CppSQLite3Query();

    int numFields() const;

    int fieldIndex(const char* szField) const;
    const char* fieldName(int nCol) const;

    const char* fieldDeclType(int nCol) const;
    int fieldDataType(int nCol) const;

    const char* fieldValue(int nField) const;
    const char* fieldValue(const char* szField) const;

    int getIntField(int nField, int nNullValue=0) const;
    int getIntField(const char* szField, int nNullValue=0) const;

    long long getInt64Field(int nField, long long nNullValue=0) const;
    long long getInt64Field(const char* szField, long long nNullValue=0) const;

    float getFloatField(int nField, float fNullValue=0.0f) const;
    float getFloatField(const char* szField, float fNullValue=0.0f) const;

    double getDoubleField(int nField, double dNullValue=0.0) const;
    double getDoubleField(const char* szField, double dNullValue=0.0) const;

    const char* getStringField(int nField, const char* szNullValue="") const;
    const char* getStringField(const char* szField, const char* szNullValue="") const;

    const unsigned char* getBlobField(int nField, int& nLen) const;
    const unsigned char* getBlobField(const char* szField, int& nLen) const;

    bool fieldIsNull(int nField) const;
    bool fieldIsNull(const char* szField) const;

    bool eof() const;

    void nextRow();

    void finalize();

private:

    void checkVM() const;

    sqlite3* mpDB;
    sqlite3_stmt* mpVM;
    bool mbEof;
    int mnCols;
    bool mbOwnVM;
};


class CppSQLite3Table
{
public:

    CppSQLite3Table();

    CppSQLite3Table(const CppSQLite3Table& rTable);

    CppSQLite3Table(char** paszResults, int nRows, int nCols);

    virtual ~CppSQLite3Table();

    CppSQLite3Table& operator=(const CppSQLite3Table& rTable);

    int numFields() const;

    int numRows() const;

    const char* fieldName(int nCol) const;

    const char* fieldValue(int nField) const;
    const char* fieldValue(const char* szField) const;

    int getIntField(int nField, int nNullValue=0) const;
    int getIntField(const char* szField, int nNullValue=0) const;

    float getFloatField(int nField, float fNullValue=0.0f) const;
    float getFloatField(const char* szField, float fNullValue=0.0f) const;

    double getDoubleField(int nField, double dNullValue=0.0) const;
    double getDoubleField(const char* szField, double dNullValue=0.0) const;

    const char* getStringField(int nField, const char* szNullValue="") const;
    const char* getStringField(const char* szField, const char* szNullValue="") const;

    bool fieldIsNull(int nField) const;
    bool fieldIsNull(const char* szField) const;

    void setRow(int nRow);

    void finalize();

private:

    void checkResults() const;

    int mnCols;
    int mnRows;
    int mnCurrentRow;
    char** mpaszResults;
};


class CppSQLite3Statement
{
public:

    CppSQLite3Statement();

    CppSQLite3Statement(const CppSQLite3Statement& rStatement);

    CppSQLite3Statement(sqlite3* pDB, sqlite3_stmt* pVM);

    virtual ~CppSQLite3Statement();

    CppSQLite3Statement& operator=(const CppSQLite3Statement& rStatement);

    int execDML();

    CppSQLite3Query execQuery();

    void bind(int nParam, const char* szValue);
    void bind(int nParam, const int nValue);
    void bind(int nParam, const long long nValue);
    void bind(int nParam, const double dwValue);
    void bind(int nParam, const unsigned char* blobValue, int nLen);
    void bindNull(int nParam);

    void reset();

    void finalize();

private:

    void checkDB() const;
    void checkVM() const;

    sqlite3* mpDB;
    sqlite3_stmt* mpVM;
};


class CppSQLite3DB
{
public:

    CppSQLite3DB();

    virtual ~CppSQLite3DB();

    void open(const char* szFile);

    void close();

    bool tableExists(const char* szTable);

    int execDML(const char* szSQL);

    CppSQLite3Query execQuery(const char* szSQL);

    int execScalar(const char* szSQL, int nNullSentinel = 0);

    CppSQLite3Table getTable(const char* szSQL);

    CppSQLite3Statement compileStatement(const char* szSQL);

    sqlite_int64 lastRowId() const;

    void interrupt() { sqlite3_interrupt(mpDB); }

    void setBusyTimeout(int nMillisecs);

    static const char* SQLiteVersion() { return SQLITE_VERSION; }

private:

    CppSQLite3DB(const CppSQLite3DB& db);
    CppSQLite3DB& operator=(const CppSQLite3DB& db);

    sqlite3_stmt* compile(const char* szSQL);

    void checkDB() const;

    sqlite3* mpDB;
    int mnBusyTimeoutMs;
};

#endif
