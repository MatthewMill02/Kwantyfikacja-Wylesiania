cmake_minimum_required(VERSION 3.16)

project(Wylesianie VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Charts)


find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Charts)


set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    models/analysisrequest.cpp
    models/analysisrequest.h
    models/analysisresult.cpp
    models/analysisresult.h
    models/bandbuffer.cpp
    models/bandbuffer.h

    network/backendclient.cpp
    network/backendclient.h
    network/analysisresponseparser.cpp
    network/analysisresponseparser.h

    processing/imagerenderer.cpp
    processing/imagerenderer.h
    processing/mockdatagenerator.cpp
    processing/mockdatagenerator.h

    utils/geepixelbounds.cpp
    utils/geepixelbounds.h

    widgets/imagepanelwidget.cpp
    widgets/imagepanelwidget.h
    widgets/resultsviewwidget.cpp
    widgets/resultsviewwidget.h
    widgets/splitpanelwidget.cpp
    widgets/splitpanelwidget.h
    widgets/osmmapwidget.cpp
    widgets/osmmapwidget.h

    theme/apptheme.cpp
    theme/apptheme.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Wylesianie
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(Wylesianie SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(Wylesianie
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(Wylesianie PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Wylesianie PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network


    Qt${QT_VERSION_MAJOR}::Charts


)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Wylesianie)
endif()
set_target_properties(Wylesianie PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Wylesianie
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Wylesianie)
endif()
