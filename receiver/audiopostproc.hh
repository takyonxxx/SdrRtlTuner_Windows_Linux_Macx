#ifndef __SDR_RX_AUDIOPOSTPROC_HH__
#define __SDR_RX_AUDIOPOSTPROC_HH__

#include "sdr/subsample.hh"
#include "sdr/portaudio.hh"
#include "sdr/firfilter.hh"
#include "gui/gui.hh"

#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>


class AudioPostProc : public QObject, public sdr::Sink<int16_t>
{
  Q_OBJECT

public:
  explicit AudioPostProc(QObject *parent=0);
  virtual ~AudioPostProc();

  /** Implements sdr::Sink<double> interface. */
  virtual void config(const sdr::Config &src_cfg);
  virtual void process(const sdr::Buffer<int16_t> &buffer, bool allow_overwrite);

  bool lowPassEnabled() const;
  void enableLowPass(bool enable);

  double lowPassFreq() const;
  void setLowPassFreq(double freq);

  size_t lowPassOrder() const;
  void setLowPassOrder(size_t order);

  sdr::gui::Spectrum *spectrum() const;

protected:
  sdr::FIRLowPass<int16_t> *_low_pass;
  sdr::PortSink            *_sink;
  sdr::SubSample<int16_t>  *_sub_sample;
  sdr::gui::Spectrum       *_audio_spectrum;
};


class AudioPostProcView: public QWidget
{
  Q_OBJECT

public:
  explicit AudioPostProcView(AudioPostProc *proc, QWidget *parent=0);
  virtual ~AudioPostProcView();

protected slots:
  void onLowPassToggled(bool enable);
  void onSetLowPassFreq(QString value);
  void onSetLowPassOrder(int value);

protected:
  AudioPostProc *_proc;
  QLineEdit *_lp_freq;
  QSpinBox  *_lp_order;
  sdr::gui::SpectrumView *_spectrum;
};

#endif // AUDIOPOSTPROC_HH
