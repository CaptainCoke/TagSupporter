#include "PlaybackWidget.h"
#include <QMessageBox>
#include <QTime>
#include "ui_PlaybackWidget.h"

PlaybackWidget::PlaybackWidget(QWidget *pclParent)
: QWidget( pclParent )
, m_pclUI( std::make_unique<Ui::PlaybackWidget>() )
, m_pclMediaPlayer( std::make_unique<QMediaPlayer>() )
{
    m_pclUI->setupUi(this);
    connect( m_pclUI->playButton, SIGNAL(clicked()), m_pclMediaPlayer.get(), SLOT(play()) );
    connect( m_pclUI->stopButton, SIGNAL(clicked()), m_pclMediaPlayer.get(), SLOT(stop()) );
    connect( m_pclUI->positionSlider, SIGNAL(valueChanged(int)), this, SLOT(setPlayerPosition(int)) );
    connect( m_pclMediaPlayer.get(), SIGNAL(positionChanged(qint64)), this, SLOT(playerPositionChanged(qint64)) );
    connect( m_pclMediaPlayer.get(), SIGNAL(error(QMediaPlayer::Error)), this, SLOT(mediaPlayerError(QMediaPlayer::Error)) );
    connect( m_pclMediaPlayer.get(), SIGNAL(durationChanged(qint64)), this, SLOT(playerDurationChanged(qint64)) );
    
    
    m_pclMediaPlayer->setVolume(100);
    m_pclUI->playButton->setEnabled(false);
    m_pclUI->stopButton->setEnabled(false);
    m_pclUI->positionSlider->setEnabled(false);
}

PlaybackWidget::~PlaybackWidget() = default;

void PlaybackWidget::setSource(const QUrl &strFileURL)
{
    m_pclUI->trackTimeLabel->clear();
    m_pclMediaPlayer->setMedia(strFileURL);
}


static QString formatPlaybackTime( const QMediaPlayer & rclPlayer )
{
    QString str_pos      = QTime(0,0).addMSecs(rclPlayer.position()).toString("m:ss");
    QString str_duration = QTime(0,0).addMSecs(rclPlayer.duration()).toString("m:ss");
    return QString( "%1/%2" ).arg(str_pos,str_duration);
}

void PlaybackWidget::playerPositionChanged(qint64 iPos)
{
    m_pclUI->positionSlider->blockSignals(true);
    m_pclUI->positionSlider->setValue(iPos);
    m_pclUI->positionSlider->blockSignals(false);
    m_pclUI->trackTimeLabel->setText( formatPlaybackTime( *m_pclMediaPlayer) );
}

void PlaybackWidget::playerDurationChanged(qint64 iDuration)
{
    m_pclUI->positionSlider->blockSignals(true);
    m_pclUI->positionSlider->setRange(0,iDuration);
    m_pclUI->positionSlider->blockSignals(false);
    m_pclUI->trackTimeLabel->setText( formatPlaybackTime( *m_pclMediaPlayer) );
    if ( iDuration > 0 )
    {
        m_pclUI->playButton->setEnabled(true);
        m_pclUI->stopButton->setEnabled(true);
        m_pclUI->positionSlider->setEnabled(true);
    }
}

void PlaybackWidget::setPlayerPosition(int iPos)
{
    m_pclMediaPlayer->blockSignals(true);
    m_pclMediaPlayer->setPosition(iPos);
    m_pclMediaPlayer->blockSignals(false);
    m_pclUI->trackTimeLabel->setText( formatPlaybackTime( *m_pclMediaPlayer) );
}

void PlaybackWidget::mediaPlayerError(QMediaPlayer::Error error)
{
    QString str_message;
    switch( error )
    {
    case QMediaPlayer::NoError: return;
    case QMediaPlayer::ResourceError: str_message = "A media resource couldn't be resolved."; break;
    case QMediaPlayer::FormatError: str_message = "The format of a media resource isn't (fully) supported. Playback may still be possible, but without an audio or video component."; break;
    case QMediaPlayer::NetworkError: str_message = "A network error occurred."; break;
    case QMediaPlayer::AccessDeniedError: str_message = "There are not the appropriate permissions to play a media resource."; break;
    case QMediaPlayer::ServiceMissingError: str_message = "A valid playback service was not found, playback cannot proceed."; break;
    default: str_message = "unknown media player error";        
    }
    QMessageBox::critical( this, "Playback Error", str_message );
}
