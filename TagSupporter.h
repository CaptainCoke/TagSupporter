#ifndef TAGSUPPORTER_H
#define TAGSUPPORTER_H

#include <QMainWindow>
#include <memory>

namespace Ui {
    class TagSupporter;
}
class TagSupporter : public QMainWindow
{
    Q_OBJECT

public:
    explicit TagSupporter(QWidget *parent = nullptr);
    ~TagSupporter() override;

    void scanFolder(QString strFolder);
    QString getLastUsedFolder() const;

protected slots:
    void metadataError(QString);
    void infoParsingError(QString);
    void infoParsingInfo(QString);
    void databaseError(QString);

    void noFileSelected();
    void fileSelected(const QString& strFullFilePath);
    void saveFile(const QString& strFullFilePath);

private:
    std::unique_ptr<Ui::TagSupporter>            m_pclUI;
    std::shared_ptr<class WikipediaParser>       m_pclGermanWikipediaParser, m_pclEnglishWikipediaParser;
    std::shared_ptr<class DiscogsParser>         m_pclDiscogsParser;
    std::unique_ptr<class QNetworkAccessManager> m_pclNetworkAccess;
    std::shared_ptr<class EmbeddedSQLConnection> m_pclDB;
    std::list<class TemporaryRecursiveCopy>      m_lstTemporaryFiles;
};

#endif // TAGSUPPORTER_H
