#include "EmbeddedSQLConnection.h"
#include <mysql/mysql.h>
#include <array>

#define CHECK_MYSQL( Fun, Description ) if ( int errornum = Fun; errornum != 0 ) throwMysqlError( errornum, "failed to " Description ": %1 (error %2)" );


struct EmbeddedSQLConnection::MySQLConn : public MYSQL {};
    
EmbeddedSQLConnection::EmbeddedSQLConnection( QObject *pclParent )
: QObject(pclParent)
{}

EmbeddedSQLConnection::~EmbeddedSQLConnection()
{
    closeServer();
}
    
void EmbeddedSQLConnection::openServer( const QString& strBasedir )
{
    //close old server
    closeServer();

    // prepare the "command-line" arguments struct for server initialization.
    // According to doc, the data is copied, so it should be destroyed at the end of the scope
    QByteArray arr_datadir        = QString("--datadir=%1/mysqle").arg(strBasedir).toLocal8Bit();
#ifdef WIN32
    QByteArray arr_defaults_file  = QString("--defaults-file=%1/my.ini").arg(strBasedir).toLocal8Bit();
    QByteArray arr_socket         = "--shared-memory=1";
#else
    QByteArray arr_defaults_file  = QString("--defaults-file=%1/my.cnf").arg(strBasedir).toLocal8Bit();
    QByteArray arr_socket         = QString("--socket=%1/sock").arg(strBasedir).toLocal8Bit();
#endif
    std::array<std::string,3> arr_other = {"--default-storage-engine=MyISAM","--skip-grant-tables","--skip-innodb"};
    std::array<char*,7> arr_server_options;
    arr_server_options[0] = nullptr; //acording to doc, the first item is ignored (as it would usually be the program's name...
    arr_server_options[1] = arr_defaults_file.data();
    arr_server_options[2] = arr_datadir.data();
    arr_server_options[3] = arr_socket.data();
    arr_server_options[4] = const_cast<char*>(arr_other[0].c_str());
    arr_server_options[5] = const_cast<char*>(arr_other[1].c_str());
    arr_server_options[6] = const_cast<char*>(arr_other[2].c_str());
    if ( mysql_library_init( static_cast<int>(arr_server_options.size()), arr_server_options.data(), nullptr ) != 0 )
        throw std::runtime_error( "failed to init embedded mysql server" );
    
    m_bServer = true;
}

void EmbeddedSQLConnection::closeServer()
{
    // disconnect any existing connection before closing the DB
    disconnectFromDB();
    if ( m_bServer )
    {
        mysql_library_end();
        m_bServer = false;
    }
}
   

void EmbeddedSQLConnection::connectToEmbeddedDB( const QString& strDatabase )
{
    if ( !m_bServer )
        throw std::runtime_error("mysql server not running");
    
    //disconnect previous connection first
    disconnectFromDB();
    
    try
    {
        m_pclClient = reinterpret_cast<MySQLConn*>( mysql_init(nullptr) );
        if ( !m_pclClient )
            throw std::runtime_error( "failed to init mysql client" );
    
        CHECK_MYSQL( mysql_options(m_pclClient, MYSQL_OPT_USE_EMBEDDED_CONNECTION, nullptr), "set option for using embedded MySQL" );
        MYSQL* mysql = mysql_real_connect(m_pclClient, nullptr, nullptr, nullptr, qPrintable(strDatabase), 0, nullptr,0);
        if ( mysql == nullptr || mysql != m_pclClient )
            throwMysqlError( "failed to connect to database: %1" );
        emit connected();
    }
    catch ( ... )
    {
        disconnectFromDB();
        throw;
    }
}

void EmbeddedSQLConnection::connectToExternalDB(const QString & strUser,const QString & strPassword,const QString & strDatabase)
{
    //disconnect previous connection first
    disconnectFromDB();

    try
    {
        m_pclClient = reinterpret_cast<MySQLConn*>( mysql_init(nullptr) );
        if ( !m_pclClient )
            throw std::runtime_error( "failed to init mysql client" );

        MYSQL* mysql = mysql_real_connect(m_pclClient, nullptr, qPrintable(strUser), qPrintable(strPassword), qPrintable(strDatabase), 0, nullptr,0);
        if ( mysql == nullptr || mysql != m_pclClient )
            throwMysqlError( "failed to connect to database: %1" );
        emit connected();
    }
    catch ( ... )
    {
        disconnectFromDB();
        throw;
    }
}

template<class ResultT, class ParseFunT>
ResultT EmbeddedSQLConnection::queryAndParseResults( const QString& strQuery, ParseFunT parseFun )
{
    ResultT vec_results;
    if ( !isConnected() )
        throw std::runtime_error("not connected to a database");
    
    CHECK_MYSQL( mysql_real_query(m_pclClient, strQuery.toLatin1().data(), strQuery.length() ), "query database" )
 
    MYSQL_RES* pcl_results = mysql_store_result( m_pclClient );
    if ( !pcl_results ) // query might have returned an empty result set
    {
        CHECK_MYSQL( mysql_errno(m_pclClient), "store query result" );
        return vec_results;
    }
    
    try
    {
        vec_results = parseFun( pcl_results );
        mysql_free_result( pcl_results );
        return vec_results;
    }
    catch ( ... )
    {
        mysql_free_result( pcl_results );
        throw;
    }
}

/// return value contains one row of the result per vector entry, which in turn contains a vector of fields
std::vector<std::vector<QString>> EmbeddedSQLConnection::query( const QString& strQuery )
{
    try
    {
        return queryAndParseResults<std::vector<std::vector<QString>>>( strQuery, []( MYSQL_RES* pcl_results ) {
            std::vector<std::vector<QString>> vec_results;
            size_t ui_num_rows = mysql_num_rows(pcl_results);
            size_t ui_num_cols = mysql_num_fields(pcl_results);
            vec_results.reserve( ui_num_rows );

            MYSQL_ROW cl_record;
            while ( (cl_record = mysql_fetch_row(pcl_results)) )
            {
                std::vector<QString> vec_row( ui_num_cols );
                for ( size_t i = 0; i < ui_num_cols; ++i )
                {
                    if ( cl_record[i] != nullptr ) //nullptr indicates a NULL result value. In that case, leave the QString to be null()
                        vec_row[i] = QString::fromLatin1( cl_record[i] );
                }
                vec_results.emplace_back( std::move(vec_row) );
            }
            return vec_results;
        } );
    }
    catch( const std::exception& rclExc )
    {
        emit error(qPrintable(QString("unknown error during execution of query\n\n%1\n\n:%2").arg(strQuery,QString(rclExc.what()))));
    }
    catch(...)
    {
        emit error(qPrintable(QString("unknown error during execution of query\n\n%1").arg(strQuery)));
    }
    return {};
}

void EmbeddedSQLConnection::disconnectFromDB()
{
    if ( m_pclClient )
    {
        mysql_close( m_pclClient );
        m_pclClient = nullptr;
    }
}

bool EmbeddedSQLConnection::isConnected() const
{
    return m_pclClient != nullptr;
}

void EmbeddedSQLConnection::throwMysqlError( QString strText ) const
{
    QString str_error( mysql_error(m_pclClient) );
    str_error.replace("%","%%");
    throw std::runtime_error( qPrintable( strText.arg( str_error ) ) );
}

void EmbeddedSQLConnection::throwMysqlError( int iError, QString strText ) const
{
    QString str_error( mysql_error(m_pclClient) );
    str_error.replace("%","%%");
    throw std::runtime_error( qPrintable( strText.arg( str_error ).arg(iError) ) );
}
