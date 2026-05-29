# QT Modules
QT += qml \
      quick \
      core \
      gui \
      multimedia

# Target
TARGET = camera
TEMPLATE = app

DEFINES += QT_NO_DEBUG_OUTPUT strip
QMAKE_CXXFLAGS += -W

contains(CONFIG, DOLPHIN) {
    DEFINES += DOLPHIN
} else: contains(CONFIG, KLAMATH) {
    DEFINES += KLAMATH
}

# Sources and Headers
SOURCES += main.cpp \
    camera.cpp \
    CapturePhoto.cpp \
    CaptureVideo.cpp \
    GstPhoto.cpp \
    GstPreveiwVideo.cpp \
    GstStreaming.cpp \
    GstStreamingFile.cpp \
    ImageItem.cpp \
    ImagePreview.cpp \
    Preview.cpp \
    VideoPlayer.cpp \
    camera_texture.cpp \
    effects_controller.cpp \
    roi_control.cpp \
    v4l2_ctl.cpp \
    yuv2rgb_material.cpp \
    camera_player.cpp \
    imagestream.cpp \
    GstStreamingRTP.cpp \
    streaming_server.cpp

HEADERS += \
    camera.h\
    nv12_frame.h \
    CapturePhoto.h \
    CaptureVideo.h \
    GstPhoto.h \
    GstPreviewVideo.h \
    GstStreaming.h \
    GstStreamingFile.h \
    ImageItem.h \
    ImagePreview.h \
    Preview.h \
    VideoPlayer.h \
    VideoStreamer.h \
    camera_texture.h \
    effects_controller.h \
    roi_control.h \
    v4l2_ctl.h \
    yuv2rgb_material.h \
    camera_player.h \
    imagestream.h \
    GstStreamingRTP.h \
    streaming_server.h

DEFINES += "CONFIG_RDK_PLATFORM"
SOURCES += rdk_camera.cpp
HEADERS += rdk_camera.h

DEFINES += "YUV_SEMIPLANNER_SHADER"
DEFINES += "USE_QUEUE_HOLDER"

unix:INCLUDEPATH += ${PKG_CONFIG_SYSROOT_DIR}/usr/include/gstreamer-1.0 \
                ${PKG_CONFIG_SYSROOT_DIR}/usr/include/glib-2.0/ \
                ${PKG_CONFIG_SYSROOT_DIR}/usr/lib64/glib-2.0/include/ \
                ${PKG_CONFIG_SYSROOT_DIR}/usr/lib/glib-2.0/include/

unix:LIBS += -lgstreamer-1.0 \
    -lglib-2.0 \
    -lgobject-2.0 \
    -lgstapp-1.0 \
    -lgstvideo-1.0

# Qml Files
OTHER_FILES += main.qml

# Resources
RESOURCES += qml.qrc\
        images.qrc



QMAKE_RESOURCE_FLAGS += -no-compress

target.path = /usr/bin/
INSTALLS += target

# Objects Pathes
DESTDIR = bin
UI_DIR = build
MOC_DIR = build
RCC_DIR = build
OBJECTS_DIR = build
