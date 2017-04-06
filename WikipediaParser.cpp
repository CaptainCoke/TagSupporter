#include "WikipediaParser.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpressionMatchIterator>
#include "WikipediaInfoSources.h"

WikipediaParser::WikipediaParser(QNetworkAccessManager *pclNetworkAccess, QObject *pclParent)
: OnlineSourceParser(pclParent)
, m_pclNetworkAccess(pclNetworkAccess)
{
}


WikipediaParser::~WikipediaParser() = default;

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

void WikipediaParser::sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle )
{
    // clear the previous results
    clearResults();
    m_strAlbumTitle  = albumTitle;
    m_strTrackTitle  = trackTitle;
    m_strTrackArtist = trackArtist;
    
    // the track artist could also be combination of artist name (e.g. "feat." or "&" or "with"
    QStringList lst_artists = trackArtist.split( QRegularExpression("\\s(feat\\.|&|and|with|featuring)\\s", QRegularExpression::CaseInsensitiveOption), QString::SkipEmptyParts );
    lst_artists << trackArtist;
    lst_artists.removeDuplicates();
            
    resolveTitleURLs( createTitleRequests( lst_artists, trackTitle, albumTitle ) );
}

void WikipediaParser::clearResults()
{
    m_mapParsedInfos.clear();
    m_lstParsedPages.clear();
    m_strAlbumTitle.clear();
    m_strTrackTitle.clear();
    m_strTrackArtist.clear();
    m_bSearchConducted = false;
    
    //TODO: cancel any pending requests
}

void WikipediaParser::parseFromURL(const QUrl &rclUrl)
{
    // check if URL is of this wikipedia language (or from wikipedia at all)
    if ( !rclUrl.host().startsWith( m_strLanguageSubDomain + ".wikipedia" ) )
        return;
    
    // get the title from the URL
    QString str_title = rclUrl.path().split( "/" ).back();
    QNetworkReply *pcl_reply = m_pclNetworkAccess->get( createContentRequest(QStringList(str_title)) );
    connect(pcl_reply, SIGNAL(finished()), this, SLOT(replyReceived()));
}

void WikipediaParser::replyReceived()
{
    QNetworkReply* pcl_reply = dynamic_cast<QNetworkReply*>( sender() );
    if ( !pcl_reply )
        return;
    
    // check for error
    if ( pcl_reply->error() == QNetworkReply::NoError )
        parseWikipediaAPIJSONReply( pcl_reply->readAll() );
    else
        emit error( QString("Network reply to %1 received error: %2").arg(pcl_reply->url().toString(),pcl_reply->errorString()) );   
    pcl_reply->deleteLater();
}

void WikipediaParser::resolveTitleURLs(QStringList lstTitles)
{
    lstTitles.removeDuplicates();
    for ( QString& str_title : lstTitles )
         str_title = QUrl::toPercentEncoding(str_title);
    if ( !lstTitles.isEmpty() )
    {
        QNetworkReply *pcl_reply = m_pclNetworkAccess->get( createContentRequest(lstTitles) );
        connect(pcl_reply, SIGNAL(finished()), this, SLOT(replyReceived()));
    }
    else
        emit error("unable to query wikipedia without either artist, title or album information");
}

void WikipediaParser::resolveSearchQueries(QStringList lstQueries)
{
    lstQueries.removeDuplicates();
    if ( lstQueries.isEmpty() )
        emit error("unable to search wikipedia without either artist, title or album information");
    for ( const QString& str_query : lstQueries )
    {
        QNetworkReply *pcl_reply = m_pclNetworkAccess->get( createSearchRequest(QUrl::toPercentEncoding(str_query)) );
        connect(pcl_reply, SIGNAL(finished()), this, SLOT(replyReceived()));
    }   
}


void WikipediaParser::resolveCoverImageURLs( QStringList lstCoverImages )
{
    lstCoverImages.removeDuplicates();
    for ( QString& str_title : lstCoverImages )
         str_title = QUrl::toPercentEncoding(str_title);
    if ( !lstCoverImages.isEmpty() )
    {
        QNetworkReply *pcl_reply = m_pclNetworkAccess->get( createImageRequest(lstCoverImages) );
        connect(pcl_reply, SIGNAL(finished()), this, SLOT(replyReceived()));
    }
    else
        emit error("unable to query wikipedia to resolve images without any image titles");
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
    QJsonObject arr_pages         = cl_doc.object()["query"].toObject()["pages"].toObject();
    QJsonArray arr_search_results = cl_doc.object()["query"].toObject()["search"].toArray();
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
                    replaceCoverImageURL( std::move(str_title), std::move(str_url) );
                    continue;
                }
            }
        }
        lst_error_pages << str_title;
    }
    for ( const QJsonValue& rcl_result : arr_search_results )
    {
        QJsonObject cl_result = rcl_result.toObject();
        QString str_title = cl_result["title"].toString();
        if ( !m_lstParsedPages.contains(str_title) )
            lst_redirect_titles << str_title;
    }
    
    if ( lst_redirect_titles.empty() && lst_cover_images.empty() )
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
    if ( !lst_cover_images.empty() )
    {
        // make another call to resolve all cover image URLs at once
        resolveCoverImageURLs(std::move(lst_cover_images));
    }
    if ( !lst_redirect_titles.empty() )
    {
        // make another call to resolve all redirect URLs
        resolveTitleURLs(std::move(lst_redirect_titles));
    }
    // verbose info:
    emit info( QString("found %1 pages: %2\nfailed for %3 pages: %4")
                              .arg(m_lstParsedPages.size()).arg(m_lstParsedPages.join("; "))
                              .arg(lst_error_pages.size()).arg(lst_error_pages.join("; ")) );
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
        int i_open_count = 1;
        for ( int i_current_char = i_start_of_content; i_current_char < strContent.size()-1; ++i_current_char )
        {
            if ( strContent[i_current_char] == QChar('{') && strContent[i_current_char+1] == QChar('{') )
                i_open_count++;
            else if ( strContent[i_current_char] == QChar('}') && strContent[i_current_char+1] == QChar('}') )
                i_open_count--;
            if ( i_open_count == 0 )
            {
                int i_length = i_current_char-i_start_of_content-1;
                if ( i_length > 0 )
                    lst_items << QStringRef( &strContent, i_start_of_content, i_length ).toString();
                break;
            }
            if ( strContent[i_current_char] == QChar('|') && i_open_count == 1 ) // we encountered a content separator in the box' context
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
            m_mapParsedInfos[ (lst_infos.size() == 1) ? str_entry : (str_entry + " ("+QString::number(++i_counter) + ")") ] = std::move(pcl_info);
    }
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
        lst_pages_with_significance.emplace_back( rcl_item.second->significance(), rcl_item.first );
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
                   << (str_artist+" discography");
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
