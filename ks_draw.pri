INCLUDEPATH += $${PWD}

# ks
PATH_KS_DRAW = $${PWD}/ks/draw

HEADERS += \
    $${PATH_KS_DRAW}/KsDrawSystem.hpp \
    $${PATH_KS_DRAW}/KsDrawScene.hpp \
    $${PATH_KS_DRAW}/KsDrawDrawStage.hpp \
    $${PATH_KS_DRAW}/KsDrawComponents.hpp \
    $${PATH_KS_DRAW}/KsDrawRenderStats.hpp \
    $${PATH_KS_DRAW}/KsDrawDefaultDrawStage.hpp \
    $${PATH_KS_DRAW}/KsDrawDebugTextDrawStage.hpp \
    $${PATH_KS_DRAW}/KsDrawDefaultDrawKey.hpp \
    $${PATH_KS_DRAW}/KsDrawDrawCallUpdater.hpp \
    $${PATH_KS_DRAW}/KsDrawRenderSystem.hpp \
    $${PATH_KS_DRAW}/KsDrawBatchSystem.hpp

SOURCES += \
    $${PATH_KS_DRAW}/KsDrawComponents.cpp \
    $${PATH_KS_DRAW}/KsDrawRenderStats.cpp \
    $${PATH_KS_DRAW}/KsDrawDebugTextDrawStage.cpp \
    $${PATH_KS_DRAW}/KsDrawDefaultDrawKey.cpp \
    $${PATH_KS_DRAW}/KsDrawBatchSystem.cpp
