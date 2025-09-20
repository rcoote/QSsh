include(../qssh.pri)

QT += gui widgets sql

TARGET=hostmanager
SOURCES+=main.cpp \
    HostManagerWindow.cpp \
    argumentscollector.cpp \
    remoteprocesstest.cpp \
    treeitem.cpp \
    treemodel.cpp

HEADERS+=\
    HostManagerWindow.h \
    argumentscollector.h \
    remoteprocesstest.h \
    treeitem.h \
    treemodel.h

RESOURCES += \
    resources.qrc

FORMS += \
    HostManagerWindow.ui
