#include "MetadataWidget.h"
#include "ui_MetadataWidget.h"

#include <QBuffer>
#include <QFileDialog>
#include <QMessageBox>
#include <QDate>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <taglib/xiphcomment.h>
#include <taglib/flacpicture.h>
#include <taglib/flacfile.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/attachedpictureframe.h>

const QStringList MetadataWidget::s_lstStandardTags = QStringList() 
    << "TITLE" << "ALBUM" << "ARTIST" << "TRACKNUMBER" << "DATE" << "GENRE";

const QStringList MetadataWidget::s_lstExtendedTags = QStringList() 
    << "ALBUMARTIST" << "TRACKTOTAL" << "DISCNUMBER";

// enum to consistently address the standard tag list
enum StandardTags {  TrackTitle = 0, Album = 1, TrackArtist = 2, TrackNumber = 3, Year = 4, Genre = 5 };
// enum to consistently address the extended tag list
enum ExtendedTags {  AlbumArtist = 0, TotalTracks = 1, DiscNumber = 2 };

static inline QString T2Q( const TagLib::String& str )
{
    return QString::fromUtf8( str.to8Bit(true).c_str() );
}
static inline TagLib::String Q2T( const QString& str )
{
    return TagLib::String( str.toUtf8().data(), TagLib::String::UTF8 );
}

MetadataWidget::MetadataWidget(QWidget *parent)
: QWidget(parent)
, m_pclUI( std::make_unique<Ui::MetadataWidget>() )
{
    m_pclUI->setupUi(this);
    
    connect( m_pclUI->trackArtistEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(trackArtistChanged(const QString &)) );
    connect( m_pclUI->albumArtistEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(albumArtistChanged(const QString &)) );
    connect( m_pclUI->albumEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(albumChanged(const QString &)) );
    connect( m_pclUI->titleEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(titleChanged(const QString &)) );
    connect( m_pclUI->genreCombo, SIGNAL(currentTextChanged(const QString &)), this, SIGNAL(genreChanged(const QString &)) );
    connect( m_pclUI->trackSpin, SIGNAL(valueChanged(int)), this, SIGNAL(trackNumberChanged(int)) );
    connect( m_pclUI->yearSpin, SIGNAL(valueChanged(int)), this, SIGNAL(yearChanged(int)) );
    
    connect( m_pclUI->trackArtistEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->albumEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->titleEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->trackSpin, SIGNAL(valueChanged(int)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->albumArtistEdit, SIGNAL(textChanged(const QString&)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->cdEdit, SIGNAL(textChanged(const QString&)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->yearSpin, SIGNAL(valueChanged(int)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->genreCombo, SIGNAL(currentTextChanged(const QString&)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->totalTracksSpin, SIGNAL(valueChanged(int)), this, SIGNAL(metadataModified()) );
    connect( m_pclUI->clearOtherTagsCheck, SIGNAL(clicked()), this, SIGNAL(metadataModified()));
    
    connect( m_pclUI->clearCoverButton, SIGNAL(clicked()), this, SLOT(clearCover()));
    connect( m_pclUI->loadCoverButton, SIGNAL(clicked()), this, SLOT(loadCover()));
    connect( m_pclUI->googleCoverButton, SIGNAL(clicked()), this, SLOT(googleCover()));
    connect( m_pclUI->singleTrackButton, SIGNAL(clicked()), this, SLOT(setSingleTrack()) );
    
    connect( this, &MetadataWidget::metadataModified, [this]{m_bIsModified = true;} );
}

MetadataWidget::~MetadataWidget() = default;

bool MetadataWidget::isModified() const
{
    return m_bIsModified;
}

void MetadataWidget::setGenreList(const QStringList &lstGenres)
{
    m_pclUI->genreCombo->clear();
    m_pclUI->genreCombo->addItems(lstGenres);
}

void MetadataWidget::parseGenericTagInformation(TagLib::Tag *pclTag)
{
    setTrackArtist(T2Q(pclTag->artist()));
    setTrackTitle(T2Q(pclTag->title()));
    setAlbum(T2Q(pclTag->album()));
    setTrackNumber( pclTag->track() );
    setYear( pclTag->year() );
    setGenre( T2Q(pclTag->genre()) );
}

template<class SetterFun>
bool MetadataWidget::setFreeTag( const QString& strQueryTag, const QString& strTag, const QStringList& lstEntries, SetterFun funSetTag )
{
    if ( strTag.compare( strQueryTag, Qt::CaseInsensitive ) == 0 )
    {
        if ( lstEntries.empty() )
            emit error( QString("unexpected number of entries for %1: found %2 entries").arg(strQueryTag).arg(lstEntries.size()) );
        else if ( lstEntries.size() > 1 )
            emit error( QString("unexpected number of entries for %1: found %2 entries (%3)").arg(strQueryTag).arg(lstEntries.size()).arg(lstEntries.join(";")) );
        else if ( !funSetTag( lstEntries.front() ) )
            emit error( QString("failed to set %1 to value \"%2\"").arg(strQueryTag).arg(lstEntries.front()) );         
        return true;
    }
    return false;
}

QStringList MetadataWidget::parseFreeTagInformation( const TagLib::PropertyMap& mapProps )
{    
    QStringList lst_other_tags;
    for ( const auto & rcl_item : mapProps )
    {
        QString str_tag = T2Q( rcl_item.first );
        if ( s_lstStandardTags.contains( str_tag, Qt::CaseInsensitive ) )
            continue;
        QStringList lst_sub_entries;
        for ( const TagLib::String& str_item : rcl_item.second )
            lst_sub_entries << T2Q( str_item );
        if ( !setFreeTag(s_lstExtendedTags.at(AlbumArtist),str_tag, lst_sub_entries, [this](const QString& strEntry){ setAlbumArtist(strEntry); return true; } ) )
        if ( !setFreeTag(s_lstExtendedTags.at(TotalTracks),str_tag,lst_sub_entries, [this](const QString& strEntry){ bool b_ok; setTotalTracks( strEntry.toInt(&b_ok) ); return b_ok; } ) )
        if ( !setFreeTag(s_lstExtendedTags.at(DiscNumber),str_tag,lst_sub_entries, [this](const QString& strEntry){ setDiscNumber(strEntry); return true; } ) )
            lst_other_tags << QString("%1 = %2").arg( str_tag ).arg( lst_sub_entries.join(";") );
    }
    return lst_other_tags;
}

template<class PictureType, typename EnumType>
PictureType* MetadataWidget::selectFrontCoverFromPictureList( const std::list<PictureType*>& lstPictures, EnumType FrontCoverValue )
{
    if ( lstPictures.empty() ) // no pictures? then we're done
        return nullptr;
    
    // get cover picture
    PictureType* pcl_selected_picture = nullptr;
    if ( lstPictures.size() == 1 )
        pcl_selected_picture = lstPictures.front();
    else //try to find the picture with type "FrontCover"
    {
        for ( PictureType* pcl_picture : lstPictures )
            if (pcl_picture->type() == FrontCoverValue )
            {
                if ( pcl_selected_picture == nullptr)
                    pcl_selected_picture = pcl_picture;
                else
                {
                    //warn about multiple front cover images, keep the first one
                    emit error( "more than 1 picture with type FrontCover found!" );
                    break;
                }
            }
        if ( pcl_selected_picture == nullptr )
        {
            pcl_selected_picture = lstPictures.front(); //arbitrarily take first one and warn
            emit error( QString("file contains %1 pictures - none of type FrontCover!").arg(lstPictures.size()) );
        }
    }
    return pcl_selected_picture;
}

void MetadataWidget::parseFile( TagLib::FLAC::File& rclFile )
{
    if (!rclFile.tag())
        throw std::runtime_error( "file doesn't have a tag" );
    QStringList lst_other_tags;
    QStringList lst_included_tags;
    if ( rclFile.hasID3v1Tag() )
    {
        parseGenericTagInformation( rclFile.ID3v1Tag() );
        lst_other_tags += parseFreeTagInformation( rclFile.ID3v1Tag()->properties() );
        lst_included_tags << "ID3v1";
    }
    if ( rclFile.hasID3v2Tag() )
    {
        parseGenericTagInformation( rclFile.ID3v2Tag() );
        lst_other_tags += parseFreeTagInformation( rclFile.ID3v2Tag()->properties() );
        lst_included_tags << "ID3v2";
    }
    if ( rclFile.hasXiphComment() )
    {
        parseGenericTagInformation( rclFile.xiphComment() );
        lst_other_tags += parseFreeTagInformation( rclFile.xiphComment()->properties() );
        lst_included_tags << "XiphComment";
    }
    lst_other_tags.removeDuplicates();
    m_pclUI->otherTagsList->addItems(lst_other_tags);
    m_pclUI->tagTypesLabel->setText( lst_included_tags.join(", ") );
    
    TagLib::FLAC::Picture* pcl_selected_picture = selectFrontCoverFromPictureList( std::list<TagLib::FLAC::Picture*>{rclFile.pictureList().begin(), rclFile.pictureList().end()}, TagLib::FLAC::Picture::FrontCover);
    
    if ( pcl_selected_picture )
    {
        m_pclFullResCover = std::make_unique<QPixmap>();
        m_pclFullResCover->loadFromData( QByteArray( pcl_selected_picture->data().data(), pcl_selected_picture->data().size() ) );
        showCover();
    }
}

void MetadataWidget::parseFile( TagLib::MPEG::File& rclFile )
{
    if (!rclFile.tag())
        throw std::runtime_error( "file doesn't have a tag" );
    QStringList lst_other_tags;
    QStringList lst_included_tags;
    if ( rclFile.hasID3v1Tag() )
    {
        parseGenericTagInformation( rclFile.ID3v1Tag() );
        lst_other_tags += parseFreeTagInformation( rclFile.ID3v1Tag()->properties() );
        lst_included_tags << "ID3v1";
    }
    if ( rclFile.hasAPETag() )
    {
        parseGenericTagInformation( rclFile.APETag() );
        lst_other_tags += parseFreeTagInformation( rclFile.APETag()->properties() );
        lst_included_tags << "APE";
    }
    if ( rclFile.hasID3v2Tag() )
    {
        parseGenericTagInformation( rclFile.ID3v2Tag() );
        lst_other_tags += parseFreeTagInformation( rclFile.ID3v2Tag()->properties() );
        lst_included_tags << "ID3v2";
    }
    lst_other_tags.removeDuplicates();
    m_pclUI->otherTagsList->addItems(lst_other_tags);
    m_pclUI->tagTypesLabel->setText( lst_included_tags.join(", ") );
    
    if ( !rclFile.hasID3v2Tag() ) // no ID3v2 --> no pictures. Then we're done
        return;
    
    std::list<TagLib::ID3v2::AttachedPictureFrame*> lst_pictures;
    for ( TagLib::ID3v2::Frame* pcl_frame : rclFile.ID3v2Tag()->frameList() )
    {
        auto pcl_picture = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>( pcl_frame );
        if (pcl_picture)
            lst_pictures.push_back(pcl_picture);
    }
    TagLib::ID3v2::AttachedPictureFrame* pcl_selected_picture = selectFrontCoverFromPictureList( lst_pictures, TagLib::ID3v2::AttachedPictureFrame::FrontCover);
    
    if ( pcl_selected_picture )
    {
        m_pclFullResCover = std::make_unique<QPixmap>();
        m_pclFullResCover->loadFromData( QByteArray( pcl_selected_picture->picture().data(), pcl_selected_picture->picture().size() ) );
        showCover();
    }
}

static TagLib::ByteVector getJPEGData(const QPixmap& rclPixmap)
{
    QByteArray cl_jpeg_data;
    QBuffer cl_buffer(&cl_jpeg_data);
    rclPixmap.save(&cl_buffer, "JPG");
    cl_buffer.close();
    return TagLib::ByteVector( cl_jpeg_data.data(), cl_jpeg_data.size() );
}

bool MetadataWidget::applyTags( TagLib::FLAC::File& rclFile )
{
    // if selected to clear out other tags, start by stripping all tags
    if ( m_pclUI->clearOtherTagsCheck->isChecked() )
        rclFile.strip( TagLib::FLAC::File::AllTags );
    else // otherwise: strip ID3v1 and ID3v2 and just keep XiphComment
        rclFile.strip( TagLib::FLAC::File::ID3v1 | TagLib::FLAC::File::ID3v2 );
    
    // set tags properties
    TagLib::PropertyMap map_failed = rclFile.setProperties( setMetadataInPropertyMap( rclFile.properties() ) );
    if ( !map_failed.isEmpty() )
        emit error( QString( "failed to apply the following tag(s): %1" ).arg( T2Q(map_failed.toString()) ) );
    
    // set cover image
    if ( m_pclFullResCover )
    {
        //first clear all existing images of type "FrontCover"
        for ( TagLib::FLAC::Picture* pcl_picture : rclFile.pictureList() )
            if (pcl_picture->type() == TagLib::FLAC::Picture::FrontCover )
                rclFile.removePicture( pcl_picture, true );
        
        // create new image, if any
        if ( !m_pclFullResCover->isNull() )
        {                               
            std::unique_ptr<TagLib::FLAC::Picture> pcl_picture = std::make_unique<TagLib::FLAC::Picture>();
            pcl_picture->setType( TagLib::FLAC::Picture::FrontCover );
            pcl_picture->setMimeType( "image/jpeg" );
            pcl_picture->setWidth( m_pclFullResCover->width() );
            pcl_picture->setHeight( m_pclFullResCover->height() );
            pcl_picture->setData( getJPEGData(*m_pclFullResCover) );
            
            rclFile.addPicture( pcl_picture.release() );
        }
    }
    
    // finally: save the file
    return rclFile.save();
}

bool MetadataWidget::applyTags( TagLib::MPEG::File& rclFile )
{
    // if selected to clear out other tags, start by stripping all tags
    if ( m_pclUI->clearOtherTagsCheck->isChecked() )
    {
        if ( !rclFile.strip( TagLib::MPEG::File::AllTags ) )
            emit error( "failed to strip old tags from file" );
    }
    else // otherwise: strip ID3v1 and APE and just keep ID3v2
        if ( !rclFile.strip( TagLib::MPEG::File::ID3v1 | TagLib::MPEG::File::APE ) )
            emit error( "failed to strip ID3v1 and APE tags from file" );
    
    // set tags properties
    TagLib::PropertyMap map_failed = rclFile.setProperties( setMetadataInPropertyMap( rclFile.properties() ) );
    if ( !map_failed.isEmpty() )
        emit error( QString( "failed to apply the following tag(s): %1" ).arg( T2Q(map_failed.toString()) ) );
    
    // set cover image
    if ( m_pclFullResCover )
    {
        //first clear all existing images of type "FrontCover"
        bool b_found_front_cover_in_last_run;
        do {
            b_found_front_cover_in_last_run = false;
            for ( TagLib::ID3v2::Frame* pcl_frame : rclFile.ID3v2Tag()->frameList() )
            {
                auto pcl_picture = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>( pcl_frame );
                if (pcl_picture && pcl_picture->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover)
                {
                    rclFile.ID3v2Tag()->removeFrame( pcl_frame );
                    b_found_front_cover_in_last_run = true;
                    break;
                }
            }
        } while ( b_found_front_cover_in_last_run );
        
        // create new image, if any
        if ( !m_pclFullResCover->isNull() )
        {                                   
            std::unique_ptr<TagLib::ID3v2::AttachedPictureFrame> pcl_picture = std::make_unique<TagLib::ID3v2::AttachedPictureFrame>();
            pcl_picture->setType( TagLib::ID3v2::AttachedPictureFrame::FrontCover );
            pcl_picture->setMimeType( "image/jpeg" );
            pcl_picture->setPicture( getJPEGData(*m_pclFullResCover) );
            
            rclFile.ID3v2Tag()->addFrame( pcl_picture.release() );
        }
    }
    
    // finally: save the file, keeping only the ID3v2 and stripping all other tags
    return rclFile.save( TagLib::MPEG::File::ID3v2, true );
}

template<class FileClass>
bool MetadataWidget::tryOpenAndParse( const QString& strFilename )
{
    FileClass cl_file( strFilename.toLocal8Bit(), false );
    if ( cl_file.isValid() )
    {
        parseFile(cl_file);
        return true;
    }
    return false;
}

template<class FileClass>
bool MetadataWidget::tryOpenAndSave( const QString& strFilename )
{
    FileClass cl_file( strFilename.toLocal8Bit(), false );
    if ( cl_file.isValid() )
    {
        bool b_save_succeeded = applyTags(cl_file);
        m_pclUI->otherTagsList->clear();
        parseFile(cl_file); // read the data to update interface
        return b_save_succeeded;
    }
    return false;
}

void MetadataWidget::loadFromFile(const QString& strFilename)
{
    m_bIsModified = false;
    auto lst_blockers = blockAllFormSignals();
    m_pclUI->otherTagsList->clear();
    try
    {
        // figure out the type of file
        if ( !tryOpenAndParse<TagLib::FLAC::File>(strFilename) )
        if ( !tryOpenAndParse<TagLib::MPEG::File>(strFilename) )
            throw std::runtime_error( "failed to open or unknown file format" );
        
        emit trackArtistChanged( m_pclUI->trackArtistEdit->text() );
        emit albumArtistChanged( m_pclUI->albumArtistEdit->text() );
        emit albumChanged( m_pclUI->albumEdit->text() );
        emit titleChanged( m_pclUI->titleEdit->text() );
        emit genreChanged( m_pclUI->genreCombo->currentText() );
        emit trackNumberChanged( m_pclUI->trackSpin->value() );
        emit yearChanged( m_pclUI->yearSpin->value() );
    }
    catch ( const std::exception& rclExc )
    {
        emit error( QString( "Failed to show metadata for %1: %2").arg(strFilename).arg( rclExc.what() ) );
    }
    m_pclUI->clearOtherTagsCheck->setEnabled(m_pclUI->otherTagsList->count() > 0);
    m_pclUI->clearOtherTagsCheck->setChecked(false);
}

bool MetadataWidget::checkConsistency()
{
    QStringList lst_problems;
    // check if required fields are set
    if ( m_pclUI->trackArtistEdit->text().isEmpty() )
        lst_problems << "No track artist set.";
    if ( m_pclUI->titleEdit->text().isEmpty() )
        lst_problems << "No track title set.";
    if ( m_pclUI->albumEdit->text().isEmpty() )
        lst_problems << "No album set.";
    if ( m_pclUI->yearSpin->value() < 1800 || m_pclUI->yearSpin->value() > QDate::currentDate().year() )
        lst_problems << QString("Year is not within plausible range (%1-%2)").arg(1800).arg(QDate::currentDate().year());
    
    if ( m_pclUI->genreCombo->currentText().isEmpty() )
        lst_problems << "No genre set.";
    // check genre for consistency with given list
    else if ( m_pclUI->genreCombo->findText( m_pclUI->genreCombo->currentText(), Qt::MatchExactly ) < 0 )
        lst_problems << QString("The genre \"%1\" was not found in the list of genres.").arg(m_pclUI->genreCombo->currentText());
    
    return lst_problems.empty() ||
            QMessageBox::Yes == QMessageBox::question( this, "Metadata consistency issues", QString("There %1 metadata issue%2:\n%3\n\nAre you sure you want to continue?")
         .arg( lst_problems.size() == 1 ? "is one" : "are", lst_problems.size() == 1 ? "" : "s", lst_problems.join( "\n" ) ) );
}

bool MetadataWidget::saveToFile(const QString &strFilename)
{
    if ( !checkConsistency() )
        return false;
    try
    {
        // figure out the type of file
        if ( !tryOpenAndSave<TagLib::FLAC::File>(strFilename) )
        if ( !tryOpenAndSave<TagLib::MPEG::File>(strFilename) )
            throw std::runtime_error( "failed to open or unknown file format" );
        m_bIsModified = false;
    }
    catch ( const std::exception& rclExc )
    {
        throw std::runtime_error( QString( "Failed to save metadata for %1: %2").arg(strFilename).arg( rclExc.what() ).toStdString() );
    }
    return true;
}

void MetadataWidget::clear()
{
    auto lst_blockers = blockAllFormSignals();
    m_pclUI->tagTypesLabel->clear();
    m_pclUI->trackArtistEdit->clear();
    m_pclUI->albumArtistEdit->clear();
    m_pclUI->titleEdit->clear();
    m_pclUI->albumEdit->clear();
    m_pclUI->trackSpin->clear();
    m_pclUI->yearSpin->clear();
    m_pclUI->cdEdit->clear();
    m_pclUI->totalTracksSpin->clear();
    m_pclUI->otherTagsList->clear();
    m_pclUI->coverLabel->clear();
    m_pclUI->coverInfoLabel->clear();
    setGenre("");
    m_pclUI->clearCoverButton->setEnabled(false);
    m_pclUI->clearOtherTagsCheck->setEnabled(false);
    m_pclUI->clearOtherTagsCheck->setChecked(false);
    m_pclFullResCover = nullptr;
    m_bIsModified = false;
}

void MetadataWidget::setTrackArtist(const QString & strArtist)
{
    m_pclUI->trackArtistEdit->setText(strArtist);
}

void MetadataWidget::setAlbumArtist(const QString & strArtist)
{
    m_pclUI->albumArtistEdit->setText(strArtist);
}

void MetadataWidget::setGenre( const QString& strGenre )
{
    int i_genre_idx = m_pclUI->genreCombo->findText( strGenre, Qt::MatchExactly );
    m_pclUI->genreCombo->setCurrentIndex(i_genre_idx);
    if ( i_genre_idx < 0 )
        m_pclUI->genreCombo->setEditText(strGenre);
}

void MetadataWidget::showCover()
{
    if ( m_pclFullResCover && !m_pclFullResCover->isNull() )
    {
        m_pclUI->coverLabel->setPixmap( m_pclFullResCover->scaled(m_pclUI->coverLabel->maximumSize(),Qt::KeepAspectRatio, Qt::SmoothTransformation) );
        m_pclUI->coverInfoLabel->setText( QString("%1x%2").arg(m_pclFullResCover->size().width()).arg(m_pclFullResCover->size().height()) );
        m_pclUI->clearCoverButton->setEnabled( true );
    }
    else
    {
        m_pclUI->coverLabel->clear();
        m_pclUI->coverInfoLabel->clear();
        m_pclUI->clearCoverButton->setDisabled( true );
    }
}

void MetadataWidget::setCover(const QPixmap & rclPixmap)
{
    m_pclFullResCover = std::make_unique<QPixmap>(rclPixmap);
    showCover();
    emit metadataModified();
}

void MetadataWidget::setYear(int iYear)
{
    m_pclUI->yearSpin->setValue(iYear);
}

void MetadataWidget::setAlbum(const QString & strAlbum)
{
    m_pclUI->albumEdit->setText(strAlbum);
}

void MetadataWidget::setTrackTitle(const QString & strTitle)
{
    m_pclUI->titleEdit->setText(strTitle);
}

void MetadataWidget::setTrackNumber(int iTrack)
{
    m_pclUI->trackSpin->setValue(iTrack);
}

void MetadataWidget::setTotalTracks(int iTotal)
{
    m_pclUI->totalTracksSpin->setValue(iTotal);
}

void MetadataWidget::setDiscNumber(const QString& strDisc)
{
    m_pclUI->cdEdit->setText(strDisc);
}

void MetadataWidget::clearOtherTags()
{
    m_pclUI->clearOtherTagsCheck->setChecked(true);
    emit metadataModified();
}

void MetadataWidget::clearCover()
{
    setCover(QPixmap());
}

void MetadataWidget::loadCover()
{
    QString str_cover_file = QFileDialog::getOpenFileName( this, "Load cover from file" );
    if ( str_cover_file.isNull() )
        return;
    QPixmap cl_pixmap;
    cl_pixmap.load( str_cover_file );
    setCover( cl_pixmap );
}

void MetadataWidget::setSingleTrack()
{
    m_pclUI->trackSpin->setValue(1);
    m_pclUI->totalTracksSpin->setValue(1);
    m_pclUI->cdEdit->setText("1");
    if ( !m_pclUI->albumEdit->text().isEmpty() )
        m_pclUI->titleEdit->setText(m_pclUI->albumEdit->text());
    if ( m_pclUI->albumArtistEdit->text().isEmpty() )
        m_pclUI->albumArtistEdit->setText( m_pclUI->trackArtistEdit->text() );
}

void MetadataWidget::googleCover()
{
    if ( m_pclUI->albumEdit->text().isEmpty() )
    {
        emit error( "cannot search for cover without an album title" );
        return;
    }
    QString str_query = m_pclUI->albumEdit->text();
    if ( !m_pclUI->albumArtistEdit->text().isEmpty() )
        str_query.prepend( m_pclUI->albumArtistEdit->text()+" " );
    str_query = QUrl::toPercentEncoding(str_query.replace( QRegExp("\\s"), "+" ));
    emit searchCoverOnline(QString("https://www.google.de/search?tbm=isch&tbs=iar:s&q=%1").arg( str_query ));
}

std::list<QSignalBlocker> MetadataWidget::blockAllFormSignals()
{
    std::list<QSignalBlocker> lst_blockers;
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->trackArtistEdit) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->albumArtistEdit) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->titleEdit) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->albumEdit) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->trackSpin) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->yearSpin) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->cdEdit) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->totalTracksSpin) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->otherTagsList) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->coverLabel) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->coverInfoLabel) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->genreCombo ) );
    lst_blockers.emplace_back( QSignalBlocker(m_pclUI->clearOtherTagsCheck ) );
    return lst_blockers;
}

TagLib::PropertyMap MetadataWidget::setMetadataInPropertyMap( TagLib::PropertyMap mapProperties ) const
{
    TagLib::PropertyMap map_properties;
    if ( !m_pclUI->clearOtherTagsCheck->isChecked() )
        map_properties = std::move( mapProperties ); //preserve existing properties
                
    map_properties[ Q2T(s_lstStandardTags.at(TrackArtist)) ] = Q2T(m_pclUI->trackArtistEdit->text());
    map_properties[ Q2T(s_lstExtendedTags.at(AlbumArtist)) ] = Q2T(m_pclUI->albumArtistEdit->text());
    map_properties[ Q2T(s_lstStandardTags.at(TrackTitle)) ]  = Q2T(m_pclUI->titleEdit->text());
    map_properties[ Q2T(s_lstStandardTags.at(Album)) ]       = Q2T(m_pclUI->albumEdit->text());
    map_properties[ Q2T(s_lstStandardTags.at(TrackNumber)) ] = Q2T(QString::number(m_pclUI->trackSpin->value()));
    map_properties[ Q2T(s_lstStandardTags.at(Year)) ]        = Q2T(QString::number(m_pclUI->yearSpin->value()));
    map_properties[ Q2T(s_lstExtendedTags.at(TotalTracks)) ] = Q2T(QString::number(m_pclUI->totalTracksSpin->value()));
    map_properties[ Q2T(s_lstStandardTags.at(Genre)) ]       = Q2T(m_pclUI->genreCombo->currentText());
    if ( m_pclUI->cdEdit->text().isEmpty() )
        map_properties[ Q2T(s_lstExtendedTags.at(DiscNumber)) ]  = Q2T("1");
    else
        map_properties[ Q2T(s_lstExtendedTags.at(DiscNumber)) ]  = Q2T(m_pclUI->cdEdit->text());
    
    return map_properties;
}
