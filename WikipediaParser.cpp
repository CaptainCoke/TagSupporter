#include "WikipediaParser.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QApplication>
#include <QIcon>
#include <QRegularExpressionMatchIterator>
#include <QPainter>
#include "WikipediaInfoSources.h"
#include "CoverDownloader.h"

WikipediaParser::WikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent)
: OnlineSourceParser(pclNetworkAccess,pclParent)
, m_pclIcon( std::make_unique<QIcon>() )
, m_lruSearchResults(10000)
, m_lruRedirects(10000)
, m_lruContent(1000)
, m_lruCoverImageURLs(10000)
{
    downloadFavicon(pclNetworkAccess);
}


WikipediaParser::~WikipediaParser() = default;

bool WikipediaParser::getContentFromCacheAndQueryMissing( const QStringList& lstTitles )
{
    // look in LRU for each title before sending
    QStringList lst_non_cached = getContentFromCache( lstTitles );
    if ( lst_non_cached.isEmpty() )
        return false;
    else
        emit sendQuery( createContentRequest(lst_non_cached), SLOT(replyReceived()) );
    return true;
}

bool WikipediaParser::getCoverImageURLsFromCacheAndQueryMissing( const QStringList& lstCoverImageTitles )
{
    // look in LRU for each title before sending
    QStringList lst_non_cached = getCoverImageURLsFromCache( lstCoverImageTitles );
    if ( lst_non_cached.isEmpty() )
        return false;
    else
        emit sendQuery( createImageRequest(lstCoverImageTitles), SLOT(replyReceived()) );
    return true;
}

bool WikipediaParser::getSearchResultFromCacheAndQueryMissing( const QString& strQuery )
{
    QStringList* lst_content_titles = m_lruSearchResults[strQuery];
    if ( lst_content_titles != nullptr )
        // the result to the search was cached... great.
        // however, maybe there is still work to be done in resolving the title URLs...
        return resolveTitleURLs( *lst_content_titles );
    else
        emit sendQuery( createSearchRequest(QUrl::toPercentEncoding(strQuery)), SLOT(replyReceived()) );
    return true;
}

QStringList WikipediaParser::getCoverImageURLsFromCache( const QStringList& lstCoverImageTitles )
{
    QStringList lst_noncached_titles;
    for ( const QString& strTitle : lstCoverImageTitles )
    {
        QString* map_parsed_URL = m_lruCoverImageURLs[strTitle];
        if ( map_parsed_URL != nullptr )
            // no need to query wikipedia again, we still have the content for this title in cache
            // replace with cached url
            replaceCoverImageURL( strTitle, *map_parsed_URL );
        else
            lst_noncached_titles << strTitle;
    }
    return lst_noncached_titles;
}

QStringList WikipediaParser::getContentFromCache( const QStringList& lstTitles )
{
    QStringList lst_noncached_titles;
    for ( const QString& strTitle : lstTitles )
    {
        for ( const QString & str_redirected_title : getRedirectsFromCache(strTitle) )
        {
            SectionsToInfo* map_parsed_infos = m_lruContent[strTitle];
            if ( map_parsed_infos != nullptr )
                // no need to query wikipedia again, we still have the content in cache
                // insert parsed infos into current info table
                m_mapParsedInfos.insert( map_parsed_infos->begin(), map_parsed_infos->end() );
            else
                lst_noncached_titles << str_redirected_title;
        }
    }
    return lst_noncached_titles;
}

QStringList WikipediaParser::getRedirectsFromCache( const QString& strTitle )
{
    QStringList* lst_redirects = m_lruRedirects[strTitle];
    if ( lst_redirects )
        return QStringList(*lst_redirects) << strTitle; // add self
    else
        return QStringList(strTitle); // only return self
}

QNetworkRequest WikipediaParser::createContentRequest( const QStringList & lstTitles ) const
{
    QNetworkRequest cl_request(QUrl(QString("https://%1.wikipedia.org/w/api.php?action=query&titles=%2&prop=revisions&rvprop=content&format=json").arg( m_strLanguageSubDomain, lstTitles.join("|"))));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    return cl_request;
}

QNetworkRequest WikipediaParser::createImageRequest( const QStringList& lstTitles ) const
{
    QNetworkRequest cl_request(QUrl(QString("https://%1.wikipedia.org/w/api.php?action=query&titles=File:%2&prop=imageinfo&iilimit=1&iiprop=url&format=json").arg( m_strLanguageSubDomain, lstTitles.join("|File:"))));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    return cl_request;
}

QNetworkRequest WikipediaParser::createSearchRequest( const QString& strQuery ) const
{
    QNetworkRequest cl_request(QUrl(QString("https://%1.wikipedia.org/w/api.php?action=query&list=search&srsearch=%2&srwhat=nearmatch&srprop=redirecttitle&format=json").arg( m_strLanguageSubDomain, strQuery )));
    cl_request.setRawHeader( "User-Agent", "TagSupporter/1.0 (https://hoov.de; coke@hoov.de) BasedOnQt/5" );
    return cl_request;
}

void WikipediaParser::sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle, int iYear )
{
    // clear the previous results
    clearResults();
    m_strAlbumTitle  = albumTitle;
    m_strTrackTitle  = trackTitle;
    m_strTrackArtist = trackArtist;
    m_iYear = iYear;
    
    // the track artist could also be combination of artist name (e.g. "feat." or "&" or "with"
    QStringList lst_artists = trackArtist.split( QRegularExpression("\\s(feat\\.|&|and|with|featuring)\\s", QRegularExpression::CaseInsensitiveOption), QString::SkipEmptyParts );
    lst_artists << trackArtist;
    lst_artists.removeDuplicates();
        
    bool b_any_queries_underway = resolveTitleURLs( createTitleRequests( lst_artists, trackTitle, albumTitle ) );
    if ( !b_any_queries_underway )
        allContentAdded();
}

void WikipediaParser::clearResults()
{
    m_mapParsedInfos.clear();
    m_lstParsedPages.clear();
    m_strAlbumTitle.clear();
    m_strTrackTitle.clear();
    m_strTrackArtist.clear();
    m_iYear = -1;
    m_bSearchConducted = false;
    
    //cancel any pending requests
    emit cancelAllPendingNetworkRequests();
}

void WikipediaParser::parseFromURL(const QUrl &rclUrl)
{
    // check if URL is of this wikipedia language (or from wikipedia at all)
    if ( !rclUrl.host().startsWith( m_strLanguageSubDomain + ".wikipedia" ) )
        return;
    
    // get the title from the URL
    QString str_title = rclUrl.path().split( "/" ).back();
    
    bool b_any_queries_underway = resolveTitleURLs( QStringList(str_title) );
    if ( !b_any_queries_underway )
        allContentAdded();
}



void WikipediaParser::replyReceived()
{
    QNetworkReply* pcl_reply = dynamic_cast<QNetworkReply*>( sender() );
    if ( !pcl_reply )
        return;
    
    // check for error
    switch ( pcl_reply->error() )
    {
    case QNetworkReply::NoError:
        startParserThread( pcl_reply->readAll(), [this](QByteArray strReply){ parseWikipediaAPIJSONReply( std::move(strReply) ); } );
        break;
    case QNetworkReply::OperationCanceledError:
        emit info( QString("Network reply to %1 was canceled").arg(pcl_reply->url().toString()) );
        break;
    default:
        emit error( QString("Network reply to %1 received error: %2").arg(pcl_reply->url().toString(),pcl_reply->errorString()) );   
        break;
    }
    pcl_reply->deleteLater();
}



bool WikipediaParser::resolveTitleURLs(QStringList lstTitles)
{
    lstTitles.removeDuplicates();
    for ( QString& str_title : lstTitles )
         str_title = QUrl::toPercentEncoding(str_title);
    if ( !lstTitles.isEmpty() )
        return getContentFromCacheAndQueryMissing( lstTitles );
    else
        emit error("unable to query wikipedia without either artist, title or album information");
    return false;
}

bool WikipediaParser::resolveSearchQueries(QStringList lstQueries)
{
    lstQueries.removeDuplicates();
    if ( !lstQueries.isEmpty() )
    {
        bool b_search_query_underway = false;
        for ( const QString& str_query : lstQueries )
            b_search_query_underway |= getSearchResultFromCacheAndQueryMissing( str_query );
        return b_search_query_underway;
    }
    else
        emit error("unable to search wikipedia without either artist, title or album information");
    return false;
}

bool WikipediaParser::resolveCoverImageURLs( QStringList lstCoverImageTitles )
{
    lstCoverImageTitles.removeDuplicates();
    for ( QString& str_title : lstCoverImageTitles )
         str_title = QUrl::toPercentEncoding(str_title);
    if ( !lstCoverImageTitles.isEmpty() )
        return getCoverImageURLsFromCacheAndQueryMissing(lstCoverImageTitles);
    else
        emit error("unable to query wikipedia to resolve images without any image titles");
    return false;
}

static QString reverseNormalization( QString strTitle, const QJsonArray& arrNormalizations )
{
    for ( const QJsonValue& rcl_normalization : arrNormalizations )
    {
        if ( rcl_normalization.toObject()["to"].toString().compare( strTitle ) == 0 )
            return rcl_normalization.toObject()["from"].toString();
    }
    return strTitle;
}



void WikipediaParser::parseWikipediaAPIJSONReply( QByteArray strReply )
{
    QJsonDocument cl_doc = QJsonDocument::fromJson(strReply);
    // find the relevant information
    if ( !cl_doc.isObject() )
    {
        emit error("received an invalid JSON reply");
        return;
    }
    QJsonObject cl_query_object = cl_doc.object()["query"].toObject();
    QJsonObject arr_pages         = cl_query_object["pages"].toObject();
    QJsonArray arr_search_results = cl_query_object["search"].toArray();
    QStringList lst_error_pages, lst_cover_images, lst_redirect_titles;
    for ( const QJsonValue& rcl_page : arr_pages )
    {
        QJsonObject cl_page = rcl_page.toObject();
        QString str_title = cl_page["title"].toString();
        if ( m_lstParsedPages.contains(str_title) )
            continue;
        
        if ( !cl_page.contains("missing") )
        {
            m_lstParsedPages << str_title;
            if ( cl_page.contains("revisions") ) // reply contains wikitext content
            {
                QString str_content = cl_page["revisions"].toArray()[0].toObject()["*"].toString();
                if ( !str_content.isEmpty() )
                {
                    parseWikiText( std::move(str_title), std::move(str_content), lst_cover_images, lst_redirect_titles );
                    continue;
                }
            }
            else if ( cl_page.contains("imageinfo") ) // reply contains image URLs
            {
                QString str_url = cl_page["imageinfo"].toArray()[0].toObject()["url"].toString();
                if ( !str_url.isEmpty() )
                {
                    // check if title has been normalized and invert
                    str_title = reverseNormalization(std::move(str_title), cl_query_object["normalized"].toArray());
                    
                    // add URL to lru cache for later
                    m_lruCoverImageURLs.insert(str_title, new QString(str_title) );
                    
                    // replace in content
                    replaceCoverImageURL( std::move(str_title), std::move(str_url) );
                    continue;
                }
            }
        }
        // add entry to LRU, so we don't try to query the missing title again!
        m_lruContent.insert( QUrl::toPercentEncoding(str_title), new SectionsToInfo() );
        
        // and mark as error page
        lst_error_pages << str_title;
    }
    for ( const QJsonValue& rcl_result : arr_search_results )
    {
        QJsonObject cl_result = rcl_result.toObject();
        QString str_title = cl_result["title"].toString();
        if ( !m_lstParsedPages.contains(str_title) )
            lst_redirect_titles << str_title;
    }
    
    bool b_more_queries_underway = false;
    if ( !lst_cover_images.empty() )
    {
        // make another call to resolve all cover image URLs at once
        b_more_queries_underway |= resolveCoverImageURLs(std::move(lst_cover_images));
    }
    if ( !lst_redirect_titles.empty() )
    {
        // make another call to resolve all redirect URLs
        b_more_queries_underway |= resolveTitleURLs(std::move(lst_redirect_titles));
    }
    if ( !b_more_queries_underway )
        allContentAdded();
    // verbose info:
    emit info( QString("found %1 pages: %2\nfailed for %3 pages: %4")
                              .arg(m_lstParsedPages.size()).arg(m_lstParsedPages.join("; "))
                              .arg(lst_error_pages.size()).arg(lst_error_pages.join("; ")) );
}

void WikipediaParser::allContentAdded()
{
    if ( m_mapParsedInfos.empty() && !m_bSearchConducted )
    {
        //nothing found yet... desperately attempt to make a title search first
        m_bSearchConducted = true;
        resolveSearchQueries( QStringList() << m_strTrackArtist << m_strAlbumTitle << m_strTrackTitle );
    }
    else
        emit parsingFinished(getPages());
}

void WikipediaParser::downloadFavicon(QNetworkAccessManager *pclNetworkAccess)
{
    CoverDownloader* pcl_downloader = new CoverDownloader( pclNetworkAccess, this );
    connect( pcl_downloader, &CoverDownloader::imageReady, [this,pcl_downloader]{
        m_pclIcon = std::make_unique<QIcon>( overlayLanguageHint( pcl_downloader->getImage() ) );
        pcl_downloader->deleteLater();
    } );
    pcl_downloader->downloadImage( QUrl("https://en.wikipedia.org/favicon.ico") );
}

void WikipediaParser::replaceCoverImageURL( QString strTitle, QString strURL )
{
    for ( auto & rcl_item : m_mapParsedInfos )
    {
        auto pcl_album_box = std::dynamic_pointer_cast<WikipediaAlbumInfoBox>(rcl_item.second);
        if ( pcl_album_box && strTitle.compare( "File:"+pcl_album_box->getCover(), Qt::CaseInsensitive ) == 0 )
            pcl_album_box->setCover( strURL );
    }
}

static std::list<std::unique_ptr<WikipediaInfoBox>> getInfoBoxes(const QString& strTitle, const QString& strContent)
{
    std::list<std::unique_ptr<WikipediaInfoBox>> lst_boxes;
    QRegularExpression re_box_syntax("{{\\s*Infobox\\s+", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it_matches = re_box_syntax.globalMatch(strContent);
    while (it_matches.hasNext()) {
        QRegularExpressionMatch cl_match = it_matches.next();
        int i_start_of_content;
        i_start_of_content = cl_match.capturedEnd(0);
        
        // Look for for the matching closing pair of "}}". Watch out for possible nested {{}} lists...
        QStringList lst_items;
        int i_open_count = 1, i_link_open_count = 0;
        for ( int i_current_char = i_start_of_content; i_current_char < strContent.size()-1; ++i_current_char )
        {
            if ( strContent[i_current_char] == QChar('{') && strContent[i_current_char+1] == QChar('{') )
                i_open_count++;
            else if ( strContent[i_current_char] == QChar('}') && strContent[i_current_char+1] == QChar('}') )
                i_open_count--;
            else if ( strContent[i_current_char] == QChar('[') && strContent[i_current_char+1] == QChar('[') )
                i_link_open_count++;
            else if ( strContent[i_current_char] == QChar(']') && strContent[i_current_char+1] == QChar(']') )
                i_link_open_count--;
            if ( i_open_count == 0 )
            {
                int i_length = i_current_char-i_start_of_content-1;
                if ( i_length > 0 )
                    lst_items << QStringRef( &strContent, i_start_of_content, i_length ).toString();
                break;
            }
            if ( strContent[i_current_char] == QChar('|') && i_open_count == 1 && i_link_open_count < 1 ) // we encountered a content separator in the box' context
            {
                int i_length = i_current_char-i_start_of_content-1;
                if ( i_length > 0 )
                    lst_items << QStringRef( &strContent, i_start_of_content, i_length ).toString();
                i_start_of_content = i_current_char+1;
            }
        }
        if ( lst_items.empty() ) // no content (?!) or didn't find matching closing brackets
            continue;
        
        QString str_type = lst_items.front().simplified().toLower();
        auto pcl_box = WikipediaInfoBox::createForType( str_type );
        if ( pcl_box )
        {
            lst_items.pop_front();
            pcl_box->fill(strTitle,lst_items);
            lst_boxes.emplace_back( std::move(pcl_box) );
        }
    }
    return lst_boxes;
}

void WikipediaParser::parseWikiText( QString strTitle, QString strContent, QStringList& lstCoverImages, QStringList& lstRedirectTitles )
{    
    // check if content is a simple redirect
    if ( strContent.startsWith( "#REDIRECT", Qt::CaseInsensitive ) || strContent.startsWith("#WEITERLEITUNG", Qt::CaseInsensitive) )
    {
        for ( QString& str_link : WikipediaInfoBox::parseLinkLists( strContent ) )
            lstRedirectTitles << WikipediaInfoBox::getLinkPartOfLink( str_link );
        if ( !lstRedirectTitles.isEmpty() )
            m_lruRedirects.insert( strTitle, new QStringList(lstRedirectTitles) );
    }
    
    // remove any HTML comments
    strContent.remove(QRegularExpression("<!--.*-->"));
    
    // split content into sections
    QRegularExpression re_section_heading("[^=]==([^=].*[^=])==[^=]");
    QStringList lst_sections = strContent.split(re_section_heading);
    QRegularExpressionMatchIterator it_matches = re_section_heading.globalMatch(strContent);
    QStringList lst_headings;
    while (it_matches.hasNext()) {
        QRegularExpressionMatch cl_match = it_matches.next();
        QString str_heading = cl_match.captured(1);
        lst_headings << str_heading;
    }
    // there must always be exactly one more section than headings (the text before the first heading match).
    // we assign this one an empty heading
    lst_headings.push_front("");
    
    std::unique_ptr<SectionsToInfo> map_parsed_infos = std::make_unique<SectionsToInfo>();

    // now headings and sections match and can be iterated in sync.
    auto it_heading = lst_headings.begin();
    auto it_section = lst_sections.begin();
    for ( int i = 0; i < lst_sections.size(); ++i, ++it_heading, ++it_section )
    {
        std::list<std::shared_ptr<OnlineInfoSource>> lst_infos;
        if ( matchesDiscography(strTitle) ) // handle discrography pages different than content pages
        {
            for ( const QString& str_album_title : { m_strAlbumTitle, m_strTrackTitle } )
            {
                auto pcl_discography_info = SingleOrAlbumInDiscographyAsSource::find( str_album_title, *it_section );
                if ( pcl_discography_info )
                {
                    pcl_discography_info->fill( lemma2URL(strTitle,*it_heading), m_strTrackArtist );
                    lst_infos.emplace_back( std::dynamic_pointer_cast<OnlineInfoSource,SingleOrAlbumInDiscographyAsSource>( std::move(pcl_discography_info)) );
                }
            }
            emit info( QString("found %1 discography entries on page %2, section %3").arg(lst_infos.size()).arg(strTitle,*it_heading) );
        }
        else
        {
            auto lst_boxes = getInfoBoxes( lemma2URL(strTitle,*it_heading), *it_section );              
            emit info( QString("found %1 boxes on page %2, section %3").arg(lst_boxes.size()).arg(strTitle,*it_heading) );
            for ( auto & pcl_box : lst_boxes )
            {
                lst_infos.emplace_back( std::dynamic_pointer_cast<OnlineInfoSource,WikipediaInfoBox>( std::move(pcl_box) ) );
                auto pcl_album_box = std::dynamic_pointer_cast<WikipediaAlbumInfoBox>(lst_infos.back());
                if ( pcl_album_box && !pcl_album_box->getCover().isEmpty() )
                    lstCoverImages << pcl_album_box->getCover();
            }
        }
        int i_counter = 0;
        QString str_entry = strTitle;
        if ( !it_heading->isEmpty() ) 
            str_entry.append( " - "+*it_heading );
        
        for ( auto & pcl_info : lst_infos )
            (*map_parsed_infos)[ (lst_infos.size() == 1) ? str_entry : (str_entry + " ("+QString::number(++i_counter) + ")") ] = std::move(pcl_info);
    }
    
    // insert parsed infos into current info table
    m_mapParsedInfos.insert( map_parsed_infos->begin(), map_parsed_infos->end() );
    
    // and insert parsed infos into LRU
    m_lruContent.insert( QUrl::toPercentEncoding(strTitle), map_parsed_infos.release() );
}

QString WikipediaParser::lemma2URL(QString strLemma, QString strSection) const
{
    
    QString str_url = strLemma.replace( QRegularExpression("\\s"), "_" ).prepend( QString("https://%1.wikipedia.org/wiki/").arg(m_strLanguageSubDomain) );
    if ( !strSection.isEmpty() )
        str_url.append( "#"+strSection.replace( QRegularExpression("\\s"), "_" ) );
    return str_url;
}

QStringList WikipediaParser::getPages() const
{
    // get pages, sorted by significance
    std::vector<std::pair<int,QString>> lst_pages_with_significance;
    lst_pages_with_significance.reserve( m_mapParsedInfos.size() );
    for ( const auto & rcl_item : m_mapParsedInfos )
        lst_pages_with_significance.emplace_back( rcl_item.second->significance( m_strAlbumTitle, m_strTrackArtist, m_strTrackTitle, m_iYear ), rcl_item.first );
    std::sort( lst_pages_with_significance.begin(), lst_pages_with_significance.end() );
    QStringList lst_pages;
    for ( auto it_item = lst_pages_with_significance.rbegin(); it_item != lst_pages_with_significance.rend(); ++it_item )
        lst_pages << it_item->second;
    return lst_pages;
}

std::shared_ptr<OnlineInfoSource> WikipediaParser::getResult(const QString &strPage) const
{
    auto it_item = m_mapParsedInfos.find(strPage);
    if ( it_item != m_mapParsedInfos.end() )
        return it_item->second;
    else
        return nullptr;
}

const QIcon &WikipediaParser::getIcon() const
{
    return *m_pclIcon;
}


EnglishWikipediaParser::EnglishWikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent)
: WikipediaParser(pclNetworkAccess, pclParent)
{
    m_strLanguageSubDomain = "en";
}

QStringList EnglishWikipediaParser::createTitleRequests(const QStringList &lstArtists, const QString &trackTitle, const QString &albumTitle)
{
    // check wikipedia for artist page, album page and if possible even title page
    QStringList lst_titles;
    for ( const QString& str_artist : lstArtists )
        lst_titles << str_artist
                   << (str_artist+" (band)")
                   << (str_artist+" (musician)")
                   << (str_artist+" (singer)")
                   << (str_artist+" (artist)")
                   << (str_artist+" (group)")
                   << (str_artist+" discography")
                   << (str_artist+" albums discography")
                   << (str_artist+" singles discography");
    if ( !trackTitle.isEmpty() )
    {
        lst_titles << trackTitle
                   << trackTitle+" (song)";
        for ( const QString& str_artist : lstArtists )
            lst_titles << (trackTitle+" ("+str_artist+" song)");
    }
    if ( !albumTitle.isEmpty() )
    {
        lst_titles << albumTitle
                   << (albumTitle+" (album)")
                   << (albumTitle+" (EP)");
        for ( const QString& str_artist : lstArtists )
            lst_titles << (albumTitle+" ("+str_artist+" album)");
    }
    
    return lst_titles;
}

bool EnglishWikipediaParser::matchesDiscography(const QString &strTitle) const
{
    return strTitle.endsWith( " discography", Qt::CaseInsensitive );
}

QPixmap EnglishWikipediaParser::overlayLanguageHint(QPixmap clFavicon) const
{
    return clFavicon;
}

GermanWikipediaParser::GermanWikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent)
: WikipediaParser(pclNetworkAccess, pclParent)
{
    m_strLanguageSubDomain = "de";
}

QStringList GermanWikipediaParser::createTitleRequests(const QStringList &lstArtists, const QString &trackTitle, const QString &albumTitle)
{
    // check wikipedia for artist page, album page and if possible even title page
    QStringList lst_titles;
    for ( const QString& str_artist : lstArtists )
        lst_titles << str_artist
                   << (str_artist+" (Band)")
                   << (str_artist+" (Musiker)")
                   << (str_artist+" (Sänger)")
                   << (str_artist+" (Künstler)")
                   << (str_artist+"/Diskografie");
    if ( !trackTitle.isEmpty() )
    {
        lst_titles << trackTitle
                   << trackTitle+" (Song)"
                   << trackTitle+" (Lied)";
        for ( const QString& str_artist : lstArtists )
            lst_titles << (trackTitle+" ("+str_artist+" Song)");
    }
    if ( !albumTitle.isEmpty() )
    {
        lst_titles << albumTitle
                   << (albumTitle+" (Album)")
                   << (albumTitle+" (EP)");
        for ( const QString& str_artist : lstArtists )
            lst_titles << (albumTitle+" ("+str_artist+" Album)");
    }
    
    return lst_titles;
}

bool GermanWikipediaParser::matchesDiscography(const QString &strTitle) const
{
    return strTitle.endsWith( "/Diskografie", Qt::CaseInsensitive );
}

QPixmap GermanWikipediaParser::overlayLanguageHint(QPixmap clIcon) const
{
    QRgb black = qRgb(0, 0, 0), red = qRgb(0xFF, 0, 0), gold = qRgb(0xFF, 0xCC, 0);
    
    QSizeF  cl_stripe_size( clIcon.width() / 2.f, clIcon.width() / 2.f / 6.f );
    QPointF cl_left_top( clIcon.width()-cl_stripe_size.width(), clIcon.height()-3*cl_stripe_size.height() );
    QPointF cl_stripe_offset( 0, cl_stripe_size.height() );
    
    QPainter cl_painter( &clIcon );
    cl_painter.setBrush( QBrush( black ) );
    cl_painter.drawRect( QRectF( cl_left_top, cl_stripe_size ) );
    cl_painter.setBrush( QBrush( red ) );
    cl_painter.drawRect( QRectF( cl_left_top+cl_stripe_offset, cl_stripe_size ) );
    cl_painter.setBrush( QBrush( gold ) );
    cl_painter.drawRect( QRectF( cl_left_top+2*cl_stripe_offset, cl_stripe_size ) );
    return clIcon;
}
