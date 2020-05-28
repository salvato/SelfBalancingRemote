QT += core
QT += gui
QT += multimedia
QT += widgets


CONFIG += c++11


DEFINES += QT_DEPRECATED_WARNINGS


SOURCES += \
    AxisFrame.cpp \
    AxisLimits.cpp \
    DataSetProperties.cpp \
    GLwidget.cpp \
    axesdialog.cpp \
    datastream2d.cpp \
    geometryengine.cpp \
    main.cpp \
    mainwidget.cpp \
    plot2d.cpp \
    plotpropertiesdlg.cpp \
    utilities.cpp

HEADERS += \
    AxisFrame.h \
    AxisLimits.h \
    DataSetProperties.h \
    GLwidget.h \
    axesdialog.h \
    datastream2d.h \
    geometryengine.h \
    mainwidget.h \
    plot2d.h \
    plotpropertiesdlg.h \
    utilities.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DISTFILES += \
    10_DOF.png \
    LICENSE \
    README.md \
    cube.png \
    fshader.glsl \
    plot.png \
    vshader.glsl

RESOURCES += \
    resources.qrc \
    shaders.qrc \
    textures.qrc
