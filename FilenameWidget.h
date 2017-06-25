#ifndef FILENAMEWIDGET_H
#define FILENAMEWIDGET_H

#include <QWidget>
#include <memory>

namespace Ui {
class FilenameWidget;
}

class FilenameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FilenameWidget(QWidget* pclParent = nullptr);
    ~FilenameWidget() override;
    
    // for format, use formatters %N, %A, %T, %B, %C
    void addFilenameFormat( const QString& strTitle, const QString& strFormat );
    void addDestinationDirectoryFormat( const QString& strTitle, const QString& strFormat );
    void setDestinationBaseDirectory( const QString& strDirectory );
    
    bool isModified() const;
       
    QString filename() const;
    QString destinationSubdirectory() const;
    const QString& destinationBaseDirectory() const { return m_strDestinationBaseDirectory; }
    
public slots:
    void setFilename(const QString& strFilename);
    bool applyToFile(const QString& strFilePath); ///returns true on success
    void clear();
    
    void setTrackArtist( const QString& strArtist );
    void setAlbumArtist( const QString& strArtist );
    void setAlbum( const QString& strAlbum );
    void setTitle( const QString& strTitle );
    void setTrackNumber( int iNumber );

signals:
    void filenameModified();
    
protected slots:
    void formatterClicked();
    void destinationClicked();
    void onTextEdited(const QString& strFilename);
    
protected:
    bool filenameWithoutInvalidCharacters(const QString& strFilename);
    bool updateFilenameIfNecessary();
    
private:
    std::unique_ptr<Ui::FilenameWidget> m_pclUI;
    std::map<class QPushButton*,QString> m_mapFilenameFormatters;
    std::map<class QPushButton*,QString> m_mapDestinationFormatters;
    
    QString m_strTrackArtist, m_strAlbumArtist, m_strTitle, m_strAlbum;
    int m_iNumber{-1};
    
    bool m_bIsModified{false};
    
    QString m_strDestinationBaseDirectory;
};

#endif // FILENAMEWIDGET_H
