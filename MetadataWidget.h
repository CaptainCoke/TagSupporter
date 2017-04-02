#ifndef METADATAWIDGET_H
#define METADATAWIDGET_H

#include <QWidget>
#include <memory>
#include <QStringList>

namespace Ui {
class MetadataWidget;
}

namespace TagLib{
class Tag;
class PropertyMap;
namespace FLAC { class File; }
namespace MPEG { class File; }
}

class MetadataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MetadataWidget(QWidget *pclParent = nullptr);
    ~MetadataWidget() override;
    
    bool isModified() const;

public slots:
    void setGenreList( const QStringList& lstGenres );
    
    void loadFromFile(const QString& strFilename);
    bool saveToFile(const QString& strFilename);
    void clear();
    
    void setTrackArtist( const QString& );
    void setAlbumArtist( const QString& );
    void setGenre( const QString& );
    void setCover( const QPixmap& );
    void setYear( int );
    void setAlbum( const QString& );
    void setTrackTitle( const QString& );
    void setTrackNumber( int );
    void setTotalTracks( int );
    void setDiscNumber( const QString& );

signals:
    //specific signals (e.g. for online source updates)
    void trackArtistChanged( const QString& );
    void albumChanged( const QString& );
    void titleChanged( const QString& );
    void genreChanged( const QString& );
    void trackNumberChanged( int );
    void error( QString );
    void searchCoverOnline( const QUrl& );
    
    //general signal about modifications (e.g. to show that there is something to save)
    void metadataModified();
    
protected slots:
    void clearOtherTags();
    void clearCover();
    void loadCover();
    void setSingleTrack();
    void googleCover();
    
protected:
    void showCover();
    bool checkConsistency();
    
    // returns a list of blockers. As long as the list exists, the signals of all form elements will be blocked (useful e.g. for multiple updates in a row)
    std::list<QSignalBlocker> blockAllFormSignals();
    
    TagLib::PropertyMap setMetadataInPropertyMap( TagLib::PropertyMap mapProperties ) const;
    
    /// returns list of free tags that could not be interpreted
    QStringList parseFreeTagInformation( const TagLib::PropertyMap& mapProps );
    void parseGenericTagInformation( TagLib::Tag* pclTag );
    void parseFile( TagLib::FLAC::File& rclFile );
    void parseFile( TagLib::MPEG::File& rclFile );
    bool applyTags( TagLib::FLAC::File& rclFile );
    bool applyTags( TagLib::MPEG::File& rclFile );
    template<class FileClass>
    bool tryOpenAndParse( const QString& strFilename );
    template<class FileClass>
    bool tryOpenAndSave( const QString& strFilename );
    template<class SetterFun>
    bool setFreeTag( const QString& strTagQuery, const QString& strTag, const QStringList& lstEntries, SetterFun funSetTag );

    template<class PictureType, typename EnumType>
    PictureType* selectFrontCoverFromPictureList( const std::list<PictureType*>& lstPictures, EnumType FrontCoverValue );
private:
    static const QStringList s_lstStandardTags, s_lstExtendedTags; // the taglib "standard" tags and the list of also used extended tags
    
    std::unique_ptr<class QPixmap> m_pclFullResCover; // store the full resolution cover here
    std::unique_ptr<Ui::MetadataWidget> m_pclUI;
    QString m_strFilename;
    bool m_bIsModified{false};
};

#endif // METADATAWIDGET_H
