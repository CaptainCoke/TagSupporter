#ifndef TAGSUPPORTER_H
#define TAGSUPPORTER_H

#include <QMainWindow>
#include <memory>

namespace Ui {
    class TagSupporter;
}
class QListWidgetItem;

class TagSupporter : public QMainWindow
{
    Q_OBJECT

public:
    explicit TagSupporter(QWidget *parent = nullptr);
    ~TagSupporter() override;

    void scanFolder(const QString& strFolder);

protected slots:
    void metadataError(QString);
    void infoParsingError(QString);
    void infoParsingInfo(QString);
    void databaseError(QString);
    
    void fileModified();
    void browseForFolder();
    void selectFile(const QString& strFullFilePath);
    void saveCurrent();
    void deleteCurrent();
    void selectNextFile();
    
    void switchFile(QListWidgetItem* pclCurrent, QListWidgetItem* pclPrevious);
    
protected:
    void updateTotalFileCountLabel();
    
private:
    std::unique_ptr<Ui::TagSupporter>            m_pclUI;
    std::shared_ptr<class WikipediaParser>       m_pclGermanWikipediaParser, m_pclEnglishWikipediaParser;
    std::shared_ptr<class DiscogsParser>         m_pclDiscogsParser;
    std::unique_ptr<class QNetworkAccessManager> m_pclNetworkAccess;
    std::shared_ptr<class EmbeddedSQLConnection> m_pclDB;
    std::list<class TemporaryRecursiveCopy>      m_lstTemporaryFiles;
    
    QString                                      m_strLastScannedFolder;
};

#endif // TAGSUPPORTER_H
