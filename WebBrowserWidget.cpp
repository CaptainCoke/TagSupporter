#include "WebBrowserWidget.h"
#include <QNetworkAccessManager>
#include <QMessageBox>
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
    
    QWebPage* pcl_page = m_pclUI->webView->page();
    connect( pcl_page, SIGNAL(downloadRequested(const QNetworkRequest &)), m_pclCoverDownloader.get(), SLOT(downloadImage(const QNetworkRequest &)) );
    //customize actions
    m_pclUI->webView->pageAction( QWebPage::DownloadImageToDisk )->setText( "set as Cover" );
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

void WebBrowserWidget::showURL(const QUrl& rclUrl)
{
    m_pclUI->webView->load(rclUrl);
}
