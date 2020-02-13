#ifndef __SDR_DATA_SOURCE_HH__
#define __SDR_DATA_SOURCE_HH__

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>

#include "sdr/node.hh"

// Forward Declaration
class Receiver;



class DataSource : public QObject
{
  Q_OBJECT

public:
  explicit DataSource(QObject *parent = nullptr);
  virtual ~DataSource();

  virtual QWidget *createCtrlView();
  virtual sdr::Source *source() = 0;

  virtual void triggerNext();
  virtual void queueStarted();
  virtual void queueStopped();

  /** Can be overwritten by any sub-class to provide the tuner frequency of the source. By default,
   * this method returns 0, means there is no tuner. */
  virtual double tunerFrequency() const;
  virtual double sampleRate() const;
  virtual bool setTunerFrequency(qreal freq);
  virtual size_t getDeviceID() const;
};



class DataSourceCtrl: public QObject, public sdr::Proxy
{
  Q_OBJECT

public:
  typedef enum {
    SOURCE_RTL
  } Src;

public:
  DataSourceCtrl(Receiver *receiver);
  virtual ~DataSourceCtrl();

  inline Src source() const { return _source; }
  void setSource(Src source);

  QWidget *createCtrlView();

  /** Returns the tuner frequency of the current source or 0 if the source does not have a tuner. */
  double tunerFrequency() const;
  bool setTunerFrequency(qreal freq);
  double tunerSampleRate() const;
  size_t getDeviceId();

protected:
  void _onQueueIdle();
  void _onQueueStart();
  void _onQueueStop();

protected:
  Receiver *_receiver;
  /** Currently selected source. */
  Src _source;
  /** Currently selected source object. */
  DataSource *_src_obj;
};



class DataSourceCtrlView: public QWidget
{
  Q_OBJECT

public:
  DataSourceCtrlView(DataSourceCtrl *src_ctrl, QWidget *parent = nullptr);
  virtual ~DataSourceCtrlView();

  QWidget *currentSrcCtrl() const;

protected:
  DataSourceCtrl *_src_ctrl;
  QVBoxLayout *_layout;
  QWidget *_currentSrcCtrl;
};

#endif // __SDR_DATA_SOURCE_HH__
