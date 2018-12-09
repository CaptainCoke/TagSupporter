#ifndef AMAROKDATABASEWIDGET_H
#define AMAROKDATABASEWIDGET_H

#include <QWidget>
#include <memory>

namespace Ui {
class AmarokDatabaseWidget;
}

class EmbeddedSQLConnection;
class QListWidgetItem;

class AmarokDatabaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AmarokDatabaseWidget(QWidget *pclParent = nullptr);
    ~AmarokDatabaseWidget() override;
    
    void setDatabaseConnection( std::shared_ptr<EmbeddedSQLConnection> pclDB );

public slots:
    void setArtistQuery( const QString& strArtist );
    void setAlbumQuery( const QString& strAlbum );
    void setGenreQuery( const QString& strGenre );
    void setTitleQuery( const QString& strGenre );
    
signals:
    void genresChanged(const QStringList&);
    void closestArtistsChanged( QStringList );
    
    void setTrackArtist( const QString& );
    void setAlbumArtist( const QString& );
    void setGenre( const QString& );
    void setAlbum( const QString& );
    void setYear( int iYear );
    void setTitle( const QString& );
    
protected slots:
    void connectedToDB();
    void orderGenres(  const QString& strGenre );
    void orderArtists( const QString& strArtist );
    void orderAlbums(  const QString& strAlbum );
    void orderTitles(  const QString& strTitle );
    
    void applyGenre(QListWidgetItem* pclItem);
    void applyAlbum(QListWidgetItem* pclItem);
    void applyArtist(QListWidgetItem* pclItem);
    void applyTitle(QListWidgetItem* pclItem);
    
    void titleFilterChanged();
    void genreFilterChanged();
    
protected:
    std::vector<std::pair<QString,int>> getWithCount( const QString& strField, const QString& strArtistFilter = QString() ) const;
    std::vector<std::tuple<QString,int,QString,int>> getAlbumsWithCountAndArtistAndYear() const;
    std::vector<std::tuple<QString,QString,QString,int,QString>> getTitlesWithArtistsAndAlbumsAndYearAndGenre( const QString& strArtistFilter = QString() ) const;
    
    void setExactMatchIcon(QWidget *pclTab, bool bMatch);
private:
    std::unique_ptr<Ui::AmarokDatabaseWidget> m_pclUI;
    std::shared_ptr<EmbeddedSQLConnection>    m_pclDB;
};

#endif // AMAROKDATABASEWIDGET_H
