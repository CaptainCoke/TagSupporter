#ifndef ONLINESOURCEPARSER_H
#define ONLINESOURCEPARSER_H

#include <QObject>
#include <memory>
#include <functional>
#include <QNetworkRequest>

class OnlineInfoSource;
class QNetworkAccessManager;

class OnlineSourceParser : public QObject
{
    Q_OBJECT
public:
    explicit OnlineSourceParser( QNetworkAccessManager *pclNetworkAccess, QObject *pclParent = nullptr );
    ~OnlineSourceParser() override;
    
    virtual void sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle, int iYear ) = 0;
    virtual void clearResults() = 0;
    virtual QStringList getPages() const = 0;
    virtual std::shared_ptr<OnlineInfoSource> getResult( const QString& strPage ) const = 0;
signals:
    void error(QString);
    void info(QString);
    void parsingFinished(QStringList); // emits string list with recently added results
    void cancelAllPendingNetworkRequests();
    void sendQuery( QNetworkRequest clRequest, QString strReceivingSlot );
    
protected slots:
    virtual void onSendQuery( QNetworkRequest clRequest, QString strReceivingSlot );
    
protected:
    void startParserThread( QByteArray&& strReply, std::function<void(QByteArray)>&& funWork );
    
private:
    QNetworkAccessManager* m_pclNetworkAccess = nullptr;
};

#endif // ONLINESOURCEPARSER_H
