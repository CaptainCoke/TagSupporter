#ifndef WIKIPEDIAPARSER_H
#define WIKIPEDIAPARSER_H

#include "OnlineSourceParser.h"
#include <QCache>

class WikipediaParser : public OnlineSourceParser
{
    Q_OBJECT
public:
    ~WikipediaParser() override;
    
    void sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle, int iYear ) override;
    void clearResults() override;
    
    QStringList getPages() const override;
    std::shared_ptr<OnlineInfoSource> getResult( const QString& strPage ) const override;
    
    const QIcon& getIcon() const override;
public slots:
    void parseFromURL( const QUrl& rclUrl );
    
protected slots:
    void replyReceived();
   
protected:
    virtual QStringList createTitleRequests( const QStringList& lstArtists, const QString& trackTitle, const QString& albumTitle ) = 0;
    virtual bool matchesDiscography( const QString& strTitle ) const = 0;
    virtual QPixmap overlayLanguageHint( QPixmap clIcon ) const = 0;
    explicit WikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    
    // returns true, if result to new queries are still expected
    bool resolveSearchQueries( QStringList lstQueries );
    bool resolveTitleURLs( QStringList lstTitles );
    bool resolveCoverImageURLs( QStringList lstCoverImageTitles );
    void parseWikipediaAPIJSONReply( QByteArray strReply );
    void parseWikiText( QString strTitle, QString strContent, QStringList& lstImageTitles, QStringList& lstRedirectTitles );
    void replaceCoverImageURL( QString strTitle, QString strURL );
    QString lemma2URL( QString strLemma, QString strSection ) const;
    
    // returns true if new content was queried
    bool getContentFromCacheAndQueryMissing( const QStringList& lstTitles );
    bool getCoverImageURLsFromCacheAndQueryMissing( const QStringList& lstCoverImageTitles );
    bool getSearchResultFromCacheAndQueryMissing( const QString& strQuery );
    
    void allContentAdded();
    
    void downloadFavicon(QNetworkAccessManager *pclNetworkAccess);
    
    // gets content for titles in list from cache if possible. Returns list of noncached titles
    QStringList getContentFromCache( const QStringList& lstTitles );
    QStringList getRedirectsFromCache( const QString& strTitle );
    QStringList getCoverImageURLsFromCache( const QStringList& lstCoverImageTitles );
    
    QNetworkRequest createContentRequest( const QStringList & lstTitles ) const;
    QNetworkRequest createImageRequest( const QStringList& lstCoverImageTitles ) const;
    QNetworkRequest createSearchRequest( const QString& strQuery ) const;
    
    // remember the last requested for later use during parsing
    QString m_strTrackTitle, m_strAlbumTitle, m_strTrackArtist;
    int     m_iYear = -1;
    
    QString m_strLanguageSubDomain;
    QStringList m_lstParsedPages; // remember the already parsed pages to avoid double work due to redirects
    bool m_bSearchConducted;
    std::unique_ptr<QIcon> m_pclIcon;
    
    using SectionsToInfo = std::map<QString,std::shared_ptr<OnlineInfoSource>>;
    SectionsToInfo m_mapParsedInfos;
    
    QCache<QString,QStringList>    m_lruSearchResults;  // caches titles returned for a given search
    QCache<QString,QStringList>    m_lruRedirects;      // caches any redirects for a given title
    QCache<QString,SectionsToInfo> m_lruContent;        // caches all sections for a given title
    QCache<QString,QString>        m_lruCoverImageURLs; // caches image URLs for a given title
};

class EnglishWikipediaParser : public WikipediaParser
{
public:
    explicit EnglishWikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    ~EnglishWikipediaParser() override = default;
    
protected:
    QStringList createTitleRequests( const QStringList& lstArtists, const QString& trackTitle, const QString& albumTitle ) override;
    bool matchesDiscography( const QString& strTitle ) const override;
    QPixmap overlayLanguageHint( QPixmap clIcon ) const override;
};

class GermanWikipediaParser : public WikipediaParser
{
public:
    explicit GermanWikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    ~GermanWikipediaParser() override = default;
    
protected:
    QStringList createTitleRequests( const QStringList& lstArtists, const QString& trackTitle, const QString& albumTitle ) override;
    bool matchesDiscography( const QString& strTitle ) const override;
    QPixmap overlayLanguageHint( QPixmap clIcon ) const override;
};

#endif // WIKIPEDIAPARSER_H
