#ifndef DISCOGSPARSER_H
#define DISCOGSPARSER_H

#include "OnlineSourceParser.h"

class QNetworkAccessManager;

class DiscogsParser : public OnlineSourceParser
{
    Q_OBJECT
public:
    explicit DiscogsParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    ~DiscogsParser() override;
    
    void sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle ) override;
    void clearResults() override;
    
    QStringList getPages() const override;
    std::shared_ptr<OnlineInfoSource> getResult( const QString& strPage ) const override;
    
public slots:
    void parseFromURL( const QUrl& rclUrl );
    
protected slots:
    void searchReplyReceived();
    void contentReplyReceived();
    void imageReplyReceived();
    
protected:
    template<typename FunT>
    void replyReceived( class QNetworkReply* pclReply,FunT funAction, const char * strRedirectReplySlot);
    
    void parseSearchResult( const QByteArray& rclContent, const QUrl& rclRequestUrl );
    void parseContent( const QByteArray& rclContent, const QUrl& rclRequestURL );
    void parseImages( const QByteArray& rclContent, const QUrl& rclRequestURL );
    
    void resolveItems( QStringList lstItems );
    
    QNetworkAccessManager* m_pclNetworkAccess = nullptr;
    std::map<int,std::shared_ptr<class DiscogsInfoSource>> m_mapParsedInfos;
    void sendSearchRequest(const QString &strQuery, const QString &strType);
    void sendContentRequest(int iID, const QString &strType);
    void sendCoverRequest(int iID, const QString &strType);
};

#endif // DISCOGSPARSER_H
