#ifndef PLAYBACKWIDGETGET_H
#define PLAYBACKWIDGETGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <memory>

namespace Ui {
class PlaybackWidget;
}

class PlaybackWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaybackWidget(QWidget* pclParent = 0);
    ~PlaybackWidget() override;
    
signals:
    void durationChanged(int);
    
public slots:
    void setSource(const QUrl& strFileURL);
    
protected slots:
    void playerPositionChanged(qint64 iPos);
    void playerDurationChanged(qint64 iDuration);
    void setPlayerPosition(int iPos);
    void mediaPlayerError(QMediaPlayer::Error error);

private:
    std::unique_ptr<Ui::PlaybackWidget> m_pclUI;
    std::unique_ptr<QMediaPlayer>       m_pclMediaPlayer;
};

#endif // PLAYBACKWIDGETGET_H
