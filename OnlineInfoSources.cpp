#include "OnlineInfoSources.h"
#include "StringDistance.h"
#include <QStringList>

int OnlineArtistInfoSource::matchArtist(const QString &strArtist) const
{
    return StringDistance(getArtist(), StringDistance::CaseInsensitive).Levenshtein( strArtist );
}



int OnlineAlbumInfoSource::matchArtist(const QString &strArtist) const
{
    StringDistance cl_query(strArtist, StringDistance::CaseInsensitive);
    int i_min_distance = cl_query.Levenshtein( getAlbumArtist() );
    for ( size_t ui_track = 0; ui_track < getNumTracks(); ++ui_track )
        i_min_distance = std::min( cl_query.Levenshtein( getArtist(ui_track) ), i_min_distance );
    return i_min_distance;
}

int OnlineAlbumInfoSource::matchAlbum(const QString &strAlbum) const
{
    StringDistance cl_query(strAlbum, StringDistance::CaseInsensitive);
    int i_min_distance = std::numeric_limits<int>::max();
    for ( const QString& str_album : getAlbums() )
        i_min_distance = std::min( cl_query.Levenshtein( str_album ), i_min_distance );
    return i_min_distance;
}

int OnlineAlbumInfoSource::matchTrackTitle(const QString &strTitle) const
{
    StringDistance cl_query(strTitle, StringDistance::CaseInsensitive);
    int i_min_distance = std::numeric_limits<int>::max();
    for ( size_t ui_track = 0; ui_track < getNumTracks(); ++ui_track )
        i_min_distance = std::min( cl_query.Levenshtein( getTitle(ui_track) ), i_min_distance );
    return i_min_distance;
}
