#ifndef AUIOOUTPUTTHREAD_H
#define AUIOOUTPUTTHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include "sdr/buffer.hh"


class AudioOutputThread: public QThread
{
    Q_OBJECT
public:
    explicit AudioOutputThread(QObject *parent);
    ~AudioOutputThread();
    void stop();
    void writeBuffer(const sdr::RawBuffer &buffer);


protected:
    void run();

private:

    QMutex *mutex{};
    bool m_abort {false};
    QAudioOutput *m_audioOutput{};
    QIODevice* ioDevice{};
    QQueue<QByteArray> *queue{};
};

#endif // AUIOOUTPUTTHREAD_H
