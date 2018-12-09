#include "FileBrowserWidget.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include "ui_FileBrowserWidget.h"

enum {
    MediaSourceDirectory = Qt::UserRole,
    FileIsModified
};

FileBrowserWidget::FileBrowserWidget( QWidget *pclParent )
: QWidget(pclParent)
, m_pclUI( std::make_unique<Ui::FileBrowserWidget>() )
{
    m_pclUI->setupUi(this);

    connect( m_pclUI->browseFolderButton, &QPushButton::clicked, this, &FileBrowserWidget::browseForFolder );
    connect( m_pclUI->folderFileList, &QListWidget::currentItemChanged, this,&FileBrowserWidget::switchFile );
    connect( m_pclUI->folderFileList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* pclItem){switchFile(pclItem,pclItem);} );    

    connect( m_pclUI->saveButton, &QPushButton::clicked, this, &FileBrowserWidget::saveCurrent );
    connect( m_pclUI->nextButton, &QPushButton::clicked, this, &FileBrowserWidget::selectNextFile );
    connect( m_pclUI->deleteButton, &QPushButton::clicked, this, &FileBrowserWidget::deleteCurrent );
    connect( m_pclUI->refreshButton, &QPushButton::clicked, [this]{scanFolder(getLastUsedFolder());} );

    m_pclUI->refreshButton->setEnabled(false);
    m_pclUI->saveButton->setEnabled(false);
    m_pclUI->nextButton->setEnabled(false);
    m_pclUI->deleteButton->setEnabled(false);
}

FileBrowserWidget::~FileBrowserWidget() = default;

void FileBrowserWidget::scanFolder(QString strFolder)
{
    QDir cl_dir(strFolder);
    // sanitize folder name
    strFolder = cl_dir.absolutePath();
    
    setLastUsedFolder( strFolder );
    m_pclUI->refreshButton->setEnabled(true);
    m_pclUI->folderFileList->blockSignals(true);
    m_pclUI->folderFileList->clear();

    // list all audio files in this folder...
    QStringList lst_filters;
    lst_filters << "*.mp3" << "*.ogg" << "*.oga" << "*.flac" << "*.wma" << "*.mp4" << "*.m4a";
    QStringList lst_files = cl_dir.entryList( lst_filters, QDir::Files, QDir::Name | QDir::IgnoreCase | QDir::LocaleAware );
    for ( const QString& strFilename : lst_files )
    {
        QListWidgetItem* pcl_item = new QListWidgetItem(strFilename);
        pcl_item->setData(MediaSourceDirectory,strFolder);
        pcl_item->setData(FileIsModified,false);
        m_pclUI->folderFileList->addItem( pcl_item );
    }
    m_pclUI->folderFileList->setCurrentRow(0);
    m_pclUI->folderFileList->blockSignals(false);

    updateTotalFileCountLabel();
    
    if ( !lst_files.isEmpty() ) {
        m_pclUI->saveButton->setEnabled(false);
        m_pclUI->deleteButton->setEnabled(true);
        fileSelected(cl_dir.absoluteFilePath(lst_files.front()));
    }
    else
    {
        m_pclUI->saveButton->setEnabled(false);
        m_pclUI->deleteButton->setEnabled(false);
        emit noFileSelected();
    }
    m_pclUI->nextButton->setEnabled(lst_files.size() > 1);
    emit folderChanged( strFolder );
}

void FileBrowserWidget::browseForFolder()
{
    QString str_folder = QFileDialog::getExistingDirectory( this, "Select folder", getLastUsedFolder() );
    if ( !str_folder.isNull() )
        scanFolder(str_folder);
}

void FileBrowserWidget::updateTotalFileCountLabel()
{
    QString str_last_used_folder = getLastUsedFolder();
    QDir cl_dir(str_last_used_folder);
    int i_num_considered_files = 0;
    for ( int i_item = 0; i_item < m_pclUI->folderFileList->count(); ++i_item )
    {
        auto pcl_item = m_pclUI->folderFileList->item(i_item);
        if ( pcl_item->data(MediaSourceDirectory).toString().compare( str_last_used_folder ) == 0 )
            ++i_num_considered_files;
    }
    m_pclUI->folderContentLabel->setText( QString("%1 audio files.\n%2 other files (not displayed).").arg( i_num_considered_files ).arg(cl_dir.entryList( {}, QDir::Files).count()-i_num_considered_files) );
}

void FileBrowserWidget::deleteCurrent()
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

void FileBrowserWidget::selectNextFile()
{
    m_pclUI->folderFileList->setCurrentRow(m_pclUI->folderFileList->currentRow()+1);
}

QString FileBrowserWidget::getLastUsedFolder() const
{
    return QSettings().value("filebrowser/last_used").toString();
}

void FileBrowserWidget::setLastUsedFolder( QString strFolder ) const
{
    QSettings().setValue("filebrowser/last_used", strFolder );
}

void FileBrowserWidget::switchFile(QListWidgetItem *pclCurrent, QListWidgetItem *pclPrevious)
{
    // check if previous file was modified...
    if ( pclPrevious )
    {
        if ( isModified(pclPrevious) )
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

        setFileModified(pclPrevious,false);
        m_pclUI->saveButton->setEnabled(false);
    }
    if ( pclCurrent )
    {
        emit fileSelected(pclCurrent->data(MediaSourceDirectory).toString() + "/" + pclCurrent->text());
        m_pclUI->nextButton->setEnabled( m_pclUI->folderFileList->row(pclCurrent) < m_pclUI->folderFileList->count()-1 );
    }
    else {
        emit noFileSelected();
    }
}

bool FileBrowserWidget::isModified(QListWidgetItem* pclItem)
{
    if ( pclItem )
        return pclItem->data(FileIsModified).toBool();
    else
        return false;
}

void FileBrowserWidget::setFileModified( QListWidgetItem* pclItem, bool bModified )
{
    if ( pclItem ) {
        pclItem->setData(FileIsModified,bModified);
        QFont cl_font = pclItem->font();
        cl_font.setBold(bModified);
        pclItem->setFont( cl_font );
    }
}

void FileBrowserWidget::setFileModified( bool bModified )
{
    m_pclUI->saveButton->setEnabled(bModified);
    setFileModified( m_pclUI->folderFileList->currentItem(), bModified );
}

void FileBrowserWidget::currentFileMoved( const QString& strNewFilename, const QString& strNewFolder )
{
    auto pcl_item = m_pclUI->folderFileList->currentItem();
    if ( pcl_item )
    {
        pcl_item->setText( strNewFilename );
        if ( !strNewFolder.isEmpty() )
            pcl_item->setData( MediaSourceDirectory, strNewFolder );
            
        updateTotalFileCountLabel();

        emit fileSelected(pcl_item->data(MediaSourceDirectory).toString() + "/" + pcl_item->text());
    }
}

void FileBrowserWidget::saveCurrent()
{
    try
    {     
        auto pcl_item = m_pclUI->folderFileList->currentItem();
        if ( !pcl_item )
            throw std::runtime_error( "no file selected" );
        QString str_full_file_path = pcl_item->data(MediaSourceDirectory).toString()+"/"+pcl_item->text();
        if ( !QFile::exists( str_full_file_path ) )
            throw std::runtime_error( "selected file \""+str_full_file_path.toStdString()+"\" does not exist (any more?)!" );
        
        emit saveFile( str_full_file_path );

        // set font to italic to indicate: item visited
        QFont cl_font = pcl_item->font();
        cl_font.setItalic(true);
        pcl_item->setFont(cl_font);
    }
    catch( const std::exception& rclExc )
    {
        QMessageBox::critical( this, "Save Error", QString( "Failed to save all modifications: %1" ).arg(rclExc.what()) );
    }
}
