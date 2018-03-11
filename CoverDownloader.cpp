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
    if ( !rclURL.scheme().isEmpty() && rclURL.isValid() )
    {
        QNetworkRequest cl_request(rclURL);
        cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
        downloadImage(cl_request);
    }
    else
        emit error( QString("invalid URL requested: \"%1\"").arg(rclURL.toDisplayString()) );
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
            emit error( QString( "failed to open downloaded image data from %1" ).arg(pcl_reply->url().toDisplayString()) );   
    }
    else
        emit error( QString("failed to download cover from \"%1\": %2" ).arg( pcl_reply->url().toDisplayString() ).arg( pcl_reply->errorString() ) );
    pcl_reply->deleteLater();
}
