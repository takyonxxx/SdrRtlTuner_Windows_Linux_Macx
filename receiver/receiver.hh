#ifndef __SDR_RX_RECEIVER_HH__
#define __SDR_RX_RECEIVER_HH__

#include <QObject>

#include "source.hh"
#include "demodulator.hh"
#include "audiopostproc.hh"

#define MHZ(x)                      ((x)*1000*1000)
#define KHZ(x)                      ((x)*1*1000)
#define DEFAULT_SAMPLE_RATE         MHZ(2.5)
#define DEFAULT_FREQUENCY		    MHZ(99.45)
#define DEFAULT_FFT_SIZE		    8192 * 4
#define DEFAULT_FFT_RATE		    50 //Hz
#define DEFAULT_FREQ_STEP           5 //kHz
#define DEFAULT_AUDIO_GAIN          50
#define MAX_FFT_SIZE                DEFAULT_FFT_SIZE
#define RESET_FFT_FACTOR            -72

class Receiver: public QObject
{
  Q_OBJECT

public:
  explicit Receiver(QObject *parent = nullptr);
  virtual ~Receiver();

  bool isRunning() const;

  QWidget *createSourceCtrlView();
  QWidget *createDemodCtrlView();
  QWidget *createAudioCtrlView();

  /** Returns the tuner frequency of the source or 0 if the source does not have a tuner. */
  double tunerFrequency() const;
  bool setTunerFrequency(qreal);
  size_t getDeviceID() const;

  DemodulatorCtrl *demod() const;

signals:
  void started();
  void stopped();

public slots:  
  void start();
  void stop();

protected:
  void _onQueueStarted();
  void _onQueueStopped();

protected:
  sdr::Queue &_queue;

  DataSourceCtrl  *_src;
  DemodulatorCtrl *_demod;
  AudioPostProc *_audio;
};


#endif // RECEIVER_HH
