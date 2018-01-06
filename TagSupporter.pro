#-------------------------------------------------
#
# Project created by QtCreator 2017-01-31T20:37:25
#
#-------------------------------------------------

QT       += core gui network webenginewidgets multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TagSupporter
TEMPLATE = app


SOURCES += main.cpp\
        TagSupporter.cpp \
    WikipediaParser.cpp \
    WikipediaInfoSources.cpp \
    CoverDownloader.cpp \
    OnlineSourcesWidget.cpp \
    PlaybackWidget.cpp \
    MetadataWidget.cpp \
    FilenameWidget.cpp \
    AmarokDatabaseWidget.cpp \
    EmbeddedSQLConnection.cpp \
    WebBrowserWidget.cpp \
    DiscogsParser.cpp \
    DiscogsInfoSources.cpp \
    StringDistance.cpp \
    OnlineInfoSources.cpp \
    OnlineSourceParser.cpp

HEADERS  += TagSupporter.h \
    WikipediaParser.h \
    OnlineInfoSources.h \
    WikipediaInfoSources.h \
    CoverDownloader.h \
    OnlineSourcesWidget.h \
    OnlineSourceParser.h \
    PlaybackWidget.h \
    MetadataWidget.h \
    FilenameWidget.h \
    AmarokDatabaseWidget.h \
    EmbeddedSQLConnection.h \
    WebBrowserWidget.h \
    DiscogsParser.h \
    DiscogsInfoSources.h \
    StringDistance.h

FORMS    += TagSupporter.ui \
    OnlineSourcesWidget.ui \
    PlaybackWidget.ui \
    MetadataWidget.ui \
    FilenameWidget.ui \
    AmarokDatabaseWidget.ui \
    WebBrowserWidget.ui

LIBS += -lmysqld -lpthread -lm -lrt -latomic -lcrypt -ldl -laio -llz4 -lnuma -ltag -lz 


QMAKE_CXXFLAGS += -fopenmp
LIBS += -fopenmp
