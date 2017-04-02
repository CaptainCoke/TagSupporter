#ifndef EMBEDDEDSQLCONNECTION_H
#define EMBEDDEDSQLCONNECTION_H

#include <QObject>
#include <vector>

class EmbeddedSQLConnection : public QObject
{
    Q_OBJECT
public:
    EmbeddedSQLConnection( QObject *pclParent = nullptr );
    ~EmbeddedSQLConnection() override;
    
    void openServer( const QString& strBasedir );
    void closeServer();
    
    void connectToDB( const QString& strDatabase = "amarok" );
    void disconnectFromDB();
    bool isConnected() const;
    
    /// return value contains one row of the result per vector entry, which in turn contains a vector of fields
    std::vector<std::vector<QString>> query( const QString& strQuery );
    
signals:
    void connected();
    void error( QString );

protected:
    void throwMysqlError( QString strText ) const;
    template<class ResultT, class ParseFunT>
    ResultT queryAndParseResults( const QString& strQuery, ParseFunT parseFun );
    
    struct MySQLConn;
    MySQLConn* m_pclClient{nullptr};
    bool       m_bServer{false};
};

#endif // EMBEDDEDSQLCONNECTION_H
