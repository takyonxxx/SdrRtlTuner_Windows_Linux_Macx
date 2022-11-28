#-------------------------------------------------
#
# Project created by Türkay Biliyor 2019-08-06 T12:46:07
#
#-------------------------------------------------
QT+= core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SdrRtlTuner
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += BOOST_ALL_NO_LIB

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        gui/spectrum.cc \
        gui/spectrumview.cc \
        main.cpp \
        mainwindow.cpp \
        freqctrl.cpp \
        plotter.cpp \
        meter.cpp \
        receiver/audiootputthread.cpp \
        receiver/audiopostproc.cc \
        receiver/configuration.cc \
        receiver/demodulator.cc \
        receiver/receiver.cc \
        receiver/rtldatasource.cc \
        receiver/source.cc \
        sdr/portaudio.cc \
        sdr/aprs.cc \
        sdr/ax25.cc \
        sdr/baudot.cc \
        sdr/bch31_21.cc \
        sdr/buffer.cc \
        sdr/exception.cc \
        sdr/fsk.cc \
        sdr/logger.cc \
        sdr/node.cc \
        sdr/options.cc \
        sdr/pocsag.cc \
        sdr/psk31.cc \
        sdr/queue.cc \
        sdr/rtlsource.cc \
        sdr/sha1.cc \
        sdr/traits.cc \
        sdr/utils.cc \
        sdr/wavfile.cc


HEADERS += \
        gui/gui.hh \
        gui/spectrum.hh \
        gui/spectrumview.hh \
        mainwindow.h \
        freqctrl.h \
        plotter.h \
        meter.h \
        receiver/audiootputthread.h \
        receiver/audiopostproc.hh \
        receiver/configuration.hh \
        receiver/demodulator.hh \
        receiver/receiver.hh \
        receiver/rtldatasource.hh \
        receiver/source.hh \
        sdr/portaudio.hh \
        sdr/aprs.hh \
        sdr/autocast.hh \
        sdr/ax25.hh \
        sdr/baseband.hh \
        sdr/baudot.hh \
        sdr/bch31_21.hh \
        sdr/buffer.hh \
        sdr/buffernode.hh \
        sdr/combine.hh \
        sdr/config.hh \
        sdr/demod.hh \
        sdr/exception.hh \
        sdr/fftplan.hh \
        sdr/fftplan_fftw3.hh \
        sdr/fftplan_native.hh \
        sdr/filternode.hh \
        sdr/firfilter.hh \
        sdr/freqshift.hh \
        sdr/fsk.hh \
        sdr/interpolate.hh \
        sdr/logger.hh \
        sdr/math.hh \
        sdr/node.hh \
        sdr/operators.hh \
        sdr/options.hh \
        sdr/pocsag.hh \
        sdr/psk31.hh \
        sdr/queue.hh \
        sdr/rtlsource.hh \
        sdr/sdr.hh \
        sdr/sha1.hh \
        sdr/siggen.hh \
        sdr/streamsource.hh \
        sdr/subsample.hh \
        sdr/traits.hh \
        sdr/utils.hh \
        sdr/wavfile.hh

macx{
    message("macx enabled")
    ICON = $$PWD\icons\app.icns
    DEFINES += GQRX_OS_MACX

    INCLUDEPATH += /usr/local/include
    LIBS += /usr/local/lib/librtlsdr.dylib /usr/local/lib/libfftw3.dylib /usr/local/lib/libportaudio.dylib
}

unix:!macx{
    message("linux enabled")
    #sudo apt install libusb-1.0-0-dev
    #sudo apt-get install -y fftw3-dev
    #sudo apt install librtlsdr-dev
    #sudo apt install libportaudio2
    #sudo apt install portaudio19-dev
    #sudo apt install sox
    #sudo apt-get install libgl-dev
    #sudo apt-get install qtmultimedia5-dev

    INCLUDEPATH += /usr/lib
    INCLUDEPATH += /usr/include
    INCLUDEPATH += /usr/local/lib
    INCLUDEPATH += /usr/lib/x86_64-linux-gnu

    LIBS += -lrt -lportaudio -lrtlsdr -lfftw3
}

win32{
    message("Win32 enabled")
    RC_ICONS += $$PWD\icons\app.ico

    LIBS += -L$$PWD\libs\rtl-sdr-lib\x64 \
             -lrtlsdr

    LIBS += -L$$PWD\libs\fftw-3.3.5-dll64 \
             -llibfftw3-3

    LIBS += -L$$PWD\libs\portaudio-r1891\lib\x64\Release \
             -lportaudio_x64

    INCLUDEPATH += $$PWD\libs\rtl-sdr-lib
    INCLUDEPATH += $$PWD\libs\fftw-3.3.5-dll64
    INCLUDEPATH += $$PWD\libs\portaudio-r1891\include
}


DISTFILES += \
    icons/app.ico \
    sdr/config.hh.in

FORMS += \
    mainwindow.ui
