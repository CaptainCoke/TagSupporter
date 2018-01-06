#include "DiscogsParser.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QIcon>
#include "DiscogsInfoSources.h"
#include "CoverDownloader.h"

DiscogsParser::DiscogsParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent)
: OnlineSourceParser(pclNetworkAccess,pclParent)
, m_lruSearchResults(10000)
, m_lruContent(1000)
, m_lruCoverURLs(10000)
, m_pclIcon( std::make_unique<QIcon>() )
{
    downloadFavicon(pclNetworkAccess);
}

DiscogsParser::~DiscogsParser() = default;

void DiscogsParser::getNextSearchResultFromCacheOrSendQuery()
{
    bool b_no_queries_required = true;
    while ( !m_lstOpenSearchQueries.empty() )
    {
        QString str_query, str_type;
        std::tie(str_query,str_type) = std::move(m_lstOpenSearchQueries.front());
        m_lstOpenSearchQueries.pop_front();
    
        str_query = QUrl::toPercentEncoding( str_query );
                
        // look in LRU for search query
        int* pi_id = m_lruSearchResults[SearchQuery(str_query,str_type)];
        if ( pi_id != nullptr )
        {
            if ( *pi_id != 0 ) // check if cached search turned up any valid results
                // use cached search result
                b_no_queries_required &= getContentFromCacheAndQueryMissing( *pi_id, str_type+"s" );
        }
        else
        {
            sendSearchRequest(str_query, str_type);
            return;
        }
    }
    if ( b_no_queries_required )
        emit parsingFinished( getPages() );
}

bool DiscogsParser::getContentFromCacheAndQueryMissing(int iID, const QString &strType)
{
    SourcePtr* pcl_source = m_lruContent[ContentId(iID,strType)];
    if ( pcl_source != nullptr )
        return !(*pcl_source) || addParsedContent(*pcl_source,strType);
    
    sendContentRequest( iID, strType );
    return false;
}

bool DiscogsParser::getCoverURLFromCacheAndQueryMissing(int iID, const QString &strType)
{
    QString* str_cover_url = m_lruCoverURLs[iID];
    if ( str_cover_url != nullptr )
    {
        addCoverURLToSource( iID, *str_cover_url );
        return true;
    }
    
    sendCoverRequest(iID, strType);
    return false;
}

void DiscogsParser::sendSearchRequest( const QString& strQuery, const QString& strType )
{
    
    QNetworkRequest cl_request(QUrl(QString("https://www.discogs.com/search/?q=%1&type=%2").arg( strQuery, strType )));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    emit sendQuery( cl_request, SLOT(searchReplyReceived()) );
}

void DiscogsParser::sendContentRequest( int iID, const QString& strType )
{
    QNetworkRequest cl_request(QUrl(QString("https://api.discogs.com/%2/%1").arg(iID).arg(strType)));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    emit sendQuery( cl_request, SLOT(contentReplyReceived()) );
}

void DiscogsParser::sendCoverRequest(int iID, const QString &strType)
{
    QNetworkRequest cl_request(QUrl(QString("https://www.discogs.com/%1/%2/images").arg( strType ).arg( iID )));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    emit sendQuery( cl_request, SLOT(imageReplyReceived()) );
}


void DiscogsParser::sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle, int iYear )
{
    clearResults();
    
    m_strAlbumTitle  = albumTitle;
    m_strTrackTitle  = trackTitle;
    m_strTrackArtist = trackArtist;
    m_iYear          = iYear;
    
    // create a list of seach queries for releases
    QStringList lst_release_queries;
    if ( !trackTitle.isEmpty() )
        lst_release_queries << trackTitle;
    if ( !albumTitle.isEmpty() )
        lst_release_queries << albumTitle;
    
    lst_release_queries.removeDuplicates();
        
    for ( const QString& str_query : lst_release_queries )
        m_lstOpenSearchQueries.emplace_back( str_query, "release" );
    
    if ( !trackArtist.isEmpty())
    {
        m_lstOpenSearchQueries.emplace_front( trackArtist, "artist" );
        // place most promising queries at front of list...
        for ( const QString& str_query : lst_release_queries )
            m_lstOpenSearchQueries.emplace_front( trackArtist+" "+str_query, "release" );
    }
            
    // start by sending out the first search request in the list
    getNextSearchResultFromCacheOrSendQuery();
}

void DiscogsParser::clearResults()
{
    m_mapParsedInfos.clear();
    m_strAlbumTitle.clear();
    m_strTrackTitle.clear();
    m_strTrackArtist.clear();
    m_lstOpenSearchQueries.clear();
    m_iYear = -1;
    
    //cancel any pending requests
    emit cancelAllPendingNetworkRequests();
}

QStringList DiscogsParser::getPages() const
{
    // get pages, sorted by significance
    std::vector<std::pair<int,QString>> lst_pages_with_significance;
    lst_pages_with_significance.reserve( m_mapParsedInfos.size() );
    for ( const auto & rcl_item : m_mapParsedInfos )
        lst_pages_with_significance.emplace_back( rcl_item.second->significance(m_strAlbumTitle,m_strTrackArtist,m_strTrackTitle,m_iYear), rcl_item.second->title() );
    std::sort( lst_pages_with_significance.begin(), lst_pages_with_significance.end() );
    QStringList lst_pages;
    for ( auto it_item = lst_pages_with_significance.rbegin(); it_item != lst_pages_with_significance.rend(); ++it_item )
        lst_pages << it_item->second;
    return lst_pages;
}

std::shared_ptr<OnlineInfoSource> DiscogsParser::getResult(const QString &strPage) const
{
    for ( const auto & rcl_item : m_mapParsedInfos )
        if ( rcl_item.second->title() == strPage )
            return rcl_item.second;
    return nullptr;
}

const QIcon &DiscogsParser::getIcon() const
{
    return *m_pclIcon;
}

static bool getIdAndTypeFromURL( const QUrl& rclUrl, QString& strType, int& iId )
{
    // check if URL is discogs
    if ( !rclUrl.host().endsWith( "discogs.com" ) )
        return false;
    
    // get the id and type from the URL
    QStringList lst_url_parts = rclUrl.path().split( "/" );
    if ( !lst_url_parts.isEmpty() && lst_url_parts.back().contains("images") )
        lst_url_parts.pop_back();
    if ( lst_url_parts.size() < 2 )
        return false;
    
    // try to get the id of the content (stored in last part of URL)
    QRegularExpressionMatch cl_id_match = QRegularExpression("^([0-9]+)[^0-9]*.*").match( lst_url_parts.back() );
    if ( !cl_id_match.hasMatch() )
        return false;
    
    bool b_ok;
    iId = cl_id_match.captured(1).toInt(&b_ok);
    if ( !b_ok ) // not an ID
        return false;
    
    lst_url_parts.pop_back();
    strType = lst_url_parts.back().toLower();
    return true;
}

void DiscogsParser::parseFromURL(const QUrl &rclUrl)
{
    if ( !rclUrl.host().endsWith( "discogs.com" ) )
        return;
    
    QString str_type;
    int i_id;
    if ( getIdAndTypeFromURL(rclUrl, str_type, i_id) )
        getContentFromCacheAndQueryMissing( i_id, str_type+"s" );
    else
        emit error( QString("Network reply URL %1 could not be resolved to a known type of source").arg(rclUrl.toString()) );   
}

template<typename FunT>
void DiscogsParser::replyReceived(QNetworkReply* pclReply,FunT funAction, const char * strRedirectReplySlot)
{
    if ( !pclReply )
        return;
    
    // check for error
    switch( pclReply->error() )
    {
    case QNetworkReply::NoError:
    {
        // check if reply contains a redirect instead of content
        QVariant cl_redirect = pclReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if ( cl_redirect.isValid() )
        {
            //follow the redirect
            QUrl cl_new_url = pclReply->url().resolved( cl_redirect.toUrl() );
            QNetworkRequest cl_request(cl_new_url);
            cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
            emit sendQuery(cl_request, strRedirectReplySlot );
        }
        else
        {
            QUrl cl_url = pclReply->url();
            startParserThread( pclReply->readAll(), [funAction,cl_url](QByteArray strReply){ funAction( strReply, cl_url ); } );
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        emit info( QString("Network reply to %1 was canceled").arg(pclReply->url().toString()) );
        break;
    default:
        emit error( QString("Network reply to %1 received error: %2").arg(pclReply->url().toString(),pclReply->errorString()) );   
        break;
    }
    pclReply->deleteLater();
}

void DiscogsParser::searchReplyReceived()
{
   replyReceived( dynamic_cast<QNetworkReply*>( sender() ), [this](const QByteArray& rclContent, const QUrl& rclRequestUrl){ parseSearchResult(rclContent,rclRequestUrl); },
        SLOT(searchReplyReceived()));
   getNextSearchResultFromCacheOrSendQuery();
}

void DiscogsParser::contentReplyReceived()
{
    replyReceived( dynamic_cast<QNetworkReply*>( sender() ), [this](const QByteArray& rclContent, const QUrl& rclRequestUrl){ parseContent(rclContent,rclRequestUrl); },
        SLOT(contentReplyReceived()));
}

void DiscogsParser::imageReplyReceived()
{
    replyReceived( dynamic_cast<QNetworkReply*>( sender() ), [this](const QByteArray& rclContent, const QUrl& rclRequestUrl){ parseImages(rclContent,rclRequestUrl); },
        SLOT(imageReplyReceived()));
}

void DiscogsParser::parseContent( const QByteArray& rclContent, const QUrl& rclRequestUrl )
{
    QJsonDocument cl_doc = QJsonDocument::fromJson(rclContent);
    if ( cl_doc.isObject() )
    {
        QString str_type;
        int i_id;
        if ( getIdAndTypeFromURL( rclRequestUrl, str_type, i_id ) )
        {
            SourcePtr pcl_source;
            // figure out what type of reply this is based on API URL
            pcl_source = DiscogsInfoSource::createForType( str_type, cl_doc );
            
            // add parsed source to cache
            m_lruContent.insert(ContentId(i_id,str_type), new SourcePtr(pcl_source) );
            
            // add source to parsed infos map
            if ( !pcl_source || addParsedContent( pcl_source, str_type ) )
                emit parsingFinished( QStringList()<<pcl_source->title() );
        }
        else
            emit error( QString("Network reply URL %1 could not be resolved to a known type of source").arg(rclRequestUrl.toString()) );   
    }
    else 
        emit error("received an invalid JSON reply");
}

bool DiscogsParser::addParsedContent( SourcePtr pclSource, const QString& strType )
{
    m_mapParsedInfos[pclSource->id()] = pclSource;
    
    // check just HOW well the found source matches our original query...
    if ( pclSource->perfectMatch( m_strAlbumTitle, m_strTrackArtist, m_strTrackTitle ) ) // cancel any open search queries... it doesn't get any better than this...
        m_lstOpenSearchQueries.clear();
    
    auto pcl_album = std::dynamic_pointer_cast<DiscogsAlbumInfo>(pclSource);
    if ( pcl_album && pcl_album->getCover().isEmpty() )
        return getCoverURLFromCacheAndQueryMissing(pclSource->id(),strType.left(strType.size()-1));
    return true;
}

void DiscogsParser::parseImages( const QByteArray& rclContent, const QUrl& rclRequestUrl )
{
    QString str_type;
    int i_id;
    if ( !getIdAndTypeFromURL( rclRequestUrl, str_type, i_id ) )
    {
        emit error( QString("Network reply URL %1 could not be resolved to an ID of source").arg(rclRequestUrl.toString()) );   
        return;
    }
    
    SourcePtr pcl_source;
    
    // get first jpeg source URL in an image tag
    QRegularExpression cl_src_regexp( "<img[\\s]+src=\"([^\"]+)", QRegularExpression::CaseInsensitiveOption );
    QRegularExpressionMatchIterator cl_matches = cl_src_regexp.globalMatch( QString( rclContent ) );
    while( cl_matches.hasNext() )
    {
        QRegularExpressionMatch cl_match = cl_matches.next();
        QString str_match = cl_match.captured(1);
        if ( str_match.endsWith(".jpg", Qt::CaseInsensitive) )
        {
            // store found URL in cache
            m_lruCoverURLs.insert( i_id, new QString(str_match) );
            // and set the cover
            pcl_source = addCoverURLToSource( i_id, std::move(str_match) );
            break;
        }
    }
    if ( pcl_source )
        emit parsingFinished(QStringList()<<pcl_source->title());
    else
        emit parsingFinished(QStringList()); // we're definetely at the end of a parsing chain... do something
}

void DiscogsParser::downloadFavicon(QNetworkAccessManager *pclNetworkAccess)
{
    CoverDownloader* pcl_downloader = new CoverDownloader( pclNetworkAccess, this );
    connect( pcl_downloader, &CoverDownloader::imageReady, [this,pcl_downloader]{
        m_pclIcon = std::make_unique<QIcon>( pcl_downloader->getImage() );
        pcl_downloader->deleteLater();
    } );
    pcl_downloader->downloadImage( QUrl("https://www.discogs.com/favicon.ico") );
}

DiscogsParser::SourcePtr DiscogsParser::addCoverURLToSource( int iID, QString strCoverURL )
{
    auto it_info = m_mapParsedInfos.find( iID );
    if ( it_info == m_mapParsedInfos.end() ) // got an image result for a nonexisting item(?!)
        return nullptr;
    auto pcl_album = std::dynamic_pointer_cast<DiscogsAlbumInfo>(it_info->second);
    if ( pcl_album )
        pcl_album->setCover(std::move(strCoverURL));
    return pcl_album;
}


void DiscogsParser::parseSearchResult(const QByteArray& rclContent, const QUrl& rclRequestUrl)
{
    // get the type argument from request URL
    QRegularExpressionMatch cl_match = QRegularExpression( "&type=([^&]+)" ).match( rclRequestUrl.query(QUrl::FullyEncoded) );
    if ( !cl_match.hasMatch() )
    {
        emit error( QString("Network reply URL %1 did not contain a known query type").arg(rclRequestUrl.toString()) );   
        return;
    }
    QString str_type = cl_match.captured(1);
    
    // get the query argument from request URL
    cl_match = QRegularExpression( "^q=([^&]+)" ).match( rclRequestUrl.query(QUrl::FullyEncoded) );
    if ( !cl_match.hasMatch() )
    {
        emit error( QString("Network reply URL %1 did not contain a known query string").arg(rclRequestUrl.toString()) );   
        return;
    }
    QString str_query = cl_match.captured(1);
    
    
    // get first hyperlink that matches given type
    QString str_regexp = QString("href=\"([^\"]+%1[^\"]+)").arg(str_type);
    QRegularExpression cl_href_regexp( str_regexp, QRegularExpression::CaseInsensitiveOption );
    QRegularExpressionMatchIterator cl_matches = cl_href_regexp.globalMatch( QString( rclContent ) );
    while( cl_matches.hasNext() )
    {
        QRegularExpressionMatch cl_match = cl_matches.next();
        QUrl cl_full_url( cl_match.captured(1) );
        cl_full_url.setHost( "discogs.com" );
        QString str_type;
        int i_id;
        if ( getIdAndTypeFromURL(cl_full_url, str_type, i_id) && str_type.compare( str_type, Qt::CaseInsensitive ) == 0 )
        {
            // store search result in cache
            m_lruSearchResults.insert( SearchQuery(str_query,str_type), new int(i_id) );
            getContentFromCacheAndQueryMissing( i_id, str_type+"s" );
            return;
        }
    }
    // store in cache that there was no result
    m_lruSearchResults.insert( SearchQuery(str_query,str_type), new int(0) );
    emit info( QString("no usable results found in search reply to \"%1\"").arg( rclRequestUrl.query() ) );
}
