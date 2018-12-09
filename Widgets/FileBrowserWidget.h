#ifndef FILEBROWSERWIDGET_H
#define FILEBROWSERWIDGET_H

#include <QWidget>
#include <memory>

namespace Ui {
    class FileBrowserWidget;
}

class QListWidgetItem;

class FileBrowserWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FileBrowserWidget( QWidget *pclParent = nullptr );
    ~FileBrowserWidget() override;

    void scanFolder(QString strFolder);
    QString getLastUsedFolder() const;

signals:
    void noFileSelected();
    void fileSelected( QString );
    void folderChanged( QString );
    void saveFile(QString);

public slots:
    void setFileModified(bool bModified = true);
    void currentFileMoved( const QString& strNewFilename, const QString& strNewFolder );

protected slots:
    void browseForFolder();
    void saveCurrent();
    void deleteCurrent();
    void selectNextFile();

    void switchFile(QListWidgetItem* pclCurrent, QListWidgetItem* pclPrevious);

protected:
    void setFileModified(QListWidgetItem* pclItem, bool bModified);
    void updateTotalFileCountLabel();
    void setLastUsedFolder( QString folder ) const;
    bool isModified(QListWidgetItem *pclItem);

private:
    std::unique_ptr<Ui::FileBrowserWidget> m_pclUI;   
};

#endif
