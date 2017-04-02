#include "OnlineSourcesWidget.h"
#include <QNetworkAccessManager>
#include <QUrl>
#include <QMessageBox>
#include <QCheckBox>
#include "CoverDownloader.h"
#include "OnlineInfoSources.h"
#include "OnlineSourceParser.h"
#include "ui_OnlineSourcesWidget.h"

OnlineSourcesWidget::OnlineSourcesWidget(QWidget *pclParent)
: QWidget(pclParent)
, m_pclUI( std::make_unique<Ui::OnlineSourcesWidget>() )
, m_pclNetworkAccess( std::make_unique<QNetworkAccessManager>(nullptr) )
{
    m_pclCoverDownloader = std::make_unique<CoverDownloader>(m_pclNetworkAccess.get());
    m_pclUI->setupUi(this);
    connect( m_pclUI->sourceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showOnlineSource(int)) );
    connect( m_pclUI->applyTrackArtistButton, SIGNAL(clicked()), this, SLOT(applyTrackArtist()) );
    connect( m_pclUI->applyAlbumArtistButton, SIGNAL(clicked()), this, SLOT(applyAlbumArtist()) );
    connect( m_pclUI->applyAlbumButton, SIGNAL(clicked()), this, SLOT(applyAlbum()) );
    connect( m_pclUI->applyGenreButton, SIGNAL(clicked()), this, SLOT(applyGenre()) );
    connect( m_pclUI->applyYearButton, SIGNAL(clicked()), this, SLOT(applyYear()) );
    connect( m_pclUI->applyCoverButton, SIGNAL(clicked()), this, SLOT(applyCover()) );
    connect( m_pclUI->applyAllButton, SIGNAL(clicked()), this, SLOT(applyAll()) );
    connect( m_pclUI->genreList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(applyGenre()) );
    connect( m_pclUI->albumList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(applyAlbum()) );
    connect( m_pclCoverDownloader.get(), SIGNAL(imageReady()), this, SLOT(setCoverImageFromDownloader()), Qt::QueuedConnection );
    connect( m_pclCoverDownloader.get(), SIGNAL(error(QString)), this, SLOT(coverDownloadError(QString)), Qt::QueuedConnection );
    connect( m_pclUI->checkButton, SIGNAL(clicked()), this, SLOT(check()));
    m_pclUI->checkButton->setEnabled(false);
}

OnlineSourcesWidget::~OnlineSourcesWidget() = default;

void OnlineSourcesWidget::addParser( const QString& strName, std::shared_ptr<OnlineSourceParser> pclParser)
{
    if ( !pclParser )
        return;
    connect( pclParser.get(), SIGNAL(parsingFinished()), this, SLOT(addParsingResults()), Qt::QueuedConnection );
    m_mapParsers[strName] = std::move(pclParser);
    
    QCheckBox* pcl_check = new QCheckBox("check on "+strName);
    m_pclUI->parserLayout->addWidget( pcl_check );
    connect( pcl_check, &QCheckBox::stateChanged, [this,strName](int iChecked){ enableParser(strName,iChecked == Qt::Checked); } );
    pcl_check->setChecked(true);
    enableParser(strName,true);
}

void OnlineSourcesWidget::setGenreList(const QStringList &lstGenres)
{
    m_lstGenres = lstGenres;
    m_pclUI->genreList->setCurrentRow(highlightKnownGenres());
}

void OnlineSourcesWidget::clear()
{
    m_pclUI->sourceCombo->blockSignals(true);
    m_pclUI->sourceCombo->clear();
    m_pclUI->sourceCombo->blockSignals(false);
    clearFields();
    m_strArtist.clear();
    m_strAlbum.clear();
    m_strTrackTitle.clear();
    m_pclCoverDownloader->clear();
}

void OnlineSourcesWidget::addParsingResults()
{
    // get parser ptr
    m_pclUI->sourceCombo->blockSignals(true);
    m_pclUI->sourceCombo->clear();
    for ( const auto & rcl_parser : m_mapParsers )
        for ( const QString& str_page : rcl_parser.second->getPages() )
            m_pclUI->sourceCombo->addItem( str_page, rcl_parser.first );
    m_pclUI->sourceCombo->blockSignals(false);
    //show the first item
    showOnlineSource(0);
}

void OnlineSourcesWidget::showOnlineSource(int iIndex)
{
    clearFields();
    auto it_parser = m_mapParsers.find( m_pclUI->sourceCombo->itemData(iIndex).toString() );
    if ( it_parser == m_mapParsers.end() ) return;
    std::shared_ptr<OnlineInfoSource> pcl_source = it_parser->second->getResult(m_pclUI->sourceCombo->itemText(iIndex));
    fillArtistInfos( std::dynamic_pointer_cast<OnlineArtistInfoSource>(pcl_source) );
    fillAlbumInfos( std::dynamic_pointer_cast<OnlineAlbumInfoSource>(pcl_source) );
    
    if ( pcl_source && !pcl_source->getURL().isEmpty() )
        emit sourceURLchanged( QUrl( pcl_source->getURL() ) );
}

void OnlineSourcesWidget::coverDownloadError(QString strError)
{
    QMessageBox::critical( this, "Download Error", strError );
}

void OnlineSourcesWidget::setCoverImageFromDownloader()
{
    m_pclUI->coverLabel->setPixmap( m_pclCoverDownloader->getImage().scaled(m_pclUI->coverLabel->maximumSize(),Qt::KeepAspectRatio, Qt::SmoothTransformation) );
    m_pclUI->coverInfoLabel->setText( QString("%1x%2").arg(m_pclCoverDownloader->getImage().size().width()).arg(m_pclCoverDownloader->getImage().size().height()) );
    m_pclUI->applyCoverButton->setEnabled( true );
}

void OnlineSourcesWidget::applyTrackArtist()
{
    emit setTrackArtist( m_pclUI->trackArtistEdit->text() );
}

void OnlineSourcesWidget::applyAlbumArtist()
{
    emit setAlbumArtist( m_pclUI->albumArtistEdit->text() );
}

void OnlineSourcesWidget::applyGenre()
{
    auto pcl_genre_item = m_pclUI->genreList->currentItem();
    if ( pcl_genre_item )
        emit setGenre( pcl_genre_item->text() );
}

void OnlineSourcesWidget::applyCover()
{
    emit setCover( m_pclCoverDownloader->getImage() );
}

void OnlineSourcesWidget::applyYear()
{
    emit setYear( m_pclUI->yearEdit->text().toInt() );
}

void OnlineSourcesWidget::applyAlbum()
{
    auto pcl_album_item = m_pclUI->albumList->currentItem();
    if ( pcl_album_item )
        emit setAlbum( pcl_album_item->text() );
}

void OnlineSourcesWidget::applyAll()
{
    applyTrackArtist();
    applyAlbumArtist();
    applyGenre();
    applyCover();
    applyYear();
    applyAlbum();
}

void OnlineSourcesWidget::check()
{
    m_pclUI->sourceCombo->blockSignals(true);
    m_pclUI->sourceCombo->clear();
    m_pclUI->sourceCombo->blockSignals(false);
    for ( auto & rcl_parser_enabled : m_mapParserEnabled )
        if ( rcl_parser_enabled.second )
        {
            try
            {
                m_mapParsers.at(rcl_parser_enabled.first)->sendRequests( m_strArtist, m_strAlbum, m_strTrackTitle );
            } catch (const std::exception& rclExc ) {
                QMessageBox::critical( this, "Query Error", rclExc.what() );
            }
        }
}

void OnlineSourcesWidget::enableParser(const QString &strName, bool bEnabled)
{
    m_mapParserEnabled[strName] = bEnabled;
    m_pclUI->checkButton->setEnabled( bEnabled || std::count_if( m_mapParserEnabled.begin(), m_mapParserEnabled.end(), [](const auto & rcl_item){ return rcl_item.second; } ) > 0 );
}

template<typename GuiElemT>
static void clearAndDisable( GuiElemT* pclItem, QPushButton* pclButton)
{
    pclItem->clear();
    pclButton->setDisabled(true);
}

static void fillAndEnable( const QStringList& lstContent, QListWidget* pclList, QPushButton* pclButton, std::function<int()> funSelector = {} )
{
    if ( !lstContent.isEmpty() )
    {
        pclList->addItems( lstContent );
        if ( funSelector )
            pclList->setCurrentRow(funSelector());
    }
    pclButton->setDisabled( pclList->count() == 0 );
}

static void fillAndEnable( const QString& strContent, QLineEdit* pclEdit, QPushButton* pclButton )
{
    pclEdit->setText( strContent );
    pclButton->setDisabled(pclEdit->text().isEmpty());
}

void OnlineSourcesWidget::fillArtistInfos(std::shared_ptr<OnlineArtistInfoSource> pclSource)
{
    if ( !pclSource )
        return;
    fillAndEnable( pclSource->getArtist(), m_pclUI->trackArtistEdit, m_pclUI->applyTrackArtistButton );
    fillAndEnable( pclSource->getArtist(), m_pclUI->albumArtistEdit, m_pclUI->applyAlbumArtistButton );
    fillAndEnable( pclSource->getGenres(), m_pclUI->genreList,       m_pclUI->applyGenreButton, [this]{ return highlightKnownGenres(); } );
}

void OnlineSourcesWidget::fillAlbumInfos(std::shared_ptr<OnlineAlbumInfoSource> pclSource)
{
    if ( !pclSource )
        return;
    fillAndEnable( pclSource->getYear(),        m_pclUI->yearEdit,        m_pclUI->applyYearButton );
    fillAndEnable( pclSource->getAlbumArtist(), m_pclUI->trackArtistEdit, m_pclUI->applyTrackArtistButton );
    fillAndEnable( pclSource->getAlbumArtist(), m_pclUI->albumArtistEdit, m_pclUI->applyAlbumArtistButton );
    fillAndEnable( pclSource->getGenres(),      m_pclUI->genreList,       m_pclUI->applyGenreButton, [this]{ return highlightKnownGenres(); } );
    fillAndEnable( pclSource->getAlbums(),      m_pclUI->albumList,       m_pclUI->applyAlbumButton, []{ return 0; } );
    
    if ( pclSource && !pclSource->getCover().isEmpty() )
        m_pclCoverDownloader->downloadImage( pclSource->getCover() );
}

void OnlineSourcesWidget::clearFields()
{
    clearAndDisable( m_pclUI->trackArtistEdit, m_pclUI->applyTrackArtistButton );
    clearAndDisable( m_pclUI->albumArtistEdit, m_pclUI->applyAlbumArtistButton );
    clearAndDisable( m_pclUI->genreList,       m_pclUI->applyGenreButton );
    clearAndDisable( m_pclUI->yearEdit,        m_pclUI->applyYearButton );
    clearAndDisable( m_pclUI->albumList,       m_pclUI->applyAlbumButton );
    m_pclUI->coverLabel->clear();
    m_pclUI->coverInfoLabel->clear();
    m_pclCoverDownloader->clear();
    m_pclUI->applyCoverButton->setDisabled( true );
}

int OnlineSourcesWidget::highlightKnownGenres()
{
    int i_found = -1;
    QFont cl_font;
    for ( int i = 0; i < m_pclUI->genreList->count(); ++i )
    {
        QListWidgetItem* pcl_item = m_pclUI->genreList->item(i);
        if ( m_lstGenres.contains( pcl_item->text() ) )
        {
            if ( i_found < 0 )
            {
                i_found = i;
                cl_font = pcl_item->font();
                cl_font.setBold(true);
            }
            pcl_item->setFont(cl_font);
        }
    }
    return std::max( i_found, 0 );
}

void OnlineSourcesWidget::setArtistQuery( const QString& strArtist )
{
    m_strArtist = strArtist;
}

void OnlineSourcesWidget::setAlbumQuery( const QString& strAlbum )
{
    m_strAlbum = strAlbum;
}

void OnlineSourcesWidget::setTitleQuery( const QString& strTrackTitle )
{
    m_strTrackTitle = strTrackTitle;
}

