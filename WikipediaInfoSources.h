#ifndef WIKIPEDIAINFOSOURCES_H
#define WIKIPEDIAINFOSOURCES_H

#include <memory>
#include <QStringList>
#include "OnlineInfoSources.h"

// see https://en.wikipedia.org/wiki/Wikipedia:List_of_infoboxes#Music

class WikipediaInfoBox : public virtual OnlineInfoSource
{
public:
    WikipediaInfoBox() = default;
    ~WikipediaInfoBox() override = default;
    void fill( const QString& strURL, const QStringList& lstAttributes );
    const QString& getURL() const override { return m_strURL; }
    
    static std::unique_ptr<WikipediaInfoBox> createForType( const QString& strType);
    static QStringList parseLinkLists( const QString& strLinkLists );
    static QString getTextPartOfLink( QString strLink );
    static QString getLinkPartOfLink( QString strLink );
protected:
    virtual void setValue( const QString& strKey, const QString& strValue ) = 0;
    QString m_strURL;
};

class WikipediaArtistInfoBox : public WikipediaInfoBox, public virtual OnlineArtistInfoSource
{
public:
    ~WikipediaArtistInfoBox() override = default;
    
    const QString&     getArtist() const override { return m_strArtist; }
    const QStringList& getGenres() const override { return m_lstGenres; }
    
    static QStringList matchedTypes();
    
    int significance() const override;
    
protected:
    void setValue( const QString& strKey, const QString& strValue ) override;
    QString     m_strArtist;
    QStringList m_lstGenres;
};

class WikipediaAlbumInfoBox : public WikipediaArtistInfoBox, public virtual OnlineAlbumInfoSource
{
public:
    ~WikipediaAlbumInfoBox() override = default;
    
    const QStringList& getGenres() const override { return WikipediaArtistInfoBox::getGenres(); }
    const QString&     getArtist(size_t) const override { return WikipediaArtistInfoBox::getArtist(); }
    const QStringList& getAlbums() const override { return m_lstAlbums; }
    const QString&     getAlbumArtist() const override { return m_strArtist; }
    const QString& getCover() const override { return m_strCover; }
    const QString& getYear() const override { return m_strYear; }
    const QString& getTitle(size_t) const override { return m_strEmpty; }
    size_t getNumTracks() const override { return 0; }
    size_t getDisc(size_t) const override { return 0; }
    size_t getTrack(size_t) const override { return 0; }
    
    void setCover( QString );
    
    static QStringList matchedTypes();
    
    int significance() const override;
protected:
    void setValue( const QString& strKey, const QString& strValue ) override;
    
    static QString m_strEmpty;
    QString m_strCover, m_strYear;
    QStringList m_lstAlbums;
};

class SingleOrAlbumInDiscographyAsSource : public virtual OnlineAlbumInfoSource
{
public:
    ~SingleOrAlbumInDiscographyAsSource() override = default;
    
    static std::unique_ptr<SingleOrAlbumInDiscographyAsSource> find( const QString& strAlbum, const QString& strDiscographyPageContent );
    void fill( const QString& strURL, const QString& strArtist );
    
    const QString&     getArtist(size_t) const override { return m_strArtist; }
    const QString&     getAlbumArtist() const override { return m_strArtist; }
    const QStringList& getGenres() const override { return m_lstEmpty; }
    const QStringList& getAlbums() const override { return m_lstAlbums; }
    const QString& getCover() const override { return m_strEmpty; }
    const QString& getYear() const override { return m_strYear; }
    const QString& getURL() const override { return m_strURL; }
    const QString& getTitle(size_t) const override { return m_strEmpty; }
    size_t getNumTracks() const override { return 0; }
    size_t getDisc(size_t) const override { return 0; }
    size_t getTrack(size_t) const override { return 0; }
    
    int significance() const override;
protected:
    static QStringList m_lstEmpty;
    static QString m_strEmpty;
    QStringList m_lstAlbums;
    QString m_strYear, m_strURL, m_strArtist;
};

#endif // WIKIPEDIAINFOSOURCES_H
