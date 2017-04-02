#include "CoverDownloader.h"
#include <QPixmap>
#include <QNetworkRequest>
#include <QNetworkReply>

CoverDownloader::CoverDownloader(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent)
: QObject(pclParent)
, m_pclNetworkAccess(pclNetworkAccess)
, m_pclPixmap( std::make_unique<QPixmap>() )
{
}

CoverDownloader::~CoverDownloader() = default;

void CoverDownloader::downloadImage(const QUrl &rclURL)
{
    QNetworkRequest cl_request(rclURL);
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    downloadImage(cl_request);
}

const QPixmap &CoverDownloader::getImage() const
{
    return *m_pclPixmap;
}

void CoverDownloader::clear()
{
    *m_pclPixmap = QPixmap();
}

void CoverDownloader::downloadImage(const QNetworkRequest &rclRequest)
{
    QNetworkReply *pcl_reply = m_pclNetworkAccess->get( rclRequest );
    connect(pcl_reply, SIGNAL(finished()), this, SLOT(fileDownloaded()));
}

void CoverDownloader::fileDownloaded()
{
    QNetworkReply* pcl_reply = dynamic_cast<QNetworkReply*>( sender() );
    if ( !pcl_reply )
        return;
    
    // check for error
    if ( pcl_reply->error() == QNetworkReply::NoError )
    {
        if ( m_pclPixmap->loadFromData( pcl_reply->readAll() ) )
            emit imageReady();
        else
            emit error( "failed to open downloaded image data" );   
    }
    else
        emit error( pcl_reply->errorString() );   
    pcl_reply->deleteLater();
}
