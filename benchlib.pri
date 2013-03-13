INCLUDEPATH += $$PWD/src
HEADERS +=       $$PWD/src/database.h  $$PWD/src/reportgenerator.h 
SOURCES +=  $$PWD/src/database.cpp  $$PWD/src/reportgenerator.cpp 


CONFIG += console release
CONFIG -= app_bundle
RESOURCES = $$PWD/src/benchlib.qrc
OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
