#-------------------------------------------------
#
# Project created by QtCreator 2020-04-14T22:02:44
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ffmpeg2
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    OMXAudio.cpp \
    camera.cpp \
        main.cpp \
        mainwindow.cpp \
    OMXH264Player.cpp \
    dialog.cpp \
    GLYuvWidget.cpp \
    YUV420P_Render.cpp

HEADERS += \
    OMXAudio.h \
    camera.h \
        mainwindow.h \
    OMXH264Player.h \
    dialog.h \
    GLYuvWidget.h \
    YUV420P_Render.h

FORMS += \
        mainwindow.ui \
    dialog.ui

#INCLUDEPATH += /home/liuc/work/sysroot/opt/vc/include/
#INCLUDEPATH += /home/liuc/work/sysroot/opt/vc/src/hello_pi/libs/ilclient
#INCLUDEPATH += /usr/local/ffmpeg/include
#LIBS += -L/home/liuc/work/sysroot/opt/vc/lib
#LIBS += -L/home/liuc/work/sysroot/opt/vc/src/hello_pi/libs/ilclient \
#        -lilclient -lvcos -lopenmaxil -lbcm_host -lvchiq_arm -lpthread -lEGL -lGLESv2
#LIBS += -L/usr/local/ffmpeg/lib -lavformat -lavdevice -lavcodec -lavutil -lswresample -lswscale -lpthread -ldl -lz -lm
PI_SYSROOT=/home/liuc/work/sysroot/
INCLUDEPATH += $${PI_SYSROOT}/opt/vc/include
INCLUDEPATH += $${PI_SYSROOT}/opt/vc/src/hello_pi/libs/ilclient
INCLUDEPATH += $${PI_SYSROOT}/usr/local/ffmpeg/include
INCLUDEPATH += $${PI_SYSROOT}/usr/local/glog/include
#INCLUDEPATH += $${PI_SYSROOT}/usr/local/opencv/include

LIBS += -L$${PI_SYSROOT}/usr/local/ffmpeg/lib -lavdevice -lxcb -lxcb-shm -lasound -lm -ldl -lz -pthread -lavfilter -lxcb -lxcb-shm -lasound -lm -ldl -lz -pthread -lpostproc -lavformat -lxcb -lxcb-shm -lasound -lm -ldl -lz -pthread -lavcodec -lxcb -lxcb-shm -lasound -lm -ldl -lz -pthread -lswscale -lm -lswresample -lm -lavutil -lm
LIBS += -L$${PI_SYSROOT}/usr/local/glog/lib -lglog
#LIBS += -L$${PI_SYSROOT}/usr/local/libburn/lib -lburn_device
#LIBS += -L$${PI_SYSROOT}/usr/local/opencv/lib -lopencv_world
LIBS += -L$${PI_SYSROOT}/opt/vc/lib -lopenmaxil -lbcm_host
LIBS += -L$${PI_SYSROOT}/opt/vc/src/hello_pi/libs/ilclient -lilclient -lvcos

#INCLUDEPATH += /opt/vc/include/ /opt/vc/src/hello_pi/libs/ilclient
#LIBS += -L/opt/vc/lib -lopenmaxil -lbcm_host  -L/opt/vc/src/hello_pi/libs/ilclient -lilclient -lvcos `pkg-config libavcodec libavdevice libavfilter libavformat libavutil libswscale libswresample --libs`
#LIBS += -lopencv_world

QMAKE_LFLAGS += -Wl,-rpath=./

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
