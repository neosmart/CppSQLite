/*
 * CppSQLite
 * Developed by Rob Groves <rob.groves@btinternet.com>
 * Maintained by NeoSmart Technologies <http://neosmart.net/>
 * See LICENSE file for copyright and license info
 */

#include "CppSQLite3.h"
#include <cstdlib>
#include <fmt/core.h>
#include <string>
#include <utility>


#include <iostream>

namespace
{

////////////////////////////////////////////////////////////////////////////////

void defaultErrorHandler(int nErrorCode, std::string_view errorMessage, std::string_view /* context*/)
{
    std::string msg =
        fmt::format("{:s}[{:d}]: {:s}", CppSQLite3Exception::errorCodeAsString(nErrorCode), nErrorCode, errorMessage);

    throw CppSQLite3Exception(nErrorCode, msg);
}

void defaultLogHandler(CppSQLite3LogLevel level, std::string_view message)
{
    std::string_view clamped = message;
    // on verbose level we might get very long messages with queries that contain blob data or large strings
    // therefore perform some clamping:
    clamped = clamped.substr(0, 256);
    std::cout << fmt::format("[CppSQLite3][{}]: {}", level.name, clamped) << std::endl;
}

std::string_view levelToString(CppSQLite3LogLevel::Level level)
{
    using Level = CppSQLite3LogLevel::Level;
    switch (level)
    {
    case Level::error:
        return "Error";
    case Level::warning:
        return "Warning";
    case Level::info:
        return "Info";
    case Level::verbose:
        return "Verbose";
    }
    // unreachable
    std::terminate();
}

} // namespace


////////////////////////////////////////////////////////////////////////////////

CppSQLite3LogLevel::CppSQLite3LogLevel(Level logLevel) : code(logLevel), name(levelToString(logLevel))
{
}

////////////////////////////////////////////////////////////////////////////////


CppSQLite3Config::CppSQLite3Config() : db(nullptr), errorHandler(defaultErrorHandler), logHandler(defaultLogHandler)
{
}

void CppSQLite3Config::log(CppSQLite3LogLevel::Level level, CppSQLite3StringView message)
{
    if (enableVerboseLogging || level != CppSQLite3LogLevel::verbose)
    {
        logHandler(CppSQLite3LogLevel(level), message);
    }
}

////////////////////////////////////////////////////////////////////////////////


CppSQLite3Exception::CppSQLite3Exception(const int nErrCode, const std::string& errorMessage)
    : std::runtime_error(errorMessage), mnErrCode(nErrCode)
{
}


std::string_view CppSQLite3Exception::errorCodeAsString(int nErrCode)
{
    switch (nErrCode)
    {
    case SQLITE_OK:
        return "SQLITE_OK";
    case SQLITE_ERROR:
        return "SQLITE_ERROR";
    case SQLITE_INTERNAL:
        return "SQLITE_INTERNAL";
    case SQLITE_PERM:
        return "SQLITE_PERM";
    case SQLITE_ABORT:
        return "SQLITE_ABORT";
    case SQLITE_BUSY:
        return "SQLITE_BUSY";
    case SQLITE_LOCKED:
        return "SQLITE_LOCKED";
    case SQLITE_NOMEM:
        return "SQLITE_NOMEM";
    case SQLITE_READONLY:
        return "SQLITE_READONLY";
    case SQLITE_INTERRUPT:
        return "SQLITE_INTERRUPT";
    case SQLITE_IOERR:
        return "SQLITE_IOERR";
    case SQLITE_CORRUPT:
        return "SQLITE_CORRUPT";
    case SQLITE_NOTFOUND:
        return "SQLITE_NOTFOUND";
    case SQLITE_FULL:
        return "SQLITE_FULL";
    case SQLITE_CANTOPEN:
        return "SQLITE_CANTOPEN";
    case SQLITE_PROTOCOL:
        return "SQLITE_PROTOCOL";
    case SQLITE_EMPTY:
        return "SQLITE_EMPTY";
    case SQLITE_SCHEMA:
        return "SQLITE_SCHEMA";
    case SQLITE_TOOBIG:
        return "SQLITE_TOOBIG";
    case SQLITE_CONSTRAINT:
        return "SQLITE_CONSTRAINT";
    case SQLITE_MISMATCH:
        return "SQLITE_MISMATCH";
    case SQLITE_MISUSE:
        return "SQLITE_MISUSE";
    case SQLITE_NOLFS:
        return "SQLITE_NOLFS";
    case SQLITE_AUTH:
        return "SQLITE_AUTH";
    case SQLITE_FORMAT:
        return "SQLITE_FORMAT";
    case SQLITE_RANGE:
        return "SQLITE_RANGE";
    case SQLITE_ROW:
        return "SQLITE_ROW";
    case SQLITE_DONE:
        return "SQLITE_DONE";
    case CPPSQLITE_ERROR:
        return "CPPSQLITE_ERROR";
    default:
        return "UNKNOWN_ERROR";
    }
}

////////////////////////////////////////////////////////////////////////////////

CppSQLite3Query::CppSQLite3Query() : mConfig{}
{
    mpVM = 0;
    mbEof = true;
    mnCols = 0;
    mbOwnVM = false;
}


CppSQLite3Query::CppSQLite3Query(CppSQLite3Query&& rQuery)
{
    mConfig = rQuery.mConfig;
    mpVM = rQuery.mpVM;
    // Only one object can own the VM
    rQuery.mpVM = 0;
    mbEof = rQuery.mbEof;
    mnCols = rQuery.mnCols;
    mbOwnVM = rQuery.mbOwnVM;
}


CppSQLite3Query::CppSQLite3Query(const CppSQLite3Config& config, sqlite3_stmt* pVM, bool bEof, bool bOwnVM /*=true*/)
{
    mConfig = config;
    mpVM = pVM;
    mbEof = bEof;
    mnCols = sqlite3_column_count(mpVM);
    mbOwnVM = bOwnVM;
}


CppSQLite3Query::~CppSQLite3Query()
{
    try
    {
        finalize();
    }
    catch (const std::exception& e)
    {
        mConfig.log(CppSQLite3LogLevel::error, fmt::format("error during ~CppSQLite3Query: {}", e.what()));
    }

    catch (...)
    {
        mConfig.log(CppSQLite3LogLevel::error, fmt::format("unknown error during ~CppSQLite3Query"));
    }
}


CppSQLite3Query& CppSQLite3Query::operator=(CppSQLite3Query&& rQuery)
{
    try
    {
        finalize();
    }
    catch (const std::exception& e)
    {
        mConfig.log(CppSQLite3LogLevel::error,
                    fmt::format("failed to finalize CppSQLite3Query on reassign: {}", e.what()));
    }

    catch (...)
    {
        mConfig.log(CppSQLite3LogLevel::error,
                    fmt::format("unknown error when finalizing CppSQLite3Query on reassign"));
    }
    mpVM = rQuery.mpVM;
    rQuery.mpVM = 0;
    mbEof = rQuery.mbEof;
    mnCols = rQuery.mnCols;
    mbOwnVM = rQuery.mbOwnVM;
    mConfig = rQuery.mConfig;
    return *this;
}


int CppSQLite3Query::numFields() const
{
    checkVM();
    return mnCols;
}


const char* CppSQLite3Query::fieldValue(int nField) const
{
    checkVM();

    if (nField < 0 || nField > mnCols - 1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return (const char*)sqlite3_column_text(mpVM, nField);
}


const char* CppSQLite3Query::fieldValue(CppSQLite3StringView field) const
{
    int nField = fieldIndex(field);
    return (const char*)sqlite3_column_text(mpVM, nField);
}


int CppSQLite3Query::getIntField(int nField, int nNullValue /*=0*/) const
{
    if (fieldDataType(nField) == SQLITE_NULL)
    {
        return nNullValue;
    }
    else
    {
        return sqlite3_column_int(mpVM, nField);
    }
}


int CppSQLite3Query::getIntField(CppSQLite3StringView field, int nNullValue /*=0*/) const
{
    int nField = fieldIndex(field);
    return getIntField(nField, nNullValue);
}


long long CppSQLite3Query::getInt64Field(int nField, long long nNullValue /*=0*/) const
{
    if (fieldDataType(nField) == SQLITE_NULL)
    {
        return nNullValue;
    }
    else
    {
        return sqlite3_column_int64(mpVM, nField);
    }
}


long long CppSQLite3Query::getInt64Field(CppSQLite3StringView field, long long nNullValue /*=0*/) const
{
    int nField = fieldIndex(field);
    return getInt64Field(nField, nNullValue);
}


double CppSQLite3Query::getFloatField(int nField, double fNullValue /*=0.0*/) const
{
    if (fieldDataType(nField) == SQLITE_NULL)
    {
        return fNullValue;
    }
    else
    {
        return sqlite3_column_double(mpVM, nField);
    }
}


double CppSQLite3Query::getFloatField(CppSQLite3StringView field, double fNullValue /*=0.0*/) const
{
    int nField = fieldIndex(field);
    return getFloatField(nField, fNullValue);
}


const char* CppSQLite3Query::getStringField(int nField, const char* szNullValue /*=""*/) const
{
    if (fieldDataType(nField) == SQLITE_NULL)
    {
        return szNullValue;
    }
    else
    {
        return (const char*)sqlite3_column_text(mpVM, nField);
    }
}


const char* CppSQLite3Query::getStringField(CppSQLite3StringView field, const char* szNullValue /*=""*/) const
{
    int nField = fieldIndex(field);
    return getStringField(nField, szNullValue);
}


const unsigned char* CppSQLite3Query::getBlobField(int nField, int& nLen) const
{
    checkVM();

    if (nField < 0 || nField > mnCols - 1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    nLen = sqlite3_column_bytes(mpVM, nField);
    return (const unsigned char*)sqlite3_column_blob(mpVM, nField);
}


const unsigned char* CppSQLite3Query::getBlobField(CppSQLite3StringView field, int& nLen) const
{
    int nField = fieldIndex(field);
    return getBlobField(nField, nLen);
}


bool CppSQLite3Query::fieldIsNull(int nField) const
{
    return (fieldDataType(nField) == SQLITE_NULL);
}


bool CppSQLite3Query::fieldIsNull(CppSQLite3StringView field) const
{
    int nField = fieldIndex(field);
    return (fieldDataType(nField) == SQLITE_NULL);
}


int CppSQLite3Query::fieldIndex(CppSQLite3StringView field) const
{
    checkVM();

    if (field.c_str())
    {
        for (int nField = 0; nField < mnCols; nField++)
        {
            CppSQLite3StringView temp = sqlite3_column_name(mpVM, nField);

            if (field == temp)
            {
                return nField;
            }
        }
    }

    throw std::invalid_argument("Invalid field name requested");
}


const char* CppSQLite3Query::fieldName(int nCol) const
{
    checkVM();

    if (nCol < 0 || nCol > mnCols - 1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return sqlite3_column_name(mpVM, nCol);
}


const char* CppSQLite3Query::fieldDeclType(int nCol) const
{
    checkVM();

    if (nCol < 0 || nCol > mnCols - 1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return sqlite3_column_decltype(mpVM, nCol);
}


int CppSQLite3Query::fieldDataType(int nCol) const
{
    checkVM();

    if (nCol < 0 || nCol > mnCols - 1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return sqlite3_column_type(mpVM, nCol);
}


bool CppSQLite3Query::eof() const
{
    checkVM();
    return mbEof;
}


void CppSQLite3Query::nextRow()
{
    checkVM();

    int nRet = sqlite3_step(mpVM);

    if (nRet == SQLITE_DONE)
    {
        // no rows
        mbEof = true;
    }
    else if (nRet == SQLITE_ROW)
    {
        // more rows, nothing to do
    }
    else
    {
        if (mbOwnVM)
        {
            nRet = sqlite3_finalize(mpVM);
            mpVM = 0;
        }
        const char* szError = sqlite3_errmsg(mConfig.db);
        mConfig.errorHandler(nRet, szError, "when getting next row");
    }
}


void CppSQLite3Query::finalize()
{
    if (mpVM && mbOwnVM)
    {
        int nRet = sqlite3_finalize(mpVM);
        mpVM = 0;
        if (nRet != SQLITE_OK)
        {
            const char* szError = sqlite3_errmsg(mConfig.db);
            mConfig.errorHandler(nRet, szError, "during finalize");
        }
    }
}


void CppSQLite3Query::checkVM() const
{
    if (mpVM == 0)
    {
        throw std::logic_error("Null Virtual Machine pointer");
    }
}

////////////////////////////////////////////////////////////////////////////////

CppSQLite3Statement::CppSQLite3Statement() : mConfig{}
{
    mpVM = 0;
}


CppSQLite3Statement::CppSQLite3Statement(CppSQLite3Statement&& rStatement)
{
    mConfig = rStatement.mConfig;
    mpVM = rStatement.mpVM;
    // Only one object can own VM
    rStatement.mpVM = 0;
}


CppSQLite3Statement::CppSQLite3Statement(const CppSQLite3Config& config, sqlite3_stmt* pVM) : mConfig(config), mpVM(pVM)
{
}


CppSQLite3Statement::~CppSQLite3Statement()
{
    try
    {
        finalize();
    }
    catch (const std::exception& e)
    {
        mConfig.log(CppSQLite3LogLevel::error, fmt::format("error during ~CppSQLite3Statement: {}", e.what()));
    }

    catch (...)
    {
        mConfig.log(CppSQLite3LogLevel::error, fmt::format("unknown error during ~CppSQLite3Statement"));
    }
}


CppSQLite3Statement& CppSQLite3Statement::operator=(CppSQLite3Statement&& rStatement)
{
    try
    {
        finalize();
    }
    catch (const std::exception& e)
    {
        mConfig.log(CppSQLite3LogLevel::error,
                    fmt::format("error when finalizing CppSQLite3Statement during reassign: {}", e.what()));
    }

    catch (...)
    {
        mConfig.log(CppSQLite3LogLevel::error,
                    fmt::format("unknown error when finalizing ~CppSQLite3Statement during reassign"));
    }

    mConfig = rStatement.mConfig;
    mpVM = rStatement.mpVM;
    // Only one object can own VM
    rStatement.mpVM = 0;
    return *this;
}


int CppSQLite3Statement::execDML()
{
    checkDB();
    checkVM();

    const char* szError = 0;

    mConfig.log(CppSQLite3LogLevel::verbose, sqlite3_expanded_sql(mpVM));

    int nRet = sqlite3_step(mpVM);

    if (nRet == SQLITE_DONE)
    {
        int nRowsChanged = sqlite3_changes(mConfig.db);

        nRet = sqlite3_reset(mpVM);

        if (nRet != SQLITE_OK)
        {
            szError = sqlite3_errmsg(mConfig.db);
            mConfig.errorHandler(nRet, szError, "when getting number of rows changed");
        }

        return nRowsChanged;
    }
    else
    {
        nRet = sqlite3_reset(mpVM);
        szError = sqlite3_errmsg(mConfig.db);
        mConfig.errorHandler(nRet, szError, "when executing DML statement");
        return 0;
    }
}


CppSQLite3Query CppSQLite3Statement::execQuery()
{
    checkDB();
    checkVM();

    mConfig.log(CppSQLite3LogLevel::verbose, sqlite3_expanded_sql(mpVM));

    int nRet = sqlite3_step(mpVM);

    if (nRet == SQLITE_DONE)
    {
        // no rows
        return CppSQLite3Query(mConfig, mpVM, true /*eof*/, false);
    }
    else if (nRet == SQLITE_ROW)
    {
        // at least 1 row
        return CppSQLite3Query(mConfig, mpVM, false /*eof*/, false);
    }
    else
    {
        nRet = sqlite3_reset(mpVM);
        const char* szError = sqlite3_errmsg(mConfig.db);
        mConfig.errorHandler(nRet, szError, "when evaluating query");
        return CppSQLite3Query();
    }
}


void CppSQLite3Statement::bind(int nParam, CppSQLite3StringView value)
{
    checkVM();
    int nRes = sqlite3_bind_text(mpVM, nParam, value.c_str(), -1, SQLITE_TRANSIENT);
    checkReturnCode(nRes, "when binding string param");
}


void CppSQLite3Statement::bind(int nParam, const int nValue)
{
    checkVM();
    int nRes = sqlite3_bind_int(mpVM, nParam, nValue);
    checkReturnCode(nRes, "when binding int param");
}


void CppSQLite3Statement::bind(int nParam, const long long nValue)
{
    checkVM();
    int nRes = sqlite3_bind_int64(mpVM, nParam, nValue);
    checkReturnCode(nRes, "when binding int64 param");
}


void CppSQLite3Statement::bind(int nParam, const double dValue)
{
    checkVM();
    int nRes = sqlite3_bind_double(mpVM, nParam, dValue);
    checkReturnCode(nRes, "when binding double param");
}


void CppSQLite3Statement::bind(int nParam, const unsigned char* blobValue, int nLen)
{
    checkVM();
    int nRes = sqlite3_bind_blob(mpVM, nParam, (const void*)blobValue, nLen, SQLITE_TRANSIENT);

    checkReturnCode(nRes, "when binding blob param");
}


void CppSQLite3Statement::bindNull(int nParam)
{
    checkVM();
    int nRes = sqlite3_bind_null(mpVM, nParam);
    checkReturnCode(nRes, "when binding NULL param");
}


void CppSQLite3Statement::reset()
{
    if (mpVM)
    {
        int nRet = sqlite3_reset(mpVM);
        checkReturnCode(nRet, "when reseting statement");
    }
}


void CppSQLite3Statement::finalize()
{
    if (mpVM)
    {
        int nRet = sqlite3_finalize(mpVM);
        mpVM = 0;
        checkReturnCode(nRet, "when finalizing statement");
    }
}


void CppSQLite3Statement::checkDB() const
{
    if (mConfig.db == 0)
    {
        throw std::logic_error("Database not open");
    }
}


void CppSQLite3Statement::checkVM() const
{
    if (mpVM == 0)
    {
        throw std::logic_error("Null Virtual Machine pointer");
    }
}

void CppSQLite3Statement::checkReturnCode(int nRes, const char* context)
{
    if (nRes != SQLITE_OK)
    {
        const char* szError = sqlite3_errmsg(mConfig.db);
        mConfig.errorHandler(nRes, szError, context);
    }
}


////////////////////////////////////////////////////////////////////////////////

CppSQLite3DB::CppSQLite3DB() : mConfig{}, mnBusyTimeoutMs(60'000) // 60 seconds
{
}

CppSQLite3DB::~CppSQLite3DB()
{
    try
    {
        close();
    }
    catch (std::exception& e)
    {
        mConfig.log(CppSQLite3LogLevel::error, fmt::format("error during ~CppSQLite3DB: {}", e.what()));
    }
    catch (...)
    {
        mConfig.log(CppSQLite3LogLevel::error, fmt::format("unknown error during ~CppSQLite3DB"));
    }
}

void CppSQLite3DB::open(CppSQLite3StringView fileName, int flags)
{
    if (mConfig.db != nullptr)
    {
        throw std::logic_error("Previous db handle was not closed");
    }
    int nRet = sqlite3_open_v2(fileName.c_str(), &mConfig.db, flags, nullptr);

    if (nRet != SQLITE_OK)
    {
        const char* szError = sqlite3_errmsg(mConfig.db);
        auto msg = fmt::format("when opening {:s}", fileName.c_str());
        mConfig.errorHandler(nRet, szError, msg.c_str());
    }

    setBusyTimeout(mnBusyTimeoutMs);
}


void CppSQLite3DB::close()
{
    if (mConfig.db)
    {
        auto nRet = sqlite3_close(mConfig.db);
        if (nRet == SQLITE_OK)
        {
            mConfig.db = nullptr;
        }
        else
        {
            const char* szError = sqlite3_errmsg(mConfig.db);
            mConfig.errorHandler(nRet, szError, "when closing connection");
        }
    }
}

void CppSQLite3DB::enableVerboseLogging(bool enable)
{
    mConfig.enableVerboseLogging = enable;
}

bool CppSQLite3DB::isOpened() const
{
    return mConfig.db != nullptr;
}


CppSQLite3Statement CppSQLite3DB::compileStatement(CppSQLite3StringView szSQL)
{
    checkDB();

    sqlite3_stmt* pVM = compile(szSQL);
    return CppSQLite3Statement(mConfig, pVM);
}


bool CppSQLite3DB::tableExists(CppSQLite3StringView table)
{
    std::string buffer(256, '\0');
    // use sqlite3_snprintf function as it properly escapes the input string
    sqlite3_snprintf(static_cast<int>(buffer.size()), buffer.data(),
                     "select count(*) from sqlite_master where type='table' and name=%Q", table.c_str());
    int nRet = execScalar(buffer.c_str());
    return (nRet > 0);
}

int CppSQLite3DB::execDML(CppSQLite3StringView szSQL)
{
    checkDB();

    char* szError = 0;


    mConfig.log(CppSQLite3LogLevel::verbose, szSQL);

    int nRet = sqlite3_exec(mConfig.db, szSQL.c_str(), 0, 0, &szError);

    if (nRet == SQLITE_OK)
    {
        return sqlite3_changes(mConfig.db);
    }
    else
    {
        std::string error = "Unknown error";
        if (szError != nullptr)
        {
            error = szError;
            sqlite3_free((void*)szError);
        }
        else
        {
            error = sqlite3_errmsg(mConfig.db);
        }
        mConfig.errorHandler(nRet, error.c_str(), "when executing DML query");
        return nRet;
    }
}


CppSQLite3Query CppSQLite3DB::execQuery(CppSQLite3StringView szSQL)
{
    checkDB();

    sqlite3_stmt* pVM = compile(szSQL);

    mConfig.log(CppSQLite3LogLevel::verbose, szSQL);

    int nRet = sqlite3_step(pVM);

    if (nRet == SQLITE_DONE)
    {
        // no rows
        return CppSQLite3Query(mConfig, pVM, true /*eof*/);
    }
    else if (nRet == SQLITE_ROW)
    {
        // at least 1 row
        return CppSQLite3Query(mConfig, pVM, false /*eof*/);
    }
    else
    {
        nRet = sqlite3_finalize(pVM);
        const char* szError = sqlite3_errmsg(mConfig.db);
        mConfig.errorHandler(nRet, szError, "when evaluating query");
        return CppSQLite3Query();
    }
}


int CppSQLite3DB::execScalar(CppSQLite3StringView szSQL)
{
    CppSQLite3Query q = execQuery(szSQL);

    if (q.eof() || q.numFields() < 1)
    {
        throw std::invalid_argument("Invalid scalar query");
    }

    return atoi(q.fieldValue(0));
}

sqlite_int64 CppSQLite3DB::lastRowId() const
{
    return sqlite3_last_insert_rowid(mConfig.db);
}


void CppSQLite3DB::setBusyTimeout(int nMillisecs)
{
    mnBusyTimeoutMs = nMillisecs;
    sqlite3_busy_timeout(mConfig.db, mnBusyTimeoutMs);
}

void CppSQLite3DB::setErrorHandler(CppSQLite3ErrorHandler h)
{
    mConfig.errorHandler = h;
}

void CppSQLite3DB::setLogHandler(CppSQLite3LogHandler h)
{
    mConfig.logHandler = h;
}

void CppSQLite3DB::performCheckpoint(CppSQLite3StringView dbName, int mode)
{
    int nRet = sqlite3_wal_checkpoint_v2(mConfig.db, dbName.c_str(), mode, nullptr, nullptr);
    if (nRet != SQLITE_OK)
    {
        const char* szError = sqlite3_errmsg(mConfig.db);
        mConfig.errorHandler(nRet, szError, "when performing checkpoint");
    }
}


void CppSQLite3DB::checkDB() const
{
    if (!mConfig.db)
    {
        throw std::logic_error("Database not open");
    }
}


sqlite3_stmt* CppSQLite3DB::compile(CppSQLite3StringView szSQL)
{
    checkDB();

    const char* szTail = 0;
    sqlite3_stmt* pVM;

    int prepareFlags = 0;
    int nRet = sqlite3_prepare_v3(mConfig.db, szSQL.c_str(), -1, prepareFlags, &pVM, &szTail);
    const char* szError = sqlite3_errmsg(mConfig.db);

    if (nRet != SQLITE_OK)
    {
        mConfig.errorHandler(nRet, szError, "when compiling statement");
    }

    return pVM;
}
