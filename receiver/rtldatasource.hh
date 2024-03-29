#ifndef __SDR_RX_RTLDATASOURCE_HH__
#define __SDR_RX_RTLDATASOURCE_HH__

#include "source.hh"
#include "sdr/rtlsource.hh"
#include "sdr/utils.hh"
#include "sdr/autocast.hh"
#include "configuration.hh"

#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QMenu>

/** Persistent configuration of the RTL device. */
class RTLDataSourceConfig
{
public:
  RTLDataSourceConfig();
  virtual ~RTLDataSourceConfig();

  double frequency() const;
  void storeFrequency(double f);

  double sampleRate() const;
  void storeSampleRate(double rate);

protected:
  /** The global config instance. */
  Configuration &_config;
};


class RTLDataSource : public DataSource
{
  Q_OBJECT

public:
  RTLDataSource(QObject *parent=0);
  virtual ~RTLDataSource();

  virtual QWidget *createCtrlView();
  virtual sdr::Source *source();

  virtual void queueStarted();
  virtual void queueStopped();

  virtual double tunerFrequency() const;
  virtual bool setTunerFrequency(qreal freq);
  virtual bool setFreqCorrection(qreal ppm);
  double frequencyCorrection() const;

  bool isActive() const;

  double frequency() const;
  virtual void setFrequency(double freq);

  double sampleRate() const;
  void setSampleRate(double rate);

  bool agcEnabled() const;
  void enableAGC(bool enable);

  double gain() const;
  void setGain(double gain);
  const std::vector<double> &gainFactors() const;

  double IQBalance() const;
  void setIQBalance(double balance);

  bool setDevice(size_t idx);

  static size_t numDevices();
  static std::string deviceName(size_t idx);

  size_t deviceID;

  size_t getDeviceID() const;

private:
  sdr::RTLSource *_device;
  sdr::AutoCast< std::complex<int16_t> > *_to_int16;
  sdr::IQBalance< std::complex<int16_t> > _balance;
  RTLDataSourceConfig _config;
};


class RTLCtrlView: public QWidget
{
  Q_OBJECT

public:
  RTLCtrlView(RTLDataSource *source, QWidget *parent=0);
  virtual ~RTLCtrlView();
  void update();

protected slots:
  void onDeviceSelected(int idx);
  void onSetFrequency();
  void onSetFrequencyCorrection();
  void onSampleRateSelected(int idx);
  void onGainChanged(int idx);
  void onAGCToggled(bool enabled);

signals:
  void source_setFrequency(qint64 freq);
  void source_setFrequencyCorrection(qint64 ppm);

private:
  QLabel *_infoMessage;
  RTLDataSource *_source;

  QComboBox *_devices;
  QLineEdit *_freq;
  QLineEdit *_freqCorrection;
  QMenu     *_freqMenu;
  QAction   *_saveFreqAction;
  QComboBox *_sampleRates;
  QComboBox *_gain;
  QCheckBox *_agc;
};

#endif // __SDR_RX_RTLDATASOURCE_HH__
