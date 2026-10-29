QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    phonetickeyboard.cpp \
    phoneticlineedit.cpp \
    DictEngine/dictzipreader.cpp \
    DictEngine/phoneticsearchengine.cpp \
    DictEngine/phoneticlistmodel.cpp \
    DictEngine/wordsearchengine.cpp \
    DictEngine/worddefinitionengine.cpp \
    DictEngine/wordlistmodel.cpp

HEADERS += \
    mainwindow.h \
    phonetickeyboard.h \
    phoneticlineedit.h \
    DictEngine/dictzipreader.h \
    DictEngine/phoneticsearchengine.h \
    DictEngine/phoneticlistmodel.h \
    DictEngine/wordsearchengine.h \
    DictEngine/worddefinitionengine.h \
    DictEngine/wordlistmodel.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
