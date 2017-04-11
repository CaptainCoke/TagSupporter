#include "DiscogsParser.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QRegularExpression>
#include "DiscogsInfoSources.h"

DiscogsParser::DiscogsParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent)
: OnlineSourceParser(pclParent)
, m_pclNetworkAccess(pclNetworkAccess)
{
}

DiscogsParser::~DiscogsParser() = default;

void DiscogsParser::sendNextSearchRequest()
{
    if ( m_lstOpenSearchQueries.empty() )
        return;
    
    QString str_query, str_type;
    std::tie(str_query,str_type) = std::move(m_lstOpenSearchQueries.front());
    m_lstOpenSearchQueries.pop_front();
    
    QNetworkRequest cl_request(QUrl(QString("https://www.discogs.com/search/?q=%1&type=%2").arg( QUrl::toPercentEncoding(str_query), str_type )));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    QNetworkReply* pcl_reply = m_pclNetworkAccess->get( cl_request );
    connect(this, SIGNAL(cancelAllPendingNetworkRequests()), pcl_reply, SLOT(abort()) );
    connect(pcl_reply, SIGNAL(finished()), this, SLOT(searchReplyReceived()));
}

void DiscogsParser::sendContentRequest( int iID, const QString& strType )
{
    QNetworkRequest cl_request(QUrl(QString("https://api.discogs.com/%2/%1").arg(iID).arg(strType)));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    QNetworkReply* pcl_reply = m_pclNetworkAccess->get( cl_request );
    connect(this, SIGNAL(cancelAllPendingNetworkRequests()), pcl_reply, SLOT(abort()) );
    connect(pcl_reply, SIGNAL(finished()), this, SLOT(contentReplyReceived()));
}

void DiscogsParser::sendCoverRequest(int iID, const QString &strType)
{
    QNetworkRequest cl_request(QUrl(QString("https://www.discogs.com/%1/%2/images").arg( strType ).arg( iID )));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    QNetworkReply* pcl_reply = m_pclNetworkAccess->get( cl_request );
    connect(this, SIGNAL(cancelAllPendingNetworkRequests()), pcl_reply, SLOT(abort()) );
    connect(pcl_reply, SIGNAL(finished()), this, SLOT(imageReplyReceived()));
}


void DiscogsParser::sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle )
{
    clearResults();
    
    m_strAlbumTitle  = albumTitle;
    m_strTrackTitle  = trackTitle;
    m_strTrackArtist = trackArtist;
    
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
        for ( QString& str_query : lst_release_queries )
            m_lstOpenSearchQueries.emplace_front( str_query, "release" );
    }
    
    // start by sending out the first search request in the list
    sendNextSearchRequest();
}

void DiscogsParser::clearResults()
{
    m_mapParsedInfos.clear();
    m_strAlbumTitle.clear();
    m_strTrackTitle.clear();
    m_strTrackArtist.clear();
    m_lstOpenSearchQueries.clear();
    
    //cancel any pending requests
    emit cancelAllPendingNetworkRequests();
}

QStringList DiscogsParser::getPages() const
{
    // get pages, sorted by significance
    std::vector<std::pair<int,QString>> lst_pages_with_significance;
    lst_pages_with_significance.reserve( m_mapParsedInfos.size() );
    for ( const auto & rcl_item : m_mapParsedInfos )
        lst_pages_with_significance.emplace_back( rcl_item.second->significance(), rcl_item.second->title() );
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
        sendContentRequest( i_id, str_type+"s" );
    else
        emit error( QString("Network reply URL %1 could not be resolved to a known type of source").arg(rclUrl.toString()) );   
}

template<typename FunT>
void DiscogsParser::replyReceived(QNetworkReply* pclReply,FunT funAction, const char * strRedirectReplySlot)
{
    if ( !pclReply )
        return;
    
    // check for error
    if ( pclReply->error() == QNetworkReply::NoError )
    {
        // check if reply contains a redirect instead of content
        QVariant cl_redirect = pclReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if ( cl_redirect.isValid() )
        {
            //follow the redirect
            QUrl cl_new_url = pclReply->url().resolved( cl_redirect.toUrl() );
            QNetworkRequest cl_request(cl_new_url);
            cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
            QNetworkReply* pcl_reply = m_pclNetworkAccess->get( cl_request );
            connect(this, SIGNAL(cancelAllPendingNetworkRequests()), pcl_reply, SLOT(abort()) );
            if ( !connect(pcl_reply, SIGNAL(finished()), this, strRedirectReplySlot) )
                emit error( QString( "Original reply to %1 requested redirect to %2, but no suitable SLOT to handle redirect could be connected" ) );
        }
        else
            funAction( pclReply->readAll(), pclReply->url() );
    }
    else
        emit error( QString("Network reply to %1 received error: %2").arg(pclReply->url().toString(),pclReply->errorString()) );   
    pclReply->deleteLater();
}

void DiscogsParser::searchReplyReceived()
{
   replyReceived( dynamic_cast<QNetworkReply*>( sender() ), [this](const QByteArray& rclContent, const QUrl& rclRequestUrl){ parseSearchResult(rclContent,rclRequestUrl); },
        SLOT(searchReplyReceived()));
   sendNextSearchRequest();
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
            std::shared_ptr<DiscogsInfoSource> pcl_source;
            // figure out what type of reply this is based on API URL
            pcl_source = DiscogsInfoSource::createForType( str_type, cl_doc );
            if ( pcl_source )
                m_mapParsedInfos[pcl_source->id()] = pcl_source;
            
            auto pcl_album = std::dynamic_pointer_cast<DiscogsAlbumInfo>(pcl_source);
            if ( pcl_album && pcl_album->getCover().isEmpty() )
                sendCoverRequest(pcl_source->id(),str_type.left(str_type.size()-1));
            else
                emit parsingFinished(QStringList()<<pcl_source->title());
            
            // check just HOW well the found source matches our original query...
            bool b_perfect_match = pcl_source->matchesAlbum(m_strAlbumTitle) && pcl_source->matchesArtist(m_strTrackArtist) && pcl_source->matchesTrackTitle(m_strTrackTitle);
            if ( b_perfect_match ) // cancel any open search queries... it doesn't get any better than this...
                m_lstOpenSearchQueries.clear();
        }
        else
            emit error( QString("Network reply URL %1 could not be resolved to a known type of source").arg(rclRequestUrl.toString()) );   
    }
    else 
        emit error("received an invalid JSON reply");
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
    auto it_info = m_mapParsedInfos.find( i_id );
    if ( it_info == m_mapParsedInfos.end() ) // got an image result for a nonexisting item(?!)
        return;
    auto pcl_album = std::dynamic_pointer_cast<DiscogsAlbumInfo>(it_info->second);
    if ( !pcl_album )
        return;
    
    // get first jpeg source URL in an image tag
    QRegularExpression cl_src_regexp( "<img[\\s]+src=\"([^\"]+)", QRegularExpression::CaseInsensitiveOption );
    QRegularExpressionMatchIterator cl_matches = cl_src_regexp.globalMatch( QString( rclContent ) );
    while( cl_matches.hasNext() )
    {
        QRegularExpressionMatch cl_match = cl_matches.next();
        QString str_match = cl_match.captured(1);
        if ( str_match.endsWith(".jpg", Qt::CaseInsensitive) )
        {
            pcl_album->setCover(std::move(str_match));
            break;
        }
    }
    emit parsingFinished(QStringList()<<pcl_album->title());
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
            sendContentRequest( i_id, str_type+"s" );
            return;
        }
    }
    emit info( QString("no usable results found in search reply to \"%1\"").arg( rclRequestUrl.query() ) );
}
