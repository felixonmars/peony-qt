INCLUDEPATH += $$PWD

HEADERS += $$PWD/peony-core_global.h \
           $$PWD/file-info.h \
           $$PWD/file-info-job.h \
           $$PWD/file-info-manager.h \
           $$PWD/file-enumerator.h \
           $$PWD/mount-operation.h \
           $$PWD/file-watcher.h \
           $$PWD/connect-server-dialog.h \
    $$PWD/volume-manager.h \
    $$PWD/gerror-wrapper.h \
    $$PWD/gobject-template.h \
    $$PWD/file-utils.h

SOURCES += $$PWD/file-info.cpp \
           $$PWD/file-info-job.cpp \
           $$PWD/file-info-manager.cpp \
           $$PWD/file-enumerator.cpp \
           $$PWD/mount-operation.cpp \
           $$PWD/file-watcher.cpp \
           $$PWD/connect-server-dialog.cpp \
    $$PWD/volume-manager.cpp \
    $$PWD/gerror-wrapper.cpp \
    $$PWD/gobject-template.cpp \
    $$PWD/file-utils.cpp

FORMS += $$PWD/connect-server-dialog.ui
