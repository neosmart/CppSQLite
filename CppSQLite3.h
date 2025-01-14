/*
 * CppSQLite
 * Developed by Rob Groves <rob.groves@btinternet.com>
 * Maintained by NeoSmart Technologies <http://neosmart.net/>
 * See LICENSE file for copyright and license info
 */

#ifndef CppSQLite3_H
#define CppSQLite3_H

#include <cstdio>
#include <cstring>
#include <sqlite3.h>

#include <stdexcept>

#define CPPSQLITE_ERROR 1000

/**
 * @brief CppSQLite3StringView defines a very basic string view and can be constructed from C-style strings and
 * std::strings.
 * Unlike std::string_view, It doesn't store a size and input strings need to be null-terminated and UTF-8 encoded.
 * Unlike std::string_view, it can safely be created from a nullptr.
 */
class CppSQLite3StringView
{
public:
    CppSQLite3StringView(const std::string& str) : str_(str.c_str()) // implicitly converts from std::string
    {
    }

    CppSQLite3StringView(const char* str) : str_(str) // implicitly converts from C-style strings
    {
    }

    constexpr const char* c_str() const
    {
        return str_;
    }

    operator std::string_view() const
    {
        return str_ == nullptr ? std::string_view() : std::string_view(str_);
    }

private:
    const char* str_ = nullptr;
};


inline bool operator==(CppSQLite3StringView lhs, CppSQLite3StringView rhs) noexcept
{
    return std::string_view(lhs.c_str()) == std::string_view(rhs.c_str());
}

inline bool operator!=(CppSQLite3StringView lhs, CppSQLite3StringView rhs) noexcept
{
    return std::string_view(lhs.c_str()) != std::string_view(rhs.c_str());
}

inline bool operator<(CppSQLite3StringView lhs, CppSQLite3StringView rhs) noexcept
{
    return std::string_view(lhs.c_str()) < std::string_view(rhs.c_str());
}

struct CppSQLite3LogLevel
{
    enum Level
    {
        VERBOSE,
        INFO,
        WARNING,
        ERROR
    } code;
    std::string_view name;

    explicit CppSQLite3LogLevel(Level level);
};

using CppSQLite3ErrorHandler = void (*)(int /*sqlite3_error_code*/, std::string_view /* message */,
                                        std::string_view /* context */);
using CppSQLite3LogHandler = void (*)(CppSQLite3LogLevel /*level*/, std::string_view /*message */);


class CppSQLite3Exception : public std::runtime_error
{
public:
    CppSQLite3Exception(const int nErrCode, const std::string& errorMessage);

    const int errorCode() const
    {
        return mnErrCode;
    }

    static std::string_view errorCodeAsString(int nErrCode);

private:
    int mnErrCode;
};

struct CppSQLite3Config
{
    CppSQLite3Config();
    sqlite3* db;
    CppSQLite3ErrorHandler errorHandler;
    CppSQLite3LogHandler logHandler;
    bool enableVerboseLogging = false;
    void log(CppSQLite3LogLevel::Level level, CppSQLite3StringView message);
};

class CppSQLite3Query
{
public:
    CppSQLite3Query();

    CppSQLite3Query(CppSQLite3Query&& rQuery);

    CppSQLite3Query(const CppSQLite3Config& config, sqlite3_stmt* pVM, bool bEof, bool bOwnVM = true);

    CppSQLite3Query& operator=(CppSQLite3Query&& rQuery);

    virtual ~CppSQLite3Query();

    int numFields() const;

    int fieldIndex(CppSQLite3StringView field) const;
    const char* fieldName(int nCol) const;

    const char* fieldDeclType(int nCol) const;
    int fieldDataType(int nCol) const;

    const char* fieldValue(int nField) const;
    const char* fieldValue(CppSQLite3StringView field) const;

    int getIntField(int nField, int nNullValue = 0) const;
    int getIntField(CppSQLite3StringView field, int nNullValue = 0) const;

    long long getInt64Field(int nField, long long nNullValue = 0) const;
    long long getInt64Field(CppSQLite3StringView field, long long nNullValue = 0) const;

    double getFloatField(int nField, double fNullValue = 0.0) const;
    double getFloatField(CppSQLite3StringView field, double fNullValue = 0.0) const;

    const char* getStringField(int nField, const char* szNullValue = "") const;
    const char* getStringField(CppSQLite3StringView field, const char* szNullValue = "") const;

    const unsigned char* getBlobField(int nField, int& nLen) const;
    const unsigned char* getBlobField(CppSQLite3StringView field, int& nLen) const;

    bool fieldIsNull(int nField) const;
    bool fieldIsNull(CppSQLite3StringView field) const;

    bool eof() const;

    void nextRow();

    void finalize();

private:
    void checkVM() const;

    CppSQLite3Config mConfig;
    sqlite3_stmt* mpVM;
    bool mbEof;
    int mnCols;
    bool mbOwnVM;
};

class CppSQLite3Statement
{
public:
    CppSQLite3Statement();

    CppSQLite3Statement(CppSQLite3Statement&& rStatement);

    CppSQLite3Statement(const CppSQLite3Config& config, sqlite3_stmt* pVM);

    virtual ~CppSQLite3Statement();

    CppSQLite3Statement& operator=(CppSQLite3Statement&& rStatement);

    int execDML();

    CppSQLite3Query execQuery();

    void bind(int nParam, CppSQLite3StringView value);
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
    void checkReturnCode(int returnCode, const char* context);

    CppSQLite3Config mConfig;
    sqlite3_stmt* mpVM;
};


class CppSQLite3DB
{
public:
    CppSQLite3DB();

    CppSQLite3DB(const CppSQLite3DB& db) = delete;
    CppSQLite3DB& operator=(const CppSQLite3DB& db) = delete;

    virtual ~CppSQLite3DB();

    /**
     * @brief open opens a database with the given filename
     * @param szFile the filename of the database
     * @param flags the SQLITE_OPEN_* flags that are passed on to the sqlite3_open_v2 call
     */
    void open(CppSQLite3StringView fileName, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    void close();

    /**
     * @brief enableVerboseLogging enables logging of all SQL statements & queries
     */
    void enableVerboseLogging(bool enable);

    bool isOpened() const;

    bool tableExists(CppSQLite3StringView table);

    int execDML(CppSQLite3StringView szSQL);

    CppSQLite3Query execQuery(CppSQLite3StringView szSQL);

    int execScalar(CppSQLite3StringView szSQL);

    CppSQLite3Statement compileStatement(const char* szSQL);

    sqlite_int64 lastRowId() const;

    void interrupt()
    {
        sqlite3_interrupt(mConfig.db);
    }

    void setBusyTimeout(int nMillisecs);

    void setErrorHandler(CppSQLite3ErrorHandler h);

    void setLogHandler(CppSQLite3LogHandler h);

    static const char* SQLiteVersion()
    {
        return SQLITE_VERSION;
    }

    /**
     * @brief performCheckpoint wraps sqlite3_wal_checkpoint_v2
     * @param dbName name of the attached database (or empty)
     * @param mode SQLITE_CHECKPOINT_* value
     */
    void performCheckpoint(CppSQLite3StringView dbName = "", int mode = SQLITE_CHECKPOINT_PASSIVE);

private:
    sqlite3_stmt* compile(CppSQLite3StringView szSQL);

    void checkDB() const;
    CppSQLite3Config mConfig;
    int mnBusyTimeoutMs;
};

#endif
