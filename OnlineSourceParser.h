#ifndef ONLINESOURCEPARSER_H
#define ONLINESOURCEPARSER_H

#include <QObject>
#include <memory>

class OnlineInfoSource;

class OnlineSourceParser : public QObject
{
    Q_OBJECT
public:
    explicit OnlineSourceParser( QObject *pclParent = nullptr ) : QObject( pclParent ) {}
    ~OnlineSourceParser() override = default;
    
    virtual void sendRequests( const QString& trackArtist, const QString& trackTitle, const QString& albumTitle ) = 0;
    virtual void clearResults() = 0;
    virtual QStringList getPages() const = 0;
    virtual std::shared_ptr<OnlineInfoSource> getResult( const QString& strPage ) const = 0;
signals:
    void error(QString);
    void info(QString);
    void parsingFinished(QStringList); // emits string list with recently added results
};

#endif // ONLINESOURCEPARSER_H
