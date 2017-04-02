#ifndef DISCOGSINFOSOURCES_H
#define DISCOGSINFOSOURCES_H

#include "OnlineInfoSources.h"
#include <memory>
#include <vector>
#include <QStringList>

class QJsonObject;
class QJsonDocument;

class DiscogsInfoSource : public virtual OnlineInfoSource
{
public:
    DiscogsInfoSource() = default;
    ~DiscogsInfoSource() override = default;
    
    const QString& getURL() const override { return m_strURL; }
    virtual QString title() const = 0;
    virtual int id() const { return m_iId; }
    
    static std::unique_ptr<DiscogsInfoSource> createForType( const QString& strType, const QJsonDocument& rclDoc );
protected:
    virtual void setValues( const QJsonObject& rclDoc );
    
    QString     m_strURL;
    QStringList m_lstGenres;
    int         m_iDataQuality;
    int         m_iId;
};

class DiscogsArtistInfo : public DiscogsInfoSource, public virtual OnlineArtistInfoSource
{
public:
    ~DiscogsArtistInfo() override = default;
    const QString&     getArtist() const override { return m_strArtist; }
    const QStringList& getGenres() const override { return m_lstGenres; }
    QString title() const override { return "Artist: " + m_strArtist; }
    
    static QStringList matchedTypes();
    int significance() const override;
protected:
    void setValues( const QJsonObject& rclDoc ) override;
    QString m_strArtist;
};

class DiscogsAlbumInfo : public DiscogsInfoSource, public virtual OnlineAlbumInfoSource
{
public:
    ~DiscogsAlbumInfo() override = default;
    
    const QStringList& getGenres() const override       { return m_lstGenres; }
    const QString&     getAlbumArtist()  const override { return m_strAlbumArtist; }
    const QString&     getCover()        const override { return m_strCover; }
    const QStringList& getAlbums()       const override { return m_lstAlbums; }
    const QString&     getYear()         const override { return m_strYear; }
    const QString&     getTitle(size_t)  const override;
    const QString&     getArtist(size_t) const override;
    size_t             getNumTracks()    const override;
    size_t             getDisc(size_t)   const override;
    size_t             getTrack(size_t)  const override;
    QString title() const override;
    
    static QStringList matchedTypes();
    int significance() const override;
    
    void setCover( QString strCover );
protected:
    void setValues( const QJsonObject& rclDoc ) override;
    
    static QString m_strEmpty;
    QString m_strCover, m_strYear, m_strAlbumArtist;
    QStringList m_lstAlbums, m_lstTitles, m_lstArtists;
    std::vector<std::pair<size_t,size_t>> m_vecDiscTrack;
};

#endif // DISCOGSINFOSOURCES_H
