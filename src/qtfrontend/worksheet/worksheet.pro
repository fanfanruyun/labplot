include(../config.pri)
TEMPLATE = lib
CONFIG += plugin static
DEPENDPATH += . .. ../../backend ../../commonfrontend ../../commonfrontend/worksheet ../../backend/worksheet ../core ../../backend/core
INCLUDEPATH += .. ../../backend ../../commonfrontend
TARGET = ../$$qtLibraryTarget(scidavis_worksheet)
QT += xml

debug {
	CONFIG -= static
	DEFINES += QT_STATICPLUGIN
}

#FORMS += controltabs.ui \

HEADERS += \
	WorksheetModule.h \
	WorksheetView.h \
	WorksheetModel.h \
	WorksheetGraphicsScene.h \
	Worksheet.h \
	AbstractWorksheetElement.h \
	AbstractPlot.h \
	DecorationPlot.h \
	WorksheetElementGroup.h \
	WorksheetRectangleElement.h \
	
SOURCES += \
	WorksheetModule.cpp \
	WorksheetView.cpp \
	Worksheet.cpp \
	WorksheetModel.cpp \
	WorksheetGraphicsScene.cpp \
	AbstractWorksheetElement.cpp \
	AbstractPlot.cpp \
	DecorationPlot.cpp \
	WorksheetElementGroup.cpp \
	WorksheetRectangleElement.cpp \

