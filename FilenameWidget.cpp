#include "FilenameWidget.h"
#include <QFileInfo>
#include <QPushButton>
#include <QDir>
#include "ui_FilenameWidget.h"

FilenameWidget::FilenameWidget(QWidget *pclParent) :
    QWidget(pclParent),
    m_pclUI(std::make_unique<Ui::FilenameWidget>())
{
    m_pclUI->setupUi(this);
    m_pclUI->destinationCheck->hide();
    connect( m_pclUI->filenameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onTextEdited(const QString&)) );
    connect( this, &FilenameWidget::filenameModified, [this]{ m_bIsModified = true; } );
}

void FilenameWidget::addFilenameFormat(const QString &strTitle, const QString& strFormat)
{
    QPushButton* pcl_button = new QPushButton(strTitle,this);
    pcl_button->setCheckable(true);
    pcl_button->setChecked(false);
    m_pclUI->filenameButtonLayout->addWidget( pcl_button);
    connect( pcl_button, SIGNAL(clicked()), this, SLOT(formatterClicked()) );
    
    m_mapFilenameFormatters[pcl_button] = strFormat;
}

void FilenameWidget::addDestinationDirectoryFormat( const QString& strTitle, const QString& strFormat )
{
    QPushButton* pcl_button = new QPushButton(strTitle,this);
    pcl_button->setCheckable(true);
    pcl_button->setChecked(false);
    m_pclUI->destinationButtonLayout->addWidget( pcl_button);
    connect( pcl_button, SIGNAL(clicked()), this, SLOT(destinationClicked()) );
    
    m_mapDestinationFormatters[pcl_button] = strFormat;
    m_pclUI->destinationCheck->show();
}

bool FilenameWidget::isModified() const
{
    return m_bIsModified;
}

void FilenameWidget::setDestinationBaseDirectory(const QString &strDirectory)
{
    m_strDestinationBaseDirectory = strDirectory;
}

QString FilenameWidget::filename() const
{
    return m_pclUI->filenameEdit->text();
}

FilenameWidget::~FilenameWidget() = default;

void FilenameWidget::setFilename( const QString& strFilename )
{
    m_bIsModified = false;
    bool b_replaced = filenameWithoutInvalidCharacters(QFileInfo(strFilename).fileName());
    for ( auto &rcl_button_item : m_mapFilenameFormatters )
        rcl_button_item.first->setEnabled(true);
    if ( !updateFilenameIfNecessary() && b_replaced )
        emit filenameModified();
}

bool FilenameWidget::applyToFile(const QString &strFilePath)
{
    QFileInfo cl_old_path(strFilePath);
    QString strNewFilePath;
    if ( m_pclUI->destinationCheck->isChecked() )
    {
        QString str_sub_dir = destinationSubdirectory();
        // try to create new subdirectory if it doesn't exist yet
        if ( !QDir( destinationBaseDirectory() ).mkpath( str_sub_dir ) )
            return false;
        strNewFilePath = destinationBaseDirectory() + "/" + str_sub_dir;
    }
    else
        strNewFilePath = cl_old_path.absolutePath();
    strNewFilePath += "/" + filename();
    //quickly check if old and new path are the same --> nothing to do
    if ( QFileInfo(strNewFilePath) == cl_old_path )
        m_bIsModified = false;
    else // otherwise: attempt to rename the file
        m_bIsModified = !QFile::rename(strFilePath, strNewFilePath);
    return !m_bIsModified;
}

void FilenameWidget::clear()
{
    m_bIsModified = false;
    m_pclUI->filenameEdit->blockSignals(true);
    m_pclUI->filenameEdit->clear();
    m_pclUI->filenameEdit->blockSignals(false);
    for ( auto &rcl_button_item : m_mapFilenameFormatters )
        rcl_button_item.first->setEnabled(false);
}

void FilenameWidget::setAlbumArtist( const QString& strArtist )
{
    m_strAlbumArtist = strArtist;
    updateFilenameIfNecessary();
}

void FilenameWidget::setTrackArtist( const QString& strArtist )
{
    m_strTrackArtist = strArtist;
    updateFilenameIfNecessary();
}

void FilenameWidget::setAlbum( const QString& strAlbum )
{
    m_strAlbum = strAlbum;
    updateFilenameIfNecessary();
}

void FilenameWidget::setTitle( const QString& strTitle )
{
    m_strTitle = strTitle;
    updateFilenameIfNecessary();
}

void FilenameWidget::setTrackNumber( int iNumber )
{
    m_iNumber = iNumber;
    updateFilenameIfNecessary();
}

void FilenameWidget::formatterClicked()
{
    QPushButton* pcl_button = dynamic_cast<QPushButton*>(sender());
    if ( !pcl_button )
        return;
    // uncheck all other buttons
    for ( auto &rcl_button_item : m_mapFilenameFormatters )
    {
        if ( rcl_button_item.first != pcl_button )
            rcl_button_item.first->setChecked(false);
    }
    updateFilenameIfNecessary();
}

void FilenameWidget::destinationClicked()
{
    QPushButton* pcl_button = dynamic_cast<QPushButton*>(sender());
    if ( !pcl_button )
        return;
    m_pclUI->destinationCheck->setChecked(pcl_button->isChecked());
    // uncheck all other buttons
    for ( auto &rcl_button_item : m_mapDestinationFormatters )
    {
        if ( rcl_button_item.first != pcl_button )
            rcl_button_item.first->setChecked(false);
    }
}

void FilenameWidget::onTextEdited(const QString& strFilename)
{
    QString str_previous_file_name = m_pclUI->filenameEdit->text();
    filenameWithoutInvalidCharacters(strFilename);
    if ( str_previous_file_name.compare( m_pclUI->filenameEdit->text() ) != 0 )
        emit filenameModified();
}

static QString replaceInvalidFilenameCharacters( QString str )
{
    str.replace( QRegularExpression("[:]"), "-" );
    str.replace( QRegularExpression("[$/\\\\?*|\"<>]"), "_" );
    // replace trailing dots as well
    QRegularExpressionMatch cl_match = QRegularExpression("^(.*[^\\.])(\\.+)$").match( str );
    if ( cl_match.hasMatch() )
        str = cl_match.captured(1) + QString( cl_match.captured(2).length(), '_' );
    return str;
}

bool FilenameWidget::filenameWithoutInvalidCharacters(const QString& strFilename)
{
    QString str_filename = replaceInvalidFilenameCharacters(strFilename);
    QSignalBlocker cl_blocker( m_pclUI->filenameEdit );
    int i_curser = m_pclUI->filenameEdit->cursorPosition();
    m_pclUI->filenameEdit->setText( str_filename );
    m_pclUI->filenameEdit->setCursorPosition(i_curser);
    return str_filename != strFilename;
}

bool FilenameWidget::updateFilenameIfNecessary()
{   
    for ( auto &rcl_button_item : m_mapFilenameFormatters )
        if ( rcl_button_item.first->isChecked() )
        {
            QStringList lst_filename_parts = rcl_button_item.second.split("%", QString::SkipEmptyParts);
            for ( QString& str_part : lst_filename_parts )
            {
                if ( str_part.startsWith("N") )
                    str_part.replace(0,1, QString("%1").arg(m_iNumber,2,10,QChar('0')) );
                else if ( str_part.startsWith("A") )
                    str_part.replace(0,1, m_strTrackArtist );
                else if ( str_part.startsWith("C") )
                    str_part.replace(0,1, m_strAlbumArtist );
                else if ( str_part.startsWith("B") )
                    str_part.replace(0,1, m_strAlbum );
                else if ( str_part.startsWith("T") )
                    str_part.replace(0,1, m_strTitle );
            }
            lst_filename_parts << "." << QFileInfo(m_pclUI->filenameEdit->text()).suffix();
            
            // replace any notallowed characters in filenames
            QString str_previous_file_name = m_pclUI->filenameEdit->text();
            filenameWithoutInvalidCharacters( lst_filename_parts.join("") );
            if ( str_previous_file_name.compare( m_pclUI->filenameEdit->text() ) != 0 )
                emit filenameModified();
            return true;
        }
    return false;
}

QString FilenameWidget::destinationSubdirectory() const
{
    if ( m_pclUI->destinationCheck->isChecked() )
        for ( auto &rcl_button_item : m_mapDestinationFormatters )
            if ( rcl_button_item.first->isChecked() )
            {
                QStringList lst_filename_parts = rcl_button_item.second.split("%", QString::SkipEmptyParts);
                for ( QString& str_part : lst_filename_parts )
                {
                    if ( str_part.startsWith("N") )
                        str_part.replace(0,1, QString("%1").arg(m_iNumber,2,10,QChar('0')) );
                    else if ( str_part.startsWith("A") )
                        str_part.replace(0,1, replaceInvalidFilenameCharacters(m_strTrackArtist) );
                    else if ( str_part.startsWith("C") )
                        str_part.replace(0,1, replaceInvalidFilenameCharacters(m_strAlbumArtist) );
                    else if ( str_part.startsWith("B") )
                        str_part.replace(0,1, replaceInvalidFilenameCharacters(m_strAlbum) );
                    else if ( str_part.startsWith("T") )
                        str_part.replace(0,1, replaceInvalidFilenameCharacters(m_strTitle) );
                }
                
                return lst_filename_parts.join("");
            }
    return QString();
}
