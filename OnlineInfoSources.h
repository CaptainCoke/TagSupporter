#ifndef ONLINEINFOSOURCES_H
#define ONLINEINFOSOURCES_H

#include <cstddef>

class QString;
class QStringList;

class OnlineInfoSource {
public:
    virtual ~OnlineInfoSource() = default;
    virtual const QString& getURL() const = 0;
    // obtain the significance of this source for a given combination of album, (track) artist and title
    virtual int significance( const QString &strAlbumTitle, const QString &strTrackArtist, const QString &strTrackTitle ) const = 0;
};

class OnlineArtistInfoSource : public virtual OnlineInfoSource {
public:
    virtual const QString&     getArtist() const = 0;
    virtual const QStringList& getGenres() const = 0;
    
    virtual int matchArtist( const QString& strArtist ) const;
};

class OnlineAlbumInfoSource : public virtual OnlineInfoSource {
public:
    virtual const QStringList& getGenres() const = 0;
    virtual const QString&     getAlbumArtist() const = 0;
    virtual const QString&     getCover()       const = 0;
    virtual const QStringList& getAlbums()      const = 0;
    virtual const QString&     getYear()        const = 0;
    virtual size_t             getNumTracks()   const = 0;
    virtual size_t             getDisc(size_t uiIndex)   const = 0;
    virtual size_t             getTrack(size_t uiIndex)  const = 0;
    virtual const QString&     getArtist(size_t uiIndex) const = 0;
    virtual const QString&     getTitle(size_t uiIndex)  const = 0;
    
    virtual int matchArtist( const QString& strArtist ) const;
    virtual int matchAlbum( const QString& strAlbum ) const;
    virtual int matchTrackTitle( const QString& strTitle ) const;
};

#endif // ONLINEINFOSOURCES_H
