#include "audiootputthread.h"
#include <QDebug>
#include <QBuffer>

AudioOutputThread::AudioOutputThread(QObject *parent):
    QThread(parent)
{
    //sample rate 16433
    QAudioFormat format;
    QAudioDeviceInfo defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();

    for (auto &deviceInfo: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (deviceInfo != defaultDeviceInfo)
            qDebug() << deviceInfo.deviceName();
    }

    format.setSampleRate(16433);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    if (!defaultDeviceInfo.isFormatSupported(format)) {
        qWarning() << "Default format not supported - trying to use nearest";
        format = defaultDeviceInfo.nearestFormat(format);
    }

    m_audioOutput = new QAudioOutput( format, this );//.reset(new QAudioOutput(defaultDeviceInfo, format));
    ioDevice = m_audioOutput->start();
    qDebug() << "Default Sound Device: " << defaultDeviceInfo.deviceName();
}

void AudioOutputThread::stop()
{
    m_abort = true;
}

void AudioOutputThread::writeBuffer(const sdr::RawBuffer &buffer)
{
    QByteArray soundBuffer(buffer.data(), buffer.bytesLen());

    auto qBuffer = new QBuffer;
    qBuffer->open(QIODevice::ReadWrite);

    qBuffer->write(soundBuffer);
    qBuffer->close();
    if (ioDevice && !m_abort)
        ioDevice->write(qBuffer->buffer());
}

void AudioOutputThread::run()
{
}
