#include "OnlineInfoSources.h"
#include <QStringList>
#include <Tools/StringDistance.h>

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

static QStringList splitTitleAtBrackets( const QString& strTitle )
{
    return strTitle.split( QRegExp("[\\(\\)\\[\\]]"), QString::SkipEmptyParts );
}

static int matchTrackTitleConsideringBrackets( const StringDistance& rclQuery, const QString &strTitle)
{
    QStringList lst_sub_titles = splitTitleAtBrackets(strTitle);
    if ( lst_sub_titles.size() > 1 )
        lst_sub_titles.push_front(strTitle);
    int i_min_distance = std::numeric_limits<int>::max();
    for ( const QString & str_sub_title : lst_sub_titles )
    {
        int i_distance = rclQuery.Levenshtein(str_sub_title.trimmed());
        if ( i_distance == 0 )
            return 0;
        i_min_distance = std::min( i_distance, i_min_distance );
    }
    return i_min_distance;
}

int OnlineAlbumInfoSource::matchTrackTitlesConsideringBrackets( const QString &strTitle1, const QString &strTitle2 )
{
    int i_min_distance = std::numeric_limits<int>::max();
    for ( const QString & str_sub_title : splitTitleAtBrackets( strTitle1 ) )
    {
        int i_distance = matchTrackTitleConsideringBrackets( StringDistance(str_sub_title, StringDistance::CaseInsensitive), strTitle2 );
        if ( i_distance == 0 )
            return 0;
        i_min_distance = std::min( i_distance, i_min_distance );
    }
    return i_min_distance;
}

int OnlineAlbumInfoSource::matchTrackTitle(const QString &strTitle) const
{
    int i_min_distance = std::numeric_limits<int>::max();
    for ( const QString & str_sub_title : splitTitleAtBrackets( strTitle ) )
    {
        StringDistance cl_query(str_sub_title, StringDistance::CaseInsensitive);
        for ( size_t ui_track = 0; ui_track < getNumTracks(); ++ui_track )
        {
            int i_distance = matchTrackTitleConsideringBrackets( cl_query, getTitle(ui_track) );
            if ( i_distance == 0 )
                return 0;
            i_min_distance = std::min( i_distance, i_min_distance );
        }
    }
    return i_min_distance;
}
