#include "WebBrowserWidget.h"
#include <QNetworkAccessManager>
#include <QMessageBox>
#include <QWebEngineProfile>
#include "ui_WebBrowserWidget.h"
#include "CoverDownloader.h"

WebBrowserWidget::WebBrowserWidget(QWidget *pclParent)
: QWidget(pclParent)
, m_pclUI(new Ui::WebBrowserWidget)
, m_pclNetworkAccess( std::make_unique<QNetworkAccessManager>(nullptr) )
{
    m_pclCoverDownloader = std::make_unique<CoverDownloader>(m_pclNetworkAccess.get());
    m_pclUI->setupUi(this);
    
    connect( m_pclUI->parseWebViewButton, SIGNAL(clicked()), this, SLOT(parseWebViewURL()) );
    connect( m_pclCoverDownloader.get(), SIGNAL(imageReady()), this, SLOT(setCoverImageFromDownloader()), Qt::QueuedConnection );
    connect( m_pclCoverDownloader.get(), SIGNAL(error(QString)), this, SLOT(coverDownloadError(QString)), Qt::QueuedConnection );
    
    QWebEngineProfile* pcl_profile = QWebEngineProfile::defaultProfile();
    connect( pcl_profile, SIGNAL(downloadRequested(QWebEngineDownloadItem*)), this, SLOT(downloadImage(QWebEngineDownloadItem*)) );
    //customize actions
    m_pclUI->webView->pageAction( QWebEnginePage::DownloadImageToDisk )->setText( "set as Cover" );
}

WebBrowserWidget::~WebBrowserWidget() = default;

void WebBrowserWidget::coverDownloadError(QString strError)
{
    QMessageBox::critical( this, "Download Error", strError );
}

void WebBrowserWidget::setCoverImageFromDownloader()
{
    emit setCover(m_pclCoverDownloader->getImage());
}

void WebBrowserWidget::parseWebViewURL()
{
    emit parseURL( m_pclUI->webView->url() );
}

void WebBrowserWidget::showURL(QUrl clUrl)
{
    m_pclUI->webView->load(clUrl);
}

void WebBrowserWidget::downloadImage(QWebEngineDownloadItem* pclDownload)
{
    m_pclCoverDownloader->downloadImage( pclDownload->url() );
    pclDownload->cancel();
    pclDownload->deleteLater();
}
