#include "DiscogsInfoSources.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "StringDistance.h"


bool DiscogsInfoSource::matchesArtist(const QString &, int ) const
{
    return true;
}

bool DiscogsInfoSource::matchesAlbum(const QString &, int ) const
{
    return true;
}

bool DiscogsInfoSource::matchesTrackTitle(const QString &, int ) const
{
    return true;
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

int DiscogsArtistInfo::significance() const
{
    return m_iDataQuality*(!m_strArtist.isEmpty() + m_lstGenres.size());
}

bool DiscogsArtistInfo::matchesArtist(const QString &strArtist, int iMaxDistance) const
{
    if ( strArtist.isEmpty() )
        return true;
    else
        return StringDistance(strArtist, StringDistance::CaseInsensitive).Levenshtein( m_strArtist ) <= iMaxDistance;
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

int DiscogsAlbumInfo::significance() const
{
    return m_iDataQuality*(!m_strAlbumArtist.isEmpty() + m_lstGenres.size()+!m_strCover.isEmpty()+!m_strYear.isEmpty()+m_lstAlbums.size())+getNumTracks();
}

void DiscogsAlbumInfo::setCover(QString strCover)
{
    m_strCover = std::move(strCover);
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
    return std::max( m_vecDiscTrack.size(), std::max<size_t>( m_lstArtists.size(), m_lstTitles.size() ) );
}

size_t DiscogsAlbumInfo::getDisc(size_t uiIndex) const
{
    if ( uiIndex < m_vecDiscTrack.size() )
        return m_vecDiscTrack.at(uiIndex).first;
    else
        return 0;
}

size_t DiscogsAlbumInfo::getTrack(size_t uiIndex) const
{
    if ( uiIndex < m_vecDiscTrack.size() )
        return m_vecDiscTrack.at(uiIndex).second;
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

static QString getFirstArtistFromList( const QJsonArray& rclArtistArray )
{
    QStringList lst_artists;
    for ( const QJsonValue& rcl_artist_info : rclArtistArray)
        lst_artists << rcl_artist_info.toObject()["name"].toString();
    lst_artists.removeDuplicates();
    if ( !lst_artists.isEmpty() )
        return lst_artists.front();
    return QString();
}

static std::pair<size_t,size_t> getDiscAndTrack( const QString& strPosition )
{
    std::pair<size_t,size_t> cl_result = {1,0};
    //try to parse position
    QStringList lst_parts = strPosition.split( "-" );
    if ( lst_parts.size() >  1 )
        cl_result.first = lst_parts.front().toInt();
    cl_result.second = lst_parts.back().toInt();
    
    return cl_result;
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
    
    m_strAlbumArtist = getFirstArtistFromList( rclDoc["artists"].toArray() );
    if ( m_strAlbumArtist.startsWith("Various", Qt::CaseInsensitive) )
        m_strAlbumArtist.clear();
    
    for ( const QJsonValue& rcl_tracks_info : rclDoc["tracklist"].toArray() )
    {
        QJsonObject cl_track = rcl_tracks_info.toObject();
        m_lstTitles  << cl_track["title"].toString();
        QJsonValue cl_track_artists = cl_track["artists"];
        if ( cl_track_artists.isArray() )
            m_lstArtists << getFirstArtistFromList( cl_track_artists.toArray() );
        m_vecDiscTrack.emplace_back( getDiscAndTrack( cl_track["position"].toString() ) );
    }
}

bool DiscogsAlbumInfo::matchesArtist(const QString &strArtist, int iMaxDistance) const
{
    if ( strArtist.isEmpty() )
        return true;
    StringDistance cl_query(strArtist, StringDistance::CaseInsensitive);
    if ( cl_query.Levenshtein( m_strAlbumArtist ) <= iMaxDistance )
        return true;
    // otherwise try all track artists...
    for ( const QString& str_artist : m_lstArtists )
        if ( cl_query.Levenshtein( str_artist ) <= iMaxDistance )
            return true;
    return false;
}

bool DiscogsAlbumInfo::matchesAlbum(const QString &strAlbum, int iMaxDistance) const
{
    if ( strAlbum.isEmpty() )
        return true;
    StringDistance cl_query(strAlbum, StringDistance::CaseInsensitive);
    for ( const QString& str_album : m_lstAlbums )
        if ( cl_query.Levenshtein( str_album ) <= iMaxDistance )
            return true;
    return false;
}

bool DiscogsAlbumInfo::matchesTrackTitle(const QString &strTitle, int iMaxDistance) const
{
    if ( strTitle.isEmpty() )
        return true;
    StringDistance cl_query(strTitle, StringDistance::CaseInsensitive);
    for ( const QString& str_title : m_lstTitles )
        if ( cl_query.Levenshtein( str_title ) <= iMaxDistance )
            return true;
    return false;
}
