QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets websockets

CONFIG += c++11

win32 {
	QMAKE_CXXFLAGS += -execution-charset:utf-8
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	External/zlib/adler32.c \
	External/zlib/compress.c \
	External/zlib/crc32.c \
	External/zlib/deflate.c \
	External/zlib/infback.c \
	External/zlib/inffast.c \
	External/zlib/inflate.c \
	External/zlib/inftrees.c \
	External/zlib/trees.c \
	External/zlib/uncompr.c \
	External/zlib/zutil.c \
	External/zlib/gzlib.c \
    ConnectionDialog.cpp \
    ImageWidget.cpp \
    SocketMessage.cpp \
    WebSocketApp.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
	External/zlib/crc32.h \
	External/zlib/deflate.h \
	External/zlib/inffast.h \
	External/zlib/inffixed.h \
	External/zlib/inflate.h \
	External/zlib/inftrees.h \
	External/zlib/trees.h \
	External/zlib/zconf.h \
	External/zlib/zlib.h \
	External/zlib/zutil.h \
    ConnectionDialog.h \
    ImageWidget.h \
    MainWindow.h \
    SocketMessage.h \
    WebSocketApp.h

FORMS += \
    ConnectionDialog.ui \
    MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
