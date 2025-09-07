include(../qssh.pri)

QT += gui widgets sql

TARGET=hostmanager
SOURCES+=main.cpp \
    HostManagerWindow.cpp \
    treeitem.cpp \
    treemodel.cpp
HEADERS+=\
    HostManagerWindow.h \
    treeitem.h \
    treemodel.h

RESOURCES += \
    resources.qrc

FORMS += \
    HostManagerWindow.ui
