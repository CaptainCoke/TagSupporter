#ifndef WIKIPEDIAPARSER_H
#define WIKIPEDIAPARSER_H

#include "OnlineSourceParser.h"

class QNetworkAccessManager;
class QNetworkRequest;

class WikipediaParser : public OnlineSourceParser
{
    Q_OBJECT
public:
    ~WikipediaParser() override;
    
    void sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle ) override;
    void clearResults() override;
    
    QStringList getPages() const override;
    std::shared_ptr<OnlineInfoSource> getResult( const QString& strPage ) const override;
    
public slots:
    void parseFromURL( const QUrl& rclUrl );
    
protected slots:
    void replyReceived();
   
protected:
    virtual QStringList createTitleRequests( const QStringList& lstArtists, const QString& trackTitle, const QString& albumTitle ) = 0;
    virtual bool matchesDiscography( const QString& strTitle ) const = 0;
    explicit WikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    
    void resolveSearchQueries( QStringList lstQueries );
    void resolveTitleURLs( QStringList lstTitles );
    void resolveCoverImageURLs( QStringList lstCoverImages );
    void parseWikipediaAPIJSONReply( QByteArray strReply );
    void parseWikiText( QString strTitle, QString strContent, QStringList& lstImageTitles, QStringList& lstRedirectTitles );
    void replaceCoverImageURL( QString strTitle, QString strURL );
    QString lemma2URL( QString strLemma, QString strSection ) const;
    
    QNetworkRequest createContentRequest( const QStringList & lstTitles ) const;
    QNetworkRequest createImageRequest( const QStringList& lstTitles ) const;
    QNetworkRequest createSearchRequest( const QString& strQuery ) const;
    
    QNetworkAccessManager* m_pclNetworkAccess = nullptr;
    QString m_strTrackTitle, m_strAlbumTitle, m_strTrackArtist; // remember the last requested for later use during parsing
    QString m_strLanguageSubDomain;
    QStringList m_lstParsedPages; // remember the already parsed pages to avoid double work due to redirects
    bool m_bSearchConducted;
    
    std::map<QString,std::shared_ptr<OnlineInfoSource>> m_mapParsedInfos;
};

class EnglishWikipediaParser : public WikipediaParser
{
public:
    explicit EnglishWikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    ~EnglishWikipediaParser() override = default;
    
protected:
    QStringList createTitleRequests( const QStringList& lstArtists, const QString& trackTitle, const QString& albumTitle ) override;
    bool matchesDiscography( const QString& strTitle ) const override;
};

class GermanWikipediaParser : public WikipediaParser
{
public:
    explicit GermanWikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr);
    ~GermanWikipediaParser() override = default;
    
protected:
    QStringList createTitleRequests( const QStringList& lstArtists, const QString& trackTitle, const QString& albumTitle ) override;
    bool matchesDiscography( const QString& strTitle ) const override;
};

#endif // WIKIPEDIAPARSER_H
