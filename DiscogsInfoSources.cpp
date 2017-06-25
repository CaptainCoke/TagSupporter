#include "DiscogsInfoSources.h"

#include <QTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace {
    enum { DiscIndex = 0, TrackIndex = 1, LengthIndex = 2 };
}

std::unique_ptr<DiscogsInfoSource> DiscogsInfoSource::createForType(const QString &strType, const QJsonDocument &rclDoc)
{
    if ( !rclDoc.isObject() )
        return nullptr;
    
    std::unique_ptr<DiscogsInfoSource> pcl_source;
    
    if ( DiscogsArtistInfo::matchedTypes().contains(strType, Qt::CaseInsensitive) )
        pcl_source = std::make_unique<DiscogsArtistInfo>();
    else if ( DiscogsAlbumInfo::matchedTypes().contains(strType, Qt::CaseInsensitive) )
        pcl_source = std::make_unique<DiscogsAlbumInfo>();
    if ( pcl_source )
        pcl_source->setValues(rclDoc.object());
    
    return pcl_source;
}

void DiscogsInfoSource::setValues(const QJsonObject &rclDoc)
{
    for ( const QJsonValue& rcl_genre : rclDoc["styles"].toArray() )
        m_lstGenres << rcl_genre.toString();
    for ( const QJsonValue& rcl_genre : rclDoc["genres"].toArray() )
        m_lstGenres << rcl_genre.toString();
    m_lstGenres.removeDuplicates();
    m_strURL = rclDoc["uri"].toString();
    m_iId = rclDoc["id"].toInt();
    m_iDataQuality = rclDoc["data_quality"].toString().compare( "Correct", Qt::CaseInsensitive ) == 0 ? 2 : 1;
}

QStringList DiscogsArtistInfo::matchedTypes()
{
    return ( QStringList() << "artists" );
}

int DiscogsArtistInfo::significance(const QString &, const QString &strTrackArtist, const QString &, int) const
{
    int i_significance = std::max(s_iMaxTolerableMatchingDifference - matchArtist(strTrackArtist),0);
    i_significance += m_iDataQuality*(!m_lstGenres.empty());
    return i_significance;
}

bool DiscogsArtistInfo::perfectMatch(const QString &strAlbumTitle, const QString &strTrackArtist, const QString &strTrackTitle, int iMaxDistance) const
{
    return strAlbumTitle.isEmpty() && strTrackTitle.isEmpty() && 
            ( strTrackArtist.isEmpty() || matchArtist(strTrackArtist) <= iMaxDistance );
}



void DiscogsArtistInfo::setValues(const QJsonObject &rclDoc)
{
    DiscogsInfoSource::setValues(rclDoc);
    m_strArtist = rclDoc["name"].toString();
}


QString DiscogsAlbumInfo::m_strEmpty;

QStringList DiscogsAlbumInfo::matchedTypes()
{
    return ( QStringList() << "releases" << "masters" );
}

int DiscogsAlbumInfo::significance(const QString &strAlbumTitle, const QString &strTrackArtist, const QString &strTrackTitle, int iYear ) const
{
    int i_significance = std::max(s_iMaxTolerableMatchingDifference - matchAlbum( strAlbumTitle ),0);
    i_significance += std::max(s_iMaxTolerableMatchingDifference - matchArtist(strTrackArtist),0);
    i_significance += std::max(s_iMaxTolerableMatchingDifference - matchTrackTitle(strTrackTitle),0);
    i_significance += m_iDataQuality*(!m_lstGenres.empty()+!m_strCover.isEmpty()+!m_strYear.isEmpty());
    i_significance += 2*std::max(0,(s_iMaxTolerableYearDifference-std::abs(m_strYear.toInt()-iYear)));
    return i_significance;
}

void DiscogsAlbumInfo::setCover(QString strCover)
{
    m_strCover = std::move(strCover);
}

bool DiscogsAlbumInfo::perfectMatch(const QString &strAlbumTitle, const QString &strTrackArtist, const QString &strTrackTitle, int iMaxDistance) const
{
    return ( strAlbumTitle.isEmpty() || matchAlbum(strAlbumTitle) <= iMaxDistance ) 
            && ( strTrackArtist.isEmpty() || matchArtist(strTrackArtist) <= iMaxDistance )
            && ( strTrackTitle.isEmpty() || matchTrackTitle(strTrackTitle) <= iMaxDistance );
}

const QString& DiscogsAlbumInfo::getTitle(size_t uiIndex) const
{
    if ( uiIndex < static_cast<size_t>(m_lstTitles.size()) )
        return m_lstTitles.at( uiIndex );
    else
        return m_strEmpty;
}

const QString& DiscogsAlbumInfo::getArtist(size_t uiIndex) const
{
    if ( m_lstArtists.empty() )
        return m_strAlbumArtist;
    if ( uiIndex < static_cast<size_t>(m_lstArtists.size()) )
        return m_lstArtists.at( static_cast<int>(uiIndex) );
    else
        return m_strEmpty;
}

size_t DiscogsAlbumInfo::getNumTracks() const
{
    return std::max( m_vecDiscTrackLength.size(), std::max<size_t>( m_lstArtists.size(), m_lstTitles.size() ) );
}

size_t DiscogsAlbumInfo::getDisc(size_t uiIndex) const
{
    if ( uiIndex < m_vecDiscTrackLength.size() )
        return std::get<DiscIndex>(m_vecDiscTrackLength.at(uiIndex));
    else
        return 0;
}

size_t DiscogsAlbumInfo::getTrack(size_t uiIndex) const
{
    if ( uiIndex < m_vecDiscTrackLength.size() )
        return std::get<TrackIndex>(m_vecDiscTrackLength.at(uiIndex));
    else
        return 0;
}

size_t DiscogsAlbumInfo::getTrackLength(size_t uiIndex) const
{
    if ( uiIndex < m_vecDiscTrackLength.size() )
        return std::get<LengthIndex>(m_vecDiscTrackLength.at(uiIndex));
    else
        return 0;
}

QString DiscogsAlbumInfo::title() const
{
    if ( !m_strAlbumArtist.isEmpty() )
        return QString( "Album: %1 (%2, by %3)" ).arg( m_lstAlbums.front(), m_strYear, m_strAlbumArtist );
    else
        return QString( "Compilation: %1 (%2)" ).arg( m_lstAlbums.front(), m_strYear );
}

/* To uniquely identify artists, discogs sometimes appends a number in brackets to the name...
 * This function removes that number in case it exists...
 */
static QString stripUniqueArtistNumbers( QString strArtist )
{
    if ( strArtist.endsWith(QChar(')')) )
        strArtist.remove( QRegExp( "\\([0-9]+\\)$" )  );
    return strArtist.trimmed();
}

static QString getFirstArtistFromList( const QJsonArray& rclArtistArray )
{
    QStringList lst_artists;
    for ( const QJsonValue& rcl_artist_info : rclArtistArray)
        lst_artists << stripUniqueArtistNumbers( rcl_artist_info.toObject()["name"].toString() );
    lst_artists.removeDuplicates();
    if ( !lst_artists.isEmpty() )
        return lst_artists.front();
    return QString();
}

static QString joinFirstArtistsFromList( const QJsonArray& rclArtistArray )
{
    QStringList lst_artists;
    for ( const QJsonValue& rcl_artist_info : rclArtistArray)
    {
        lst_artists << stripUniqueArtistNumbers( rcl_artist_info.toObject()["name"].toString() );
        QString str_join = rcl_artist_info.toObject()["join"].toString().toLower();
        if ( str_join.compare(",") == 0 )
            break;
        lst_artists << str_join;
    }
    return lst_artists.join(" ");
}

static std::tuple<size_t,size_t,size_t> getDiscAndTrack( const QString& strPosition )
{
    std::tuple<size_t,size_t,size_t> cl_result = {1,0,0};
    //try to parse position
    QStringList lst_parts = strPosition.split( "-" );
    if ( lst_parts.size() >  1 )
        std::get<DiscIndex>( cl_result ) = lst_parts.front().toInt();
    std::get<TrackIndex>( cl_result )= lst_parts.back().toInt();
    
    return cl_result;
}

static size_t stringToDuration( const QString& strDuration )
{
    QTime cl_duration = QTime::fromString( strDuration, "m:ss" );
    return static_cast<size_t>(QTime(0,0).secsTo( cl_duration ));
}

void DiscogsAlbumInfo::setValues(const QJsonObject &rclDoc)
{
    DiscogsInfoSource::setValues(rclDoc);
    
    m_lstAlbums << rclDoc["title"].toString();
    QJsonValue cl_year = rclDoc["year"];
    if ( cl_year.isDouble() )
    {
        int i_year = cl_year.toInt(-1);
        if ( i_year > 0 )
            m_strYear = QString::number(i_year);
    }
    else if ( cl_year.isString() )
        m_strYear = cl_year.toString();
    
    m_strAlbumArtist = joinFirstArtistsFromList( rclDoc["artists"].toArray() );
    if ( m_strAlbumArtist.startsWith("Various", Qt::CaseInsensitive) )
        m_strAlbumArtist.clear();
    
    for ( const QJsonValue& rcl_tracks_info : rclDoc["tracklist"].toArray() )
    {
        QJsonObject cl_track = rcl_tracks_info.toObject();
        m_lstTitles  << cl_track["title"].toString();
        QJsonValue cl_track_artists = cl_track["artists"];
        if ( cl_track_artists.isArray() )
            m_lstArtists << joinFirstArtistsFromList( cl_track_artists.toArray() );
        m_vecDiscTrackLength.emplace_back( getDiscAndTrack( cl_track["position"].toString() ) );
        std::get<LengthIndex>(m_vecDiscTrackLength.back()) = stringToDuration(cl_track["duration"].toString());
    }
}
