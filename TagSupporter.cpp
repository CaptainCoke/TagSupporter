#include "TagSupporter.h"
#include <QMessageBox>
#include <QDir>
#include <QNetworkAccessManager>
#include <Tools/EmbeddedSQLConnection.h>
#include <OnlineParsers/WikipediaParser.h>
#include <OnlineParsers/DiscogsParser.h>
#include <Tools/CoverDownloader.h>
#include <Tools/TemporaryRecursiveCopy.h>
#include "ui_TagSupporter.h"

TagSupporter::TagSupporter(QWidget *parent)
: QMainWindow(parent)
, m_pclUI( std::make_unique<Ui::TagSupporter>() )
, m_pclNetworkAccess( std::make_unique<QNetworkAccessManager>() )
, m_pclDB( std::make_shared<EmbeddedSQLConnection>() )
{
    m_pclEnglishWikipediaParser = std::make_shared<EnglishWikipediaParser>(m_pclNetworkAccess.get());
    m_pclGermanWikipediaParser  = std::make_shared<GermanWikipediaParser>(m_pclNetworkAccess.get());
    m_pclDiscogsParser          = std::make_shared<DiscogsParser>(m_pclNetworkAccess.get());
    m_pclUI->setupUi(this);
    
    connect( m_pclUI->fileBrowserWidget, &FileBrowserWidget::noFileSelected, this, &TagSupporter::noFileSelected );
    connect( m_pclUI->fileBrowserWidget, &FileBrowserWidget::fileSelected, this, &TagSupporter::fileSelected );
    connect( m_pclUI->fileBrowserWidget, &FileBrowserWidget::folderChanged, m_pclUI->filenameWidget, &FilenameWidget::setDestinationBaseDirectory );
    connect( m_pclUI->fileBrowserWidget, &FileBrowserWidget::saveFile, this, &TagSupporter::saveFile );

    connect( m_pclUI->onlineSourcesWidget, SIGNAL(sourceURLchanged(QUrl)), m_pclUI->webBrowserWidget, SLOT(showURL(QUrl)));
    connect( m_pclUI->metadataWidget, SIGNAL(searchCoverOnline(QUrl)), m_pclUI->webBrowserWidget, SLOT(showURL(QUrl)));
    
    connect( m_pclUI->metadataWidget, SIGNAL(trackArtistChanged(const QString &)), m_pclUI->amarokDatabaseWidget, SLOT(setArtistQuery(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(albumChanged(const QString &)), m_pclUI->amarokDatabaseWidget, SLOT(setAlbumQuery(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(genreChanged(const QString &)), m_pclUI->amarokDatabaseWidget, SLOT(setGenreQuery(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(titleChanged(const QString &)), m_pclUI->amarokDatabaseWidget, SLOT(setTitleQuery(const QString &)) );
    
    connect( m_pclUI->metadataWidget, SIGNAL(trackArtistChanged(const QString &)), m_pclUI->onlineSourcesWidget, SLOT(setArtistQuery(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(albumChanged(const QString &)), m_pclUI->onlineSourcesWidget, SLOT(setAlbumQuery(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(titleChanged(const QString &)), m_pclUI->onlineSourcesWidget, SLOT(setTitleQuery(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(yearChanged(int)), m_pclUI->onlineSourcesWidget, SLOT(setYearQuery(int)) );
    
    connect( m_pclUI->playbackWidget, SIGNAL(durationChanged(int)), m_pclUI->onlineSourcesWidget, SLOT(setLengthQuery(int)) );
    
    connect( m_pclUI->metadataWidget, SIGNAL(trackArtistChanged(const QString &)), m_pclUI->filenameWidget, SLOT(setTrackArtist(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(albumArtistChanged(const QString &)), m_pclUI->filenameWidget, SLOT(setAlbumArtist(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(albumChanged(const QString &)), m_pclUI->filenameWidget, SLOT(setAlbum(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(titleChanged(const QString &)), m_pclUI->filenameWidget, SLOT(setTitle(const QString &)) );
    connect( m_pclUI->metadataWidget, SIGNAL(trackNumberChanged(int)), m_pclUI->filenameWidget, SLOT(setTrackNumber(int)) );
    
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setTrackArtist(const QString&)), m_pclUI->metadataWidget, SLOT(setTrackArtist(const QString&)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setAlbumArtist(const QString&)), m_pclUI->metadataWidget, SLOT(setAlbumArtist(const QString&)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setGenre(const QString&)), m_pclUI->metadataWidget, SLOT(setGenre(const QString&)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setCover(const QPixmap&)), m_pclUI->metadataWidget, SLOT(setCover(const QPixmap&)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setYear(int)), m_pclUI->metadataWidget, SLOT(setYear(int)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setAlbum(const QString&)), m_pclUI->metadataWidget, SLOT(setAlbum(const QString&)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setDiscNumber(const QString&)), m_pclUI->metadataWidget, SLOT(setDiscNumber(const QString&)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setTrackTitle(const QString&)), m_pclUI->metadataWidget, SLOT(setTrackTitle(const QString&)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setTotalTracks(int)), m_pclUI->metadataWidget, SLOT(setTotalTracks(int)));
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(setTrackNumber(int)), m_pclUI->metadataWidget, SLOT(setTrackNumber(int)));
    
    connect( m_pclUI->webBrowserWidget, SIGNAL(setCover(const QPixmap&)), m_pclUI->metadataWidget, SLOT(setCover(const QPixmap&)));
    connect( m_pclUI->webBrowserWidget, SIGNAL(parseURL(const QUrl&)), m_pclEnglishWikipediaParser.get(), SLOT(parseFromURL(const QUrl&)));
    connect( m_pclUI->webBrowserWidget, SIGNAL(parseURL(const QUrl&)), m_pclGermanWikipediaParser.get(), SLOT(parseFromURL(const QUrl&)));
    connect( m_pclUI->webBrowserWidget, SIGNAL(parseURL(const QUrl&)), m_pclDiscogsParser.get(), SLOT(parseFromURL(const QUrl&)));
    
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(setTrackArtist(const QString&)), m_pclUI->metadataWidget, SLOT(setTrackArtist(const QString&)));
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(setAlbumArtist(const QString&)), m_pclUI->metadataWidget, SLOT(setAlbumArtist(const QString&)));
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(setGenre(const QString&)), m_pclUI->metadataWidget, SLOT(setGenre(const QString&)));
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(setYear(int)), m_pclUI->metadataWidget, SLOT(setYear(int)));
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(setAlbum(const QString&)), m_pclUI->metadataWidget, SLOT(setAlbum(const QString&)));
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(setTitle(const QString&)), m_pclUI->metadataWidget, SLOT(setTrackTitle(const QString&)));
    
    connect( m_pclUI->metadataWidget, SIGNAL(metadataModified()), m_pclUI->fileBrowserWidget, SLOT(setFileModified()) );
    connect( m_pclUI->filenameWidget, SIGNAL(filenameModified()), m_pclUI->fileBrowserWidget, SLOT(setFileModified()) );
   
    connect( m_pclEnglishWikipediaParser.get(), SIGNAL(error(QString)), this, SLOT(infoParsingError(QString)), Qt::QueuedConnection );
    connect( m_pclEnglishWikipediaParser.get(), SIGNAL(info(QString)), this, SLOT(infoParsingInfo(QString)), Qt::QueuedConnection );
    connect( m_pclGermanWikipediaParser.get(), SIGNAL(error(QString)), this, SLOT(infoParsingError(QString)), Qt::QueuedConnection );
    connect( m_pclGermanWikipediaParser.get(), SIGNAL(info(QString)), this, SLOT(infoParsingInfo(QString)), Qt::QueuedConnection );
    connect( m_pclDiscogsParser.get(), SIGNAL(error(QString)), this, SLOT(infoParsingError(QString)), Qt::QueuedConnection );
    connect( m_pclDiscogsParser.get(), SIGNAL(info(QString)), this, SLOT(infoParsingInfo(QString)), Qt::QueuedConnection );
    connect( m_pclUI->metadataWidget, SIGNAL(error(QString)), this, SLOT(metadataError(QString)), Qt::QueuedConnection );
    m_pclUI->onlineSourcesWidget->addParser("English Wikipedia", m_pclEnglishWikipediaParser);
    m_pclUI->onlineSourcesWidget->addParser("German Wikipedia", m_pclGermanWikipediaParser);
    m_pclUI->onlineSourcesWidget->addParser("Discogs", m_pclDiscogsParser);
    
    m_pclUI->filenameWidget->addFilenameFormat( "Single (Artist - Track)", "%A - %T" );
    m_pclUI->filenameWidget->addFilenameFormat( "Album (# - Track)", "%N - %T" );
    m_pclUI->filenameWidget->addFilenameFormat( "Compilation (# - Artist - Track)", "%N - %A - %T" );
    m_pclUI->filenameWidget->addDestinationDirectoryFormat( "Single (Artist)", "%C" );
    m_pclUI->filenameWidget->addDestinationDirectoryFormat( "Album (Artist/Album)", "%C/%B" );
    m_pclUI->filenameWidget->addDestinationDirectoryFormat( "Compilation (Album)", "%B" );
    
    m_pclUI->amarokDatabaseWidget->setDatabaseConnection( m_pclDB );
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(genresChanged(const QStringList&)), m_pclUI->metadataWidget, SLOT(setGenreList(const QStringList&)) );
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(genresChanged(const QStringList&)), m_pclUI->onlineSourcesWidget, SLOT(setGenreList(const QStringList&)) );
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(closestArtistsChanged(QStringList)), m_pclUI->metadataWidget, SLOT(setClosestArtists(const QStringList&)) );
    
    connect( m_pclDB.get(), SIGNAL(error(QString)), this, SLOT(databaseError(QString)), Qt::QueuedConnection );
    try
    {
        m_pclDB->connectToExternalDB();
    }
    catch ( const std::exception& rclExc )
    {
        QMessageBox::critical( this, "MySQL error", rclExc.what() );
    }
}

TagSupporter::~TagSupporter()
{
    // be sure to close the MySQL database before deleting the temporary files
    m_pclDB.reset();
    m_lstTemporaryFiles.clear();
}

void TagSupporter::noFileSelected()
{
    m_pclUI->onlineSourcesWidget->clear();
    m_pclUI->metadataWidget->clear();
    m_pclUI->filenameWidget->clear();
}

void TagSupporter::fileSelected(const QString& strFullFilePath)
{
    m_pclUI->playbackWidget->setSource(QUrl::fromLocalFile(strFullFilePath));
    m_pclUI->onlineSourcesWidget->clear();
    m_pclUI->metadataWidget->clear();
    m_pclUI->filenameWidget->clear();
    m_pclUI->metadataWidget->loadFromFile(strFullFilePath);
    m_pclUI->filenameWidget->setFilename(strFullFilePath);
    m_pclUI->onlineSourcesWidget->check();
}

void TagSupporter::infoParsingError(QString strError)
{
    QMessageBox::critical( this, "Parsing Error", strError );
}

void TagSupporter::infoParsingInfo(QString strInfo)
{
    m_pclUI->statusBar->showMessage("Parsing Info: " + strInfo);
}

void TagSupporter::databaseError(QString strError)
{
    QMessageBox::critical( this, "Database Error", strError );
}

void TagSupporter::metadataError(QString strError)
{
    QMessageBox::critical( this, "Metadata Error", strError );
}

QString TagSupporter::getLastUsedFolder() const
{
    return m_pclUI->fileBrowserWidget->getLastUsedFolder();
}

void TagSupporter::scanFolder( QString strFolder )
{
    m_pclUI->fileBrowserWidget->scanFolder( std::move(strFolder) );
}

void TagSupporter::saveFile( const QString& strFullFilePath )
{
    if ( m_pclUI->metadataWidget->isModified() )
        if ( !m_pclUI->metadataWidget->saveToFile( strFullFilePath ) )
            return; // abort here, if saving the metadata was aborted
    
    if ( m_pclUI->filenameWidget->isModified() )
    {
        if ( !m_pclUI->filenameWidget->applyToFile( strFullFilePath ) )
            throw std::runtime_error( "renaming failed" );

        QString str_new_folder;
        if ( !m_pclUI->filenameWidget->destinationSubdirectory().isEmpty() )
            str_new_folder = m_pclUI->filenameWidget->destinationBaseDirectory() + "/" + m_pclUI->filenameWidget->destinationSubdirectory();

        m_pclUI->fileBrowserWidget->currentFileMoved(m_pclUI->filenameWidget->filename(),str_new_folder);
    }
    m_pclUI->fileBrowserWidget->setFileModified( m_pclUI->metadataWidget->isModified() || m_pclUI->filenameWidget->isModified() );   
}
