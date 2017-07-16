#include "OnlineSourceParser.h"
#include <QNetworkReply>
#include <QThread>

OnlineSourceParser::OnlineSourceParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent)
: QObject( pclParent )
, m_pclNetworkAccess( pclNetworkAccess )
{
    connect( this, SIGNAL(sendQuery(const QNetworkRequest&,const char *)), this, SLOT(onSendQuery(const QNetworkRequest&,const char *)), Qt::QueuedConnection );
}

OnlineSourceParser::~OnlineSourceParser() = default;

void OnlineSourceParser::onSendQuery(const QNetworkRequest& rclRequest, const char *strReceivingSlot )
{
    QNetworkReply *pcl_reply = m_pclNetworkAccess->get( rclRequest );
    connect(this, SIGNAL(cancelAllPendingNetworkRequests()), pcl_reply, SLOT(abort()) );
    if ( !connect(pcl_reply, SIGNAL(finished()), this, strReceivingSlot ) )
        emit error( QString( "No suitable SLOT to handle redirect could be connected" ) );
}

class ParserWorkerThread : public QThread
{
public:
    ParserWorkerThread( QByteArray&& strReply, std::function<void(QByteArray)>&& funWork, QObject* pclParent )
    : QThread(pclParent)
    , m_funWork(std::move(funWork))
    , m_strReply(std::move(strReply))
    {}
    
    void run() override {
        m_funWork(std::move(m_strReply));
    }
    
protected:
    std::function<void(QByteArray)> m_funWork;
    QByteArray m_strReply;
};

void OnlineSourceParser::startParserThread( QByteArray&& strReply, std::function<void(QByteArray)>&& funWork )
{
    ParserWorkerThread *pcl_thread = new ParserWorkerThread( std::move(strReply), std::move(funWork), this );
    connect(pcl_thread, &ParserWorkerThread::finished, pcl_thread, &QObject::deleteLater);
    pcl_thread->start();
}
