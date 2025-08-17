include(../qssh.pri)

QT += gui widgets sql

TARGET=hostmanager
SOURCES+=main.cpp window.cpp \
    treeitem.cpp \
    treemodel.cpp
HEADERS+=window.h \
    treeitem.h \
    treemodel.h
FORMS=window.ui \
    mainwindow.ui

RESOURCES += \
    resources.qrc
