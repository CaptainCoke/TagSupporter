#include "TagSupporter.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QDir>
#include "EmbeddedSQLConnection.h"
#include "WikipediaParser.h"
#include "DiscogsParser.h"
#include "CoverDownloader.h"
#include "ui_TagSupporter.h"

enum 
{
    MediaSourceDirectory = Qt::UserRole
};

class TemporaryRecursiveCopy
{
public:
    TemporaryRecursiveCopy( const QString& strSrcPath, const QString& strTemporaryBasePath )
    {
        // create temporary base path (if it doesn't exist yet)
        QDir().mkpath(strTemporaryBasePath);
        QDir cl_dst_dir(strTemporaryBasePath);
        if ( !cl_dst_dir.exists() )
            throw std::runtime_error( "failed to create destination directory" );
                
        QStringList lst_files_to_copy;
        QFileInfo cl_src_info( strSrcPath );
        if ( cl_src_info.isDir() )
        {
            QDir cl_src_dir(strSrcPath);
            for ( const QString& str_filename : cl_src_dir.entryList(QDir::Files) )
                lst_files_to_copy << cl_src_dir.filePath( str_filename );
            
            for( const QString & str_dir : cl_src_dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot) )
                m_lstSubDirs.emplace_back( cl_src_dir.filePath( str_dir ), cl_dst_dir.filePath( str_dir ) );
        }
        else if ( cl_src_info.isFile() )
            lst_files_to_copy << strSrcPath;
        
        // copy all files in current level of dir-tree
        for( const QString & str_full_src_path : lst_files_to_copy )
        {
            QString str_full_dst_path = cl_dst_dir.filePath( QFileInfo(str_full_src_path).fileName() );
            if( QFile::copy( str_full_src_path, str_full_dst_path ) )
                m_lstCopiedFiles << str_full_dst_path;
        }
    }
    ~TemporaryRecursiveCopy()
    {
        // erase all temporary files that were created by copying
        for ( const QString & str_full_path : m_lstCopiedFiles )
            QFile::remove( str_full_path );
    }
  
protected:
    QStringList m_lstCopiedFiles;
    std::list<TemporaryRecursiveCopy> m_lstSubDirs;
};

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
    connect( m_pclUI->browseFolderButton, SIGNAL(clicked()), this, SLOT(browseForFolder()) );
    connect( m_pclUI->folderFileList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(switchFile(QListWidgetItem*, QListWidgetItem*)) );
    connect( m_pclUI->folderFileList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* pclItem){switchFile(pclItem,pclItem);} );
    
    connect( m_pclUI->onlineSourcesWidget, SIGNAL(sourceURLchanged(const QUrl&)), m_pclUI->webBrowserWidget, SLOT(showURL(const QUrl&)));
    connect( m_pclUI->metadataWidget, SIGNAL(searchCoverOnline(const QUrl&)), m_pclUI->webBrowserWidget, SLOT(showURL(const QUrl&)));
    connect( m_pclUI->saveButton, SIGNAL(clicked()), this, SLOT(saveCurrent()) );
    connect( m_pclUI->nextButton, SIGNAL(clicked()), this, SLOT(selectNextFile()) );
    connect( m_pclUI->deleteButton, SIGNAL(clicked()), this, SLOT(deleteCurrent()) );
    connect( m_pclUI->refreshButton, &QPushButton::clicked, [this]{scanFolder(m_strLastScannedFolder);} );
    
    
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
    
    connect( m_pclUI->metadataWidget, SIGNAL(metadataModified()), this, SLOT(fileModified()) );
    connect( m_pclUI->filenameWidget, SIGNAL(filenameModified()), this, SLOT(fileModified()) );
   
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
    m_pclUI->filenameWidget->addDestinationDirectoryFormat( "Singe (Artist)", "%C" );
    m_pclUI->filenameWidget->addDestinationDirectoryFormat( "Album (Artist/Album)", "%C/%B" );
    m_pclUI->filenameWidget->addDestinationDirectoryFormat( "Compilation (Album)", "%B" );
    
    m_pclUI->refreshButton->setEnabled(false);
    m_pclUI->saveButton->setEnabled(false);
    m_pclUI->nextButton->setEnabled(false);
    m_pclUI->deleteButton->setEnabled(false);
    
    m_pclUI->amarokDatabaseWidget->setDatabaseConnection( m_pclDB );
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(genresChanged(const QStringList&)), m_pclUI->metadataWidget, SLOT(setGenreList(const QStringList&)) );
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(genresChanged(const QStringList&)), m_pclUI->onlineSourcesWidget, SLOT(setGenreList(const QStringList&)) );
    connect( m_pclUI->amarokDatabaseWidget, SIGNAL(closestArtistsChanged(QStringList)), m_pclUI->metadataWidget, SLOT(setClosestArtists(const QStringList&)) );
    
    connect( m_pclDB.get(), SIGNAL(error(QString)), this, SLOT(databaseError(QString)), Qt::QueuedConnection );
    try
    {
        // make a temporary copy of the Amarok database in order to be able to open it even in case Amarok is currently accessing it
        QDir cl_src_dir( QDir::home().filePath( ".kde/share/apps/amarok" ) );
        QDir cl_dst_dir( QDir::temp().filePath( "TagSupporter" ) );
        m_lstTemporaryFiles.emplace_back( cl_src_dir.filePath("my.cnf"), cl_dst_dir.path() );
        m_lstTemporaryFiles.emplace_back( cl_src_dir.filePath("mysqle"), cl_dst_dir.filePath("mysqle") );
        m_pclDB->openServer(cl_dst_dir.path());
        m_pclDB->connectToDB();
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

void TagSupporter::scanFolder(const QString& strFolder)
{
    m_strLastScannedFolder = strFolder;
    m_pclUI->refreshButton->setEnabled(true);
    m_pclUI->folderFileList->blockSignals(true);
    m_pclUI->folderFileList->clear();

    // list all audio files in this folder...
    QDir cl_dir(strFolder);
    QStringList lst_filters;
    lst_filters << "*.mp3" << "*.ogg" << "*.oga" << "*.flac" << "*.wma" << "*.mp4" << "*.m4a";
    QStringList lst_files = cl_dir.entryList( lst_filters, QDir::Files, QDir::Name | QDir::IgnoreCase | QDir::LocaleAware );
    for ( const QString& strFilename : lst_files )
    {
        QListWidgetItem* pcl_item = new QListWidgetItem(strFilename);
        pcl_item->setData(MediaSourceDirectory,strFolder);
        m_pclUI->folderFileList->addItem( pcl_item );
    }
    m_pclUI->folderFileList->setCurrentRow(0);
    m_pclUI->folderFileList->blockSignals(false);

    updateTotalFileCountLabel();
    
    if ( !lst_files.isEmpty() )
        selectFile(strFolder+"/"+lst_files.front());
    else
    {
        m_pclUI->saveButton->setEnabled(false);
        m_pclUI->deleteButton->setEnabled(false);
        m_pclUI->onlineSourcesWidget->clear();
        m_pclUI->metadataWidget->clear();
        m_pclUI->filenameWidget->clear();
    }
    m_pclUI->nextButton->setEnabled(lst_files.size() > 1);
    m_pclUI->filenameWidget->setDestinationBaseDirectory( strFolder );
}

void TagSupporter::browseForFolder()
{
    QString str_folder = QFileDialog::getExistingDirectory( this, "Select folder", m_strLastScannedFolder );
    if ( !str_folder.isNull() )
        scanFolder(str_folder);
}

void TagSupporter::selectFile(const QString& strFullFilePath)
{
    m_pclUI->saveButton->setEnabled(false);
    m_pclUI->deleteButton->setEnabled(true);
    m_pclUI->playbackWidget->setSource(QUrl::fromLocalFile(strFullFilePath));
    m_pclUI->onlineSourcesWidget->clear();
    m_pclUI->metadataWidget->clear();
    m_pclUI->filenameWidget->clear();
    m_pclUI->metadataWidget->loadFromFile(strFullFilePath);
    m_pclUI->filenameWidget->setFilename(strFullFilePath);
    m_pclUI->onlineSourcesWidget->check();
}

void TagSupporter::saveCurrent()
{
    try
    {     
        auto pcl_item = m_pclUI->folderFileList->currentItem();
        if ( !pcl_item )
            throw std::runtime_error( "no file selected" );
        QString str_full_file_path = pcl_item->data(MediaSourceDirectory).toString()+"/"+pcl_item->text();
        if ( !QFile::exists( str_full_file_path ) )
            throw std::runtime_error( "selected file \""+str_full_file_path.toStdString()+"\" does not exist (any more?)!" );
        
        if ( m_pclUI->metadataWidget->isModified() )
            if ( !m_pclUI->metadataWidget->saveToFile( str_full_file_path ) )
                return; // abort here, if saving the metadata was aborted
        if ( m_pclUI->filenameWidget->isModified() )
        {
            if ( !m_pclUI->filenameWidget->applyToFile( str_full_file_path ) )
                throw std::runtime_error( "renaming failed" );
            
            QString str_new_folder;
            if ( !m_pclUI->filenameWidget->destinationSubdirectory().isEmpty() )
                str_new_folder = m_pclUI->filenameWidget->destinationBaseDirectory() + "/" + m_pclUI->filenameWidget->destinationSubdirectory();
            else
                str_new_folder = pcl_item->data(MediaSourceDirectory).toString();
            // remember to exchange the text in the list with the new filename...
            pcl_item->setText( m_pclUI->filenameWidget->filename() );
            pcl_item->setData( MediaSourceDirectory, str_new_folder );
            // and set the new filename to the player
            m_pclUI->playbackWidget->setSource(QUrl::fromLocalFile(str_new_folder+"/"+pcl_item->text()));
            // and update the file count for the directory
            updateTotalFileCountLabel();
        }
        
        QFont cl_font = pcl_item->font();
        // set font back from bold to normal to indicate: all modification saved.
        cl_font.setBold(m_pclUI->filenameWidget->isModified() || m_pclUI->metadataWidget->isModified());
        // set font to italic to indicate: item visited
        cl_font.setItalic(true);
        pcl_item->setFont(cl_font);
    }
    catch( const std::exception& rclExc )
    {
        QMessageBox::critical( this, "Save Error", QString( "Failed to save all modifications: %1" ).arg(rclExc.what()) );
    }
}

void TagSupporter::updateTotalFileCountLabel()
{
    QDir cl_dir(m_strLastScannedFolder);
    int i_num_considered_files = 0;
    for ( int i_item = 0; i_item < m_pclUI->folderFileList->count(); ++i_item )
    {
        auto pcl_item = m_pclUI->folderFileList->item(i_item);
        if ( pcl_item->data(MediaSourceDirectory).toString().compare( m_strLastScannedFolder ) == 0 )
            ++i_num_considered_files;
    }
    m_pclUI->folderContentLabel->setText( QString("%1 audio files.\n%2 other files (not displayed).").arg( i_num_considered_files ).arg(cl_dir.entryList( {}, QDir::Files).count()-i_num_considered_files) );
    
}

void TagSupporter::deleteCurrent()
{
    try
    {     
        auto pcl_item = m_pclUI->folderFileList->currentItem();
        if ( !pcl_item )
            throw std::runtime_error( "no file selected" );
        QString str_full_file_path = pcl_item->data(MediaSourceDirectory).toString()+"/"+pcl_item->text();
        if ( !QFile::exists( str_full_file_path ) )
            throw std::runtime_error( "selected file \""+str_full_file_path.toStdString()+"\" does not exist (any more?)!" );
        
        if ( QMessageBox::Yes == QMessageBox::question( this, "Delete File", QString("Are you sure you want to delete\n%1?").arg(str_full_file_path), QMessageBox::StandardButtons(QMessageBox::Yes|QMessageBox::No), QMessageBox::Yes ) )
        {
            if ( !QFile(str_full_file_path).remove() )
                throw std::runtime_error( "unable to remove file" );
            // remember to remove the file from the list
            QSignalBlocker cl_list_sig_bloker(m_pclUI->folderFileList);
            int i_row = m_pclUI->folderFileList->currentRow();
            pcl_item = m_pclUI->folderFileList->takeItem(i_row);
            delete pcl_item;
            m_pclUI->folderFileList->setCurrentRow(i_row);
            // and update the file count for the directory
            updateTotalFileCountLabel();
            switchFile( m_pclUI->folderFileList->item(i_row), nullptr );
        }
    }
    catch( const std::exception& rclExc )
    {
        QMessageBox::critical( this, "Delete Error", QString( "Failed to delete file: %1" ).arg(rclExc.what()) );
    }
}

void TagSupporter::selectNextFile()
{
    m_pclUI->folderFileList->setCurrentRow(m_pclUI->folderFileList->currentRow()+1);
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

void TagSupporter::switchFile(QListWidgetItem *pclCurrent, QListWidgetItem *pclPrevious)
{
    // check if previous file was modified...
    if ( pclPrevious )
    {
        if ( m_pclUI->metadataWidget->isModified() || m_pclUI->filenameWidget->isModified() )
        {
            // ask user and possibly switch back to old item ...
            if ( QMessageBox::question( this, "unsaved modifications", QString("There are possibly unsaved modifications for file %1. Are you sure you want to discard those?").arg( pclPrevious->text() ) ) 
                 != QMessageBox::Yes )
            {
                m_pclUI->folderFileList->blockSignals(true);
                m_pclUI->folderFileList->setCurrentItem(pclPrevious);
                m_pclUI->folderFileList->setCurrentRow(m_pclUI->folderFileList->row(pclPrevious));
                m_pclUI->folderFileList->blockSignals(false);
                m_pclUI->folderFileList->update();
                return;
            }
        }
        // set font back to normal
        QFont cl_font = pclPrevious->font();
        cl_font.setBold(false);;
        pclPrevious->setFont(cl_font);
    }
    if ( pclCurrent )
    {
        selectFile(pclCurrent->data(MediaSourceDirectory).toString() + "/" + pclCurrent->text());
        m_pclUI->nextButton->setEnabled( m_pclUI->folderFileList->row(pclCurrent) < m_pclUI->folderFileList->count()-1 );
    }
}

void TagSupporter::fileModified()
{
    m_pclUI->saveButton->setEnabled(true);
    QListWidgetItem* pcl_item = m_pclUI->folderFileList->currentItem();
    if ( pcl_item )
    {
        QFont cl_font = pcl_item->font();
        cl_font.setBold(true);
        pcl_item->setFont( cl_font );
    }
}
