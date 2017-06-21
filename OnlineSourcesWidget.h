#ifndef ONLINESOURCESWIDGET_H
#define ONLINESOURCESWIDGET_H

#include <QWidget>
#include <memory>

namespace Ui {
    class OnlineSourcesWidget;
}

class OnlineSourceParser;

class OnlineSourcesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OnlineSourcesWidget(QWidget *pclParent = nullptr);
    ~OnlineSourcesWidget() override;
    
    void addParser( const QString& strName, std::shared_ptr<OnlineSourceParser> pclParser );
    
public slots:
    void setGenreList( const QStringList& lstGenres );
    
    void setArtistQuery( const QString& strArtist );
    void setAlbumQuery( const QString& strAlbum );
    void setTitleQuery( const QString& strTitle );
    void setYearQuery( int iYear );
    void setLengthQuery( int iTrackLength );
    
    void clear();
    void check();
    
signals:
    void setTrackArtist( const QString& );
    void setAlbumArtist( const QString& );
    void setGenre( const QString& );
    void setCover( const QPixmap& );
    void setYear( int );
    void setAlbum( const QString& );
    void setDiscNumber( const QString& );
    void setTotalTracks( int );
    void setTrackNumber( int );
    void setTrackTitle( const QString& );
    
    void sourceURLchanged( const QUrl& );
    
protected slots:
    void coverDownloadError(QString);
    void setCoverImageFromDownloader();
    void setTrackArtistForTrack(class QListWidgetItem* pclItem);
    
    void addParsingResults(QStringList lstNewPages);
    void showOnlineSource(int);
    
    void applyTrackArtist();
    void applyAlbumArtist();
    void applyGenre();
    void applyCover();
    void applyYear();
    void applyAlbum();
    void applyTrack();
    void applyAll();
    
protected:
    void enableParser(const QString& strName, bool bEnabled);
    void fillArtistInfos( std::shared_ptr<class OnlineArtistInfoSource> pclSource );
    void fillAlbumInfos( std::shared_ptr<class OnlineAlbumInfoSource> pclSource );
    
    /// attempts to figure out if genres in list use an alternate spelling and add a corrected entry
    void spellCorrectGenres();
    
    /// highlights known genres, returns the index of the first known genre or 0
    int highlightKnownGenres();
    int highlightMatchingTitles();
    void clearFields();

    
private:
    std::unique_ptr<Ui::OnlineSourcesWidget>              m_pclUI;
    std::unique_ptr<class QNetworkAccessManager>          m_pclNetworkAccess;
    std::unique_ptr<class CoverDownloader>                m_pclCoverDownloader;
    std::map<QString,std::shared_ptr<OnlineSourceParser>> m_mapParsers;
    std::map<QString,bool>                                m_mapParserEnabled;
    QString m_strArtist, m_strAlbum, m_strTrackTitle;
    int     m_iYear = -1, m_iTrackLength = -1;
    QStringList m_lstGenres;
};

#endif // ONLINESOURCESWIDGET_H
