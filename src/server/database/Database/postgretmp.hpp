#include <stdarg.h>

#ifdef _WIN32
#define FD_SETSIZE 1024
#include <winsock2.h>
#include <postgre/libpq-fe.h>
#else
#include <postgresql/libpq-fe.h>
#endif

class PostgreSQLConnection : public SqlConnection
{
    public:
        PostgreSQLConnection(Database& db) : SqlConnection(db), mPGconn(nullptr) {}
        ~PostgreSQLConnection();

        bool Initialize(const char* infoString) override;

        QueryResult* Query(const char* sql) override;
        QueryNamedResult* QueryNamed(const char* sql) override;
        bool Execute(const char* sql) override;

        unsigned long escape_string(char* to, const char* from, unsigned long length);

        bool BeginTransaction() override;
        bool CommitTransaction() override;
        bool RollbackTransaction() override;

    private:
        bool _TransactionCmd(const char* sql);
        bool _Query(const char* sql, PGresult** pResult, uint64* pRowCount, uint32* pFieldCount);

        PGconn* mPGconn;
};

class DatabasePostgre : public Database
{
        friend class MaNGOS::OperatorNew<DatabasePostgre>;

    public:
        DatabasePostgre();
        ~DatabasePostgre();

        //! Initializes Postgres and connects to a server.
        /*! infoString should be formated like hostname;username;password;database. */

    protected:
        virtual SqlConnection* CreateConnection() override;

    private:
        static size_t db_count;
};