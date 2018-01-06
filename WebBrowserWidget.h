#ifndef WEBBROWSERWIDGET_H
#define WEBBROWSERWIDGET_H

#include <QWidget>
#include <memory>
#include <QUrl>

namespace Ui {
class WebBrowserWidget;
}

class WebBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WebBrowserWidget(QWidget *pclParent = nullptr);
    ~WebBrowserWidget() override;
    
public slots:
    void showURL( QUrl );
    
signals:
    void setCover( const QPixmap& );
    void parseURL( const QUrl& );
    
protected slots:
    void coverDownloadError(QString);
    void setCoverImageFromDownloader();
    void parseWebViewURL();

private:
    std::unique_ptr<Ui::WebBrowserWidget>        m_pclUI;
    std::unique_ptr<class QNetworkAccessManager> m_pclNetworkAccess;
    std::unique_ptr<class CoverDownloader>       m_pclCoverDownloader;
};

#endif // WEBBROWSERWIDGET_H
