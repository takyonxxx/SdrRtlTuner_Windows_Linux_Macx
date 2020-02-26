#include "audiootputthread.h"
#include <QDebug>
#include <QBuffer>

AudioOutputThread::AudioOutputThread(QObject *parent):
    QThread(parent)
{
    //sample rate 16433
    QAudioFormat format;
    QAudioDeviceInfo defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    mutex = new QMutex();
    queue = new QQueue<QByteArray>();

    for (auto &deviceInfo: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (deviceInfo != defaultDeviceInfo)
            qDebug() << deviceInfo.deviceName();
    }

    format.setSampleRate(22050 + 433);
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

AudioOutputThread::~AudioOutputThread()
{
    delete queue;
    delete mutex;
}

void AudioOutputThread::stop()
{
    m_abort = true;
}

void AudioOutputThread::writeBuffer(const sdr::RawBuffer &buffer)
{   
    QMutexLocker locker(mutex);
    if (ioDevice && !m_abort)
    {
        QByteArray soundBuffer(buffer.data(), buffer.bytesLen());

        auto qBuffer = new QBuffer;
        qBuffer->open(QIODevice::ReadWrite);

        qBuffer->write(soundBuffer);
        qBuffer->close();

        queue->enqueue(qBuffer->buffer());
    }
}

void AudioOutputThread::run()
{
    while (true)
    {
        if(m_abort)
            break;

        QMutexLocker locker(mutex);
        if(queue->size() > 0)
        {
            auto buff = queue->dequeue();
            ioDevice->write(buff);
            ioDevice->waitForBytesWritten(-1);
        }
    }
}
