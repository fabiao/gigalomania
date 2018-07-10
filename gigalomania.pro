TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES -= UNICODE

SOURCES += game.cpp gamestate.cpp gui.cpp image.cpp main.cpp panel.cpp player.cpp resources.cpp screen.cpp sector.cpp sound.cpp tutorial.cpp utils.cpp TinyXML/tinyxml.cpp TinyXML/tinyxmlerror.cpp TinyXML/tinyxmlparser.cpp

win32 {
    # update this to match where SDL2 includes are installed on your system
    INCLUDEPATH += ../APIs/SDL2/include/
    INCLUDEPATH += ../APIs/SDL2_image/include/
    INCLUDEPATH += ../APIs/SDL2_mixer/include/
}

LIBS += -L$$PWD # add the source folder for libs
LIBS += -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer
win32 {
    LIBS += -lUser32 -lShell32
}

dir1.source = gfx
dir2.source = sound
dir3.source = music
dir4.source = islands
DEPLOYMENTFOLDERS += dir1 dir2 dir3 dir4

include(deployment.pri)
qtcAddDeployment()
