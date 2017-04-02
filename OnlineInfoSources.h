#ifndef ONLINEINFOSOURCES_H
#define ONLINEINFOSOURCES_H

#include <cstddef>

class QString;
class QStringList;

class OnlineInfoSource {
public:
    virtual ~OnlineInfoSource() = default;
    virtual const QString& getURL() const = 0;
    virtual int significance() const = 0;
};

class OnlineArtistInfoSource : public virtual OnlineInfoSource {
public:
    virtual const QString&     getArtist() const = 0;
    virtual const QStringList& getGenres() const = 0;
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
};

#endif // ONLINEINFOSOURCES_H
