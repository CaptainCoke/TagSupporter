#include "TagSupporter.h"
#include <QApplication>
#include <QFile>
#include <QUrl>
#include <QNetworkRequest>
#include <QRegularExpressionMatchIterator>
#include <iostream>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<QNetworkRequest>("QNetworkRequest");

    QCoreApplication::setOrganizationName("Christopher Schwartz");
    QCoreApplication::setOrganizationDomain("christopherschwartz.de");
    QCoreApplication::setApplicationName("TagSupporter");

    QApplication a(argc, argv);
    TagSupporter w;
    w.showMaximized();

    // browse for folder, if given
    if ( argc > 1 )
        w.scanFolder( argv[1] );
    else if ( QString str_last_folder = w.getLastUsedFolder(); !str_last_folder.isEmpty() )
        w.scanFolder( str_last_folder );
    return a.exec();
}

/*// PARSER TESTING w/o UI
#include "DiscogsParser.h"
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QUrl>

class ParserTest : public DiscogsParser
{
public:
    using DiscogsParser::DiscogsParser;
    using DiscogsParser::parseSearchResult;
    using DiscogsParser::parseContent;
};

int main(int argc, char *argv[])
{   
    QFile cl_file( "/home/christopher/Blümchen - Boomerang.txt" );
    cl_file.open(QFile::ReadOnly);
    QByteArray html = cl_file.readAll();
    cl_file.close();
    
    QNetworkAccessManager cl_network_access;
    ParserTest cl_parser(&cl_network_access,nullptr);
    cl_parser.parseFromURL(QUrl("https://www.discogs.com/Bl%C3%BCmchen-Boomerang/release/23697"));
    cl_parser.parseFromURL(QUrl("https://www.discogs.com/artist/20156-Bl%C3%BCmchen"));
    cl_parser.parseFromURL(QUrl("https://www.discogs.com/de/Bl%C3%BCmchen-Herzfrequenz/master/89728"));
    cl_parser.parseFromURL(QUrl("https://www.discogs.com/de/Bl%C3%BCmchen-Herzfrequenz/release/23696"));
    cl_parser.parseFromURL(QUrl("https://www.discogs.com/de/Various-Bravo-Hits-13/master/963781"));
    cl_parser.parseContent( html, QUrl("https://api.discogs.com/releases/23697") );
    
    return 0;
}
*/
