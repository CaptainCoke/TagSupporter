#ifndef DISCOGSPARSER_H
#define DISCOGSPARSER_H

#include "OnlineSourceParser.h"
#include <QCache>

class QNetworkReply;
class QNetworkAccessManager;

class DiscogsParser : public OnlineSourceParser
{
    Q_OBJECT
public:
    explicit DiscogsParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    ~DiscogsParser() override;
    
    void sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle, int iYear ) override;
    void clearResults() override;
    
    QStringList getPages() const override;
    std::shared_ptr<OnlineInfoSource> getResult( const QString& strPage ) const override;
    
    const QIcon& getIcon() const override;
public slots:
    void parseFromURL( const QUrl& rclUrl );
    
protected slots:
    void searchReplyReceived();
    void contentReplyReceived();
    void imageReplyReceived();
    
protected:   
    template<typename FunT>
    void replyReceived( QNetworkReply* pclReply,FunT funAction, const char * strRedirectReplySlot);
    
    void parseSearchResult( const QByteArray& rclContent, const QUrl& rclRequestUrl );
    void parseContent( const QByteArray& rclContent, const QUrl& rclRequestURL );
    void parseImages( const QByteArray& rclContent, const QUrl& rclRequestURL );
    
    void resolveItems( QStringList lstItems );
    
    void downloadFavicon(QNetworkAccessManager *pclNetworkAccess);
    
    using SearchQuery = std::pair<QString,QString>;
    using ContentId   = std::pair<int,QString>;
    using SourcePtr   = std::shared_ptr<class DiscogsInfoSource>;
    std::map<int,SourcePtr> m_mapParsedInfos;
    
    // remember the last requested for later use during parsing
    QString m_strTrackTitle, m_strAlbumTitle, m_strTrackArtist;
    int     m_iYear = -1;
    
    
    std::list<SearchQuery>      m_lstOpenSearchQueries;
    QCache<SearchQuery,int>     m_lruSearchResults;
    QCache<ContentId,SourcePtr> m_lruContent;
    QCache<int,QString>         m_lruCoverURLs;
    
    std::unique_ptr<QIcon> m_pclIcon;
    
    void getNextSearchResultFromCacheOrSendQuery();
    
    // return true, if no network query was necessary
    bool addParsedContent( SourcePtr pclSource, const QString& strType );
    bool getContentFromCacheAndQueryMissing(int iID, const QString &strType);
    bool getCoverURLFromCacheAndQueryMissing(int iID, const QString &strType);
    SourcePtr addCoverURLToSource( int iID, QString strCoverURL );
    
    void sendSearchRequest( const QString& strQuery, const QString& strType );
    void sendContentRequest(int iID, const QString &strType);
    void sendCoverRequest(int iID, const QString &strType);
};

#endif // DISCOGSPARSER_H
