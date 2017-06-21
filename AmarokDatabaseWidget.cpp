#include "AmarokDatabaseWidget.h"
#include <QMessageBox>
#include "EmbeddedSQLConnection.h"
#include "StringDistance.h"
#include "ui_AmarokDatabaseWidget.h"

enum { OriginalFieldValue = Qt::UserRole, ItemOrderValue = Qt::UserRole+1, ItemArtistValue = Qt::UserRole+2, ItemYearValue = Qt::UserRole+3, ItemGenreValue = Qt::UserRole+4 };

class OrderedQListWidgetItem : public QListWidgetItem
{
public:
    bool operator<(const QListWidgetItem &rclOther) const override {
        bool b_success;
        double f_this_order = data(ItemOrderValue).toDouble( &b_success );
        if ( b_success )
        {
            double f_other_order = rclOther.data(ItemOrderValue).toDouble( &b_success );
            if ( b_success )
            {
                if ( f_this_order < f_other_order )
                    return true;
                else if ( f_this_order > f_other_order )
                    return false;
            }
        }
        return QListWidgetItem::operator<(rclOther);
    }
};


AmarokDatabaseWidget::AmarokDatabaseWidget(QWidget *pclParent)
: QWidget(pclParent)
, m_pclUI(std::make_unique<Ui::AmarokDatabaseWidget>() )
{
    m_pclUI->setupUi(this);
    connect( m_pclUI->genreEdit,  SIGNAL(textChanged(const QString&)), this, SLOT(orderGenres(const QString&)) );
    connect( m_pclUI->artistEdit, SIGNAL(textChanged(const QString&)), this, SLOT(orderArtists(const QString&)) );
    connect( m_pclUI->albumEdit,  SIGNAL(textChanged(const QString&)), this, SLOT(orderAlbums(const QString&)) );
    connect( m_pclUI->titleEdit,  SIGNAL(textChanged(const QString&)), this, SLOT(orderTitles(const QString&)) );
    
    connect( m_pclUI->genreList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(applyGenre(QListWidgetItem*)) );
    connect( m_pclUI->albumList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(applyAlbum(QListWidgetItem*)) );
    connect( m_pclUI->artistList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(applyArtist(QListWidgetItem*)) );
    connect( m_pclUI->titleList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(applyTitle(QListWidgetItem*)) );
    
    connect( m_pclUI->titleArtistFilterCheck, SIGNAL(stateChanged(int)), this, SLOT(titleFilterChanged()), Qt::QueuedConnection );
    connect( m_pclUI->genreArtistFilterCheck, SIGNAL(stateChanged(int)), this, SLOT(genreFilterChanged()), Qt::QueuedConnection );
}

AmarokDatabaseWidget::~AmarokDatabaseWidget() = default;


void AmarokDatabaseWidget::setDatabaseConnection(std::shared_ptr<EmbeddedSQLConnection> pclDB)
{
    m_pclDB = std::move(pclDB);
    if ( m_pclDB )
    {
        connect( m_pclDB.get(), SIGNAL(connected()), this, SLOT(connectedToDB()) );
        if ( m_pclDB->isConnected() )
            connectedToDB();
    }
}

void AmarokDatabaseWidget::setArtistQuery(const QString &strArtist)
{
    m_pclUI->artistEdit->setText(strArtist);
}

void AmarokDatabaseWidget::setAlbumQuery(const QString &strAlbum)
{
    m_pclUI->albumEdit->setText(strAlbum);
}

void AmarokDatabaseWidget::setGenreQuery(const QString &strGenre)
{
    m_pclUI->genreEdit->setText(strGenre);
}

void AmarokDatabaseWidget::setTitleQuery(const QString &strGenre)
{
    m_pclUI->titleEdit->setText(strGenre);
}

static std::unique_ptr<QListWidgetItem> entryWithCount( const std::pair<QString,int>&  rcl_entry )
{
    auto pcl_item = std::make_unique<OrderedQListWidgetItem>();
    pcl_item->setData( OriginalFieldValue, rcl_entry.first);
    pcl_item->setData( ItemOrderValue, 1 );
    pcl_item->setText( QString("%1 [%2]").arg(rcl_entry.first).arg(rcl_entry.second) );
    return pcl_item;
}

static std::unique_ptr<QListWidgetItem> entryWithCountAndArtistAndYear( const std::tuple<QString,int,QString,int>& rcl_entry )
{
    auto pcl_item = std::make_unique<OrderedQListWidgetItem>();
    pcl_item->setData( OriginalFieldValue, std::get<0>(rcl_entry));
    pcl_item->setData( ItemOrderValue, 1 );
    pcl_item->setData( ItemArtistValue, std::get<2>(rcl_entry) );
    pcl_item->setData( ItemYearValue, std::get<3>(rcl_entry) );
    if ( std::get<2>(rcl_entry).isEmpty() ) //it's a compilation
        pcl_item->setText( QString("%1 (Compilation in %3) [%2]").arg(std::get<0>(rcl_entry)).arg(std::get<1>(rcl_entry)).arg(std::get<3>(rcl_entry)) );
    else
        pcl_item->setText( QString("%1 (by %2 in %4) [%3]").arg(std::get<0>(rcl_entry),std::get<2>(rcl_entry)).arg(std::get<1>(rcl_entry)).arg(std::get<3>(rcl_entry)) );
    return pcl_item;
}

static std::unique_ptr<QListWidgetItem> entryWithArtistAndAlbumAndYearAndGenre( const std::tuple<QString,QString,QString,int,QString>& rcl_entry )
{
    auto pcl_item = std::make_unique<OrderedQListWidgetItem>();
    pcl_item->setData( OriginalFieldValue, std::get<0>(rcl_entry));
    pcl_item->setData( ItemOrderValue, 1 );
    pcl_item->setData( ItemArtistValue, std::get<1>(rcl_entry) );
    pcl_item->setData( ItemYearValue, std::get<3>(rcl_entry) );
    pcl_item->setData( ItemGenreValue, std::get<4>(rcl_entry) );
    pcl_item->setText( QString("%1 (by %2 on %3 in %5) [%4]").arg(std::get<0>(rcl_entry),std::get<1>(rcl_entry),std::get<2>(rcl_entry),std::get<4>(rcl_entry)).arg(std::get<3>(rcl_entry)) );
    return pcl_item;
}

void AmarokDatabaseWidget::connectedToDB()
{
    QSignalBlocker cl_block_genres(m_pclUI->genreList);
    QSignalBlocker cl_block_artists(m_pclUI->artistList);
    QSignalBlocker cl_block_albums(m_pclUI->albumList);
    QSignalBlocker cl_block_titles(m_pclUI->titleList);
    
    // get table entries and fill lists
    m_pclUI->genreList->clear();
    QStringList lst_genres;
    for ( const auto& rcl_genre_count : getWithCount("genre") )
    {
        m_pclUI->genreList->addItem( entryWithCount(rcl_genre_count).release() );
        lst_genres << rcl_genre_count.first;
    }
    
    m_pclUI->artistList->clear();
    for ( const auto& rcl_artist_count : getWithCount("artist") )
        m_pclUI->artistList->addItem( entryWithCount(rcl_artist_count).release() );
    
    m_pclUI->albumList->clear();
    for ( const auto& rcl_album_count_artist_year : getAlbumsWithCountAndArtistAndYear() )
        m_pclUI->albumList->addItem( entryWithCountAndArtistAndYear(rcl_album_count_artist_year).release() );
    
    m_pclUI->titleList->clear();
    for ( const auto& rcl_title_artist_album_year_genre : getTitlesWithArtistsAndAlbumsAndYearAndGenre() )
        m_pclUI->titleList->addItem( entryWithArtistAndAlbumAndYearAndGenre(rcl_title_artist_album_year_genre).release() );
    
    emit genresChanged(lst_genres);
}

static void computeOrderForList( QListWidget& rclList, const QString &strQuery )
{
    if ( strQuery.isEmpty() ) //take the easy way out. no need to do heavy compuations
    {
        #pragma omp parallel for
        for ( int i = 0; i < rclList.count(); ++i )
        {
            rclList.item(i)->setData( ItemOrderValue, 1 );
        }
    }
    else
    {
        // iterate through all entries and compute the order based on string distance (in parallel)
        StringDistance cl_query( strQuery, StringDistance::CaseInsensitive );
        #pragma omp parallel for
        for ( int i = 0; i < rclList.count(); ++i )
        {
            QListWidgetItem* pcl_item = rclList.item(i);
            pcl_item->setData( ItemOrderValue, cl_query.NormalizedLevenshtein( pcl_item->data( OriginalFieldValue ).toString() ) );
        }
    }
}

static bool selectAndShowExactMatch( QListWidget& rclList )
{
    rclList.clearSelection();
    for ( int i = rclList.count()-1; i >= 0; --i )
    {
        // look for matches with distance "0" (which must be an exact match, as no more editing was required
        QListWidgetItem* pcl_item = rclList.item(i);
        if ( pcl_item->data( ItemOrderValue ).toDouble() == 0 )
        {
            pcl_item->setSelected(true);
            rclList.scrollToItem( pcl_item );
            return true;
        }
    }
    return false;
}

void AmarokDatabaseWidget::setExactMatchIcon( QWidget* pclTab, bool bMatch )
{
    int i_tab_idx = m_pclUI->tabWidget->indexOf(pclTab);
    if ( bMatch )
        m_pclUI->tabWidget->setTabIcon( i_tab_idx, QIcon::fromTheme("emblem-favorite") );
    else
        m_pclUI->tabWidget->setTabIcon( i_tab_idx, QIcon() );
}

void AmarokDatabaseWidget::orderGenres(const QString &strGenre)
{
    computeOrderForList( *m_pclUI->genreList, strGenre );
    m_pclUI->genreList->sortItems();
    
    bool b_exact_match = selectAndShowExactMatch( *m_pclUI->genreList );
    setExactMatchIcon( m_pclUI->genreTab, b_exact_match );
}

void AmarokDatabaseWidget::orderArtists(const QString &strArtist)
{
    computeOrderForList( *m_pclUI->artistList, strArtist );
    m_pclUI->artistList->sortItems();
    
    bool b_exact_match = selectAndShowExactMatch( *m_pclUI->artistList );
    setExactMatchIcon( m_pclUI->artistTab, b_exact_match );
    
    // could be that we need to requery
    if ( m_pclUI->titleArtistFilterCheck->isChecked() )
        titleFilterChanged();
    // could be that we need to requery
    if ( m_pclUI->genreArtistFilterCheck->isChecked() )
        genreFilterChanged();
}

void AmarokDatabaseWidget::orderAlbums(const QString &strAlbum)
{
    computeOrderForList( *m_pclUI->albumList, strAlbum );
    m_pclUI->albumList->sortItems();
    bool b_exact_match = selectAndShowExactMatch( *m_pclUI->albumList );
    setExactMatchIcon( m_pclUI->albumTab, b_exact_match );
}

void AmarokDatabaseWidget::orderTitles(const QString &strTitle)
{
    computeOrderForList( *m_pclUI->titleList, strTitle );
    m_pclUI->titleList->sortItems();
    bool b_exact_match = selectAndShowExactMatch( *m_pclUI->titleList );
    setExactMatchIcon( m_pclUI->titleTab, b_exact_match );
}

void AmarokDatabaseWidget::applyGenre(QListWidgetItem* pclItem)
{
    emit setGenre( pclItem->data( OriginalFieldValue ).toString() );
}

void AmarokDatabaseWidget::applyArtist(QListWidgetItem* pclItem)
{
    emit setTrackArtist( pclItem->data( OriginalFieldValue ).toString() );
}

void AmarokDatabaseWidget::applyTitle(QListWidgetItem *pclItem)
{
    emit setTitle( pclItem->data( OriginalFieldValue ).toString() );
    emit setTrackArtist( pclItem->data( ItemArtistValue ).toString() );
    emit setYear( pclItem->data( ItemYearValue ).toInt() );
    emit setGenre( pclItem->data( ItemGenreValue ).toString() );
}

void AmarokDatabaseWidget::titleFilterChanged()
{
    QSignalBlocker cl_block_titles(m_pclUI->titleList);
    m_pclUI->titleList->clear();
    for ( const auto& rcl_title_artist_album_year_genre : getTitlesWithArtistsAndAlbumsAndYearAndGenre(m_pclUI->artistEdit->text()) )
        m_pclUI->titleList->addItem( entryWithArtistAndAlbumAndYearAndGenre(rcl_title_artist_album_year_genre).release() );
    orderTitles( m_pclUI->titleEdit->text() );
}

void AmarokDatabaseWidget::genreFilterChanged()
{
    QSignalBlocker cl_block_titles(m_pclUI->genreList);
    m_pclUI->genreList->clear();
    for ( const auto& rcl_genre_count : getWithCount("genre",m_pclUI->artistEdit->text()) )
        m_pclUI->genreList->addItem( entryWithCount(rcl_genre_count).release() );
    orderGenres( m_pclUI->genreEdit->text() );
}

void AmarokDatabaseWidget::applyAlbum(QListWidgetItem* pclItem)
{
    emit setAlbum( pclItem->data( OriginalFieldValue ).toString() );
    emit setAlbumArtist( pclItem->data( ItemArtistValue ).toString() );
    emit setYear( pclItem->data( ItemYearValue ).toInt() );
}

std::vector<std::pair<QString,int>> AmarokDatabaseWidget::getWithCount( const QString& strField, const QString& strArtistFilter ) const
{
    if ( m_pclUI->genreArtistFilterCheck->isChecked() && strArtistFilter.isEmpty() )
        return {};
    if ( m_pclDB && m_pclDB->isConnected() )
    {
        QString str_query = "SELECT %1s.name, count(*) FROM %1s";
        bool b_with_artist_filter = m_pclUI->genreArtistFilterCheck->isChecked() && !strArtistFilter.isEmpty();
        if ( b_with_artist_filter )
            str_query.append( " INNER JOIN (SELECT tracks.id as id, tracks.%1 as %1 FROM tracks INNER JOIN (SELECT id FROM artists WHERE name=\"%2\") as artists ON tracks.artist=artists.id) AS" );
        else
            str_query.append( " LEFT JOIN");
        str_query.append( " tracks ON %1s.id=tracks.%1 GROUP BY %1s.id ORDER BY %1s.name" );
        std::vector<std::vector<QString>> vec_rows = m_pclDB->query( b_with_artist_filter ? str_query.arg(strField,strArtistFilter) : str_query.arg(strField) );
        
        std::vector<std::pair<QString,int>> vec_results; vec_results.reserve(vec_rows.size());
        for ( std::vector<QString>& vec_cols : vec_rows )
            vec_results.emplace_back( std::move(vec_cols[0]), vec_cols[1].toInt() );
        return vec_results;
    }
    else
        return {};
}

std::vector<std::tuple<QString, int, QString, int> > AmarokDatabaseWidget::getAlbumsWithCountAndArtistAndYear() const
{
    if ( m_pclDB && m_pclDB->isConnected() )
    {
        std::vector<std::vector<QString>> vec_rows = m_pclDB->query( "SELECT album.name, album.num_tracks, album.artist, years.name FROM ( SELECT albums.name AS name, COUNT(tracks.id) AS num_tracks, artists.name AS artist, MAX(tracks.year) AS year FROM (albums LEFT JOIN tracks ON albums.id=tracks.album) LEFT JOIN artists ON artists.id=albums.artist GROUP BY albums.id ORDER BY albums.name ) AS album LEFT JOIN years ON album.year=years.id" );
        std::vector<std::tuple<QString,int,QString,int> > vec_results; vec_results.reserve(vec_rows.size());
        for ( std::vector<QString>& vec_cols : vec_rows )
            vec_results.emplace_back( std::move(vec_cols[0]), vec_cols[1].toInt(), std::move(vec_cols[2]), vec_cols[3].toInt() );
        return vec_results;
    }
    else
        return {};
}

std::vector<std::tuple<QString, QString, QString, int, QString> > AmarokDatabaseWidget::getTitlesWithArtistsAndAlbumsAndYearAndGenre( const QString& strArtistFilter ) const
{
    if ( m_pclUI->titleArtistFilterCheck->isChecked() && strArtistFilter.isEmpty() )
        return {};
    if ( m_pclDB && m_pclDB->isConnected() )
    {
        QString str_query = "SELECT tracks.title, artists.name, albums.name, years.name, genres.name FROM ( ( (tracks LEFT JOIN (SELECT albums.id as id, albums.name as name, artists.name as artist FROM albums LEFT JOIN artists ON albums.artist=artists.id ) AS albums ON tracks.album=albums.id) LEFT JOIN artists ON tracks.artist=artists.id ) LEFT JOIN years ON tracks.year=years.id ) LEFT JOIN genres ON tracks.genre=genres.id";
        if ( m_pclUI->titleArtistFilterCheck->isChecked() && !strArtistFilter.isEmpty() )
            str_query.append( QString(" WHERE artists.name=\"%1\" OR albums.artist=\"%1\"").arg(strArtistFilter) );
        str_query.append( " ORDER BY tracks.title" );
        std::vector<std::vector<QString>> vec_rows = m_pclDB->query( str_query );
        std::vector<std::tuple<QString, QString, QString, int, QString>> vec_results; vec_results.reserve(vec_rows.size());
        for ( std::vector<QString>& vec_cols : vec_rows )
            vec_results.emplace_back( std::move(vec_cols[0]), std::move(vec_cols[1] ), std::move(vec_cols[2]), vec_cols[3].toInt(), std::move(vec_cols[4]) );
        return vec_results;
    }
    else
        return {};
}
