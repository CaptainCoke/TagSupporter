#include "WikipediaInfoSources.h"
#include <QRegularExpressionMatchIterator>

std::unique_ptr<WikipediaInfoBox> WikipediaInfoBox::createForType(const QString &strType)
{
    if ( WikipediaArtistInfoBox::matchedTypes().contains(strType, Qt::CaseInsensitive) )
        return std::make_unique<WikipediaArtistInfoBox>();
    if ( WikipediaAlbumInfoBox::matchedTypes().contains(strType, Qt::CaseInsensitive) )
        return std::make_unique<WikipediaAlbumInfoBox>();
    else
        return nullptr;
}

QStringList WikipediaInfoBox::parseLinkLists( const QString& strLinkLists )
{
    QStringList lst_links;
    
    QRegularExpression re_link("\\[\\[([^\\]]*)\\]\\]",QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it_matches = re_link.globalMatch(strLinkLists);
    while (it_matches.hasNext()) {
        QRegularExpressionMatch cl_match = it_matches.next();
        lst_links << cl_match.captured(0);
    }
    return lst_links;
}

QString WikipediaInfoBox::getTextPartOfLink( QString strText )
{
    strText.replace( QRegularExpression("\\[{2}|\\]{2}"), "");
    return strText.split("|").back();
}

QString WikipediaInfoBox::getLinkPartOfLink( QString strText )
{
    strText.replace( QRegularExpression("\\[{2}|\\]{2}"), "");
    return strText.split("|").front();
}

void WikipediaInfoBox::fill( const QString& strURL, const QStringList& lstAttributes )
{
    m_strURL = strURL;
    for ( const QString & str_item : lstAttributes )
    {
        QStringList lst_attrib = str_item.split("=");
        QString str_key = lst_attrib.front().simplified().toLower();
        QString str_val = lst_attrib.back().simplified();
        setValue( str_key, str_val );
    }
}

void WikipediaArtistInfoBox::setValue( const QString& strKey, const QString& strValue )
{
    if ( strKey == "name" )
    {
        m_strArtist = getTextPartOfLink( strValue );
    }
    else if ( strKey == "genre" ) 
    {
        //genres are supposed to be in wikilinks. parse text for links
        m_lstGenres.clear();
        for ( QString& strLink : parseLinkLists( strValue ) )
            m_lstGenres << getTextPartOfLink(std::move(strLink));
    }
}

QStringList WikipediaArtistInfoBox::matchedTypes()
{
    return ( QStringList() << "musical artist" << "band" );
}

static QString parseYearFromDate( const QString& strDateString )
{
    QString str_year;
    
    auto fun_use_earlier_year = [&str_year]( const QString& strOtherYear )
    {
        if ( str_year.isEmpty() )
            str_year = strOtherYear;
        else if ( strOtherYear.compare( str_year ) < 0 ) // found another year thats even earlier
            str_year = strOtherYear;
    };
    
    // either in Start-date format (https://en.wikipedia.org/wiki/Template:Start_date)
    QRegularExpression cl_regexp("{{\\s*start\\s+date\\s*[|](\\s*df=yes\\s*[|])*\\s*(\\d+)\\s*([|][^}]*)*}}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it_startdate_matches = cl_regexp.globalMatch(strDateString);
    while (it_startdate_matches.hasNext())
        fun_use_earlier_year( it_startdate_matches.next().captured(2) );
    
    if ( str_year.isEmpty() ) // if no start-date format matches, it's free text
    {
        //it probably contains a four digit year
        QRegularExpressionMatchIterator it_year_matches = QRegularExpression("\\d{4}").globalMatch(strDateString);
        while (it_year_matches.hasNext())
            fun_use_earlier_year( it_year_matches.next().captured(0) );
    }
    return str_year;
}

QString WikipediaAlbumInfoBox::m_strEmpty;

void WikipediaAlbumInfoBox::setValue( const QString& strKey, const QString& strValue )
{
    if ( strKey == "cover" )
    {
        m_strCover = strValue;
    }
    else if ( strKey == "released" || strKey == "published" || strKey == "jahr" || strKey == "veröffentlichung" )
    {
        m_strYear = parseYearFromDate( strValue );
    }
    else if ( strKey == "name" || strKey == "album" || strKey == "ep" || strKey == "titel" )
    {
        m_lstAlbums << getTextPartOfLink( strValue );
    }
    else if ( strKey == "artist" || strKey == "künstler" || strKey == "musiker" )
    {
        WikipediaArtistInfoBox::setValue("name",strValue);
    }
    else
        WikipediaArtistInfoBox::setValue(strKey,strValue);
}

void WikipediaAlbumInfoBox::setCover(QString strCover)
{
    m_strCover = std::move(strCover);
}

QStringList WikipediaAlbumInfoBox::matchedTypes()
{
    return ( QStringList() << "album" << "single" << "song" << "musikalbum" );
}


std::unique_ptr<SingleOrAlbumInDiscographyAsSource> SingleOrAlbumInDiscographyAsSource::find(const QString& strAlbum, const QString &strDiscographyPageContent)
{
    // full-text search for album in discography (case insensitive)
    int i_album_start = strDiscographyPageContent.indexOf( strAlbum, 0, Qt::CaseInsensitive );
    if ( i_album_start < 0 )
        return nullptr;
    
    auto pcl_source = std::make_unique<SingleOrAlbumInDiscographyAsSource>();
    pcl_source->m_lstAlbums << strAlbum;
    
    // search backwards from position of album name to beginning of row
    int i_row_start = i_album_start;
    for ( ; i_row_start > 1; --i_row_start )
    {
        QStringRef str_row( &strDiscographyPageContent, i_row_start-1, i_album_start-i_row_start );
        if ( QRegularExpression( "^[|]\\s*-" ).match( str_row ).hasMatch() ) // found start of row!
        {
            pcl_source->m_strYear = parseYearFromDate(str_row.toString());
            break;
        }
    }
    return pcl_source;
}

QStringList SingleOrAlbumInDiscographyAsSource::m_lstEmpty;
QString SingleOrAlbumInDiscographyAsSource::m_strEmpty;

void SingleOrAlbumInDiscographyAsSource::fill(const QString &strURL, const QString &strArtist)
{
    m_strURL    = strURL;
    m_strArtist = strArtist;
}

int WikipediaArtistInfoBox::significance() const
{
    return !m_strArtist.isEmpty() + m_lstGenres.size();
}

int WikipediaAlbumInfoBox::significance() const
{
    return WikipediaArtistInfoBox::significance() + !m_strCover.isEmpty() + !m_strYear.isEmpty() + m_lstAlbums.size();
}

int SingleOrAlbumInDiscographyAsSource::significance() const
{
    return !m_strArtist.isEmpty() + !m_strYear.isEmpty() + m_lstAlbums.size();
}
