#ifndef COVERDOWNLOADER_H
#define COVERDOWNLOADER_H

#include <QObject>
#include <memory>

class QNetworkRequest;
class QNetworkAccessManager;

class CoverDownloader : public QObject
{
    Q_OBJECT
public:
    CoverDownloader(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    ~CoverDownloader() override;

    void downloadImage( const QUrl& rclURL );
    const QPixmap& getImage() const;
    void clear();

public slots:
    void downloadImage( const QNetworkRequest& rclRequest );
    
signals:
    void imageReady();
    void error(QString);

protected slots:
    void fileDownloaded();
    
protected:
    QNetworkAccessManager* m_pclNetworkAccess;
    std::unique_ptr<QPixmap> m_pclPixmap;
};

#endif // COVERDOWNLOADER_H
