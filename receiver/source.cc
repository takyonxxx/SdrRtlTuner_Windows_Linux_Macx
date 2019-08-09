#include "source.hh"
#include "receiver.hh"
#include "rtldatasource.hh"
#include "sdr/queue.hh"


#include <QWidget>
#include <QComboBox>
#include <QDebug>

/* ********************************************************************************************* *
 * Implementation of DataSource
 * ********************************************************************************************* */
DataSource::DataSource(QObject *parent)
  : QObject(parent)
{
  // pass...
}

DataSource::~DataSource() {
  // pass...
}

QWidget *
DataSource::createCtrlView() {
  return new QWidget();
}

void
DataSource::triggerNext() {
  // pass...
}

void DataSource::queueStarted() {
  // pass...
}

void DataSource::queueStopped() {
  // pass...
}

double
DataSource::tunerFrequency() const {
  return 0;
}

bool
DataSource::setTunerFrequency(qreal freq){
  // pass...
}

size_t DataSource::getDeviceID() const
{
    return 0;
}


/* ********************************************************************************************* *
 * Implementation of DataSourceCtrl
 * ********************************************************************************************* */
DataSourceCtrl::DataSourceCtrl(Receiver *receiver)
  : QObject(receiver), Proxy(), _receiver(receiver), _source(SOURCE_RTL), _src_obj(nullptr)
{
  // Initialize PortAudio
  sdr::PortAudio::init();

  // Instantiate data source
  setSource(SOURCE_RTL);

  sdr::Queue::get().addIdle(this, &DataSourceCtrl::_onQueueIdle);
  sdr::Queue::get().addStart(this, &DataSourceCtrl::_onQueueStart);
  sdr::Queue::get().addStop(this, &DataSourceCtrl::_onQueueStop);
}


DataSourceCtrl::~DataSourceCtrl() {
  // pass...
}


void
DataSourceCtrl::setSource(Src source) {
  bool was_running = _receiver->isRunning();
  if (was_running) { _receiver->stop(); }

  // Unlink current source
  _src_obj->disconnect(this);
  // Free current source late
  _src_obj->deleteLater();

  // Create and link new data source
  _source = source;
  _src_obj = new RTLDataSource(this);
  _src_obj->source()->connect(this, true);

  if (was_running) { _receiver->start(); }
}


QWidget *
DataSourceCtrl::createCtrlView() {
  return _src_obj->createCtrlView();
}

double
DataSourceCtrl::tunerFrequency() const {
    return _src_obj->tunerFrequency();
}

bool
DataSourceCtrl::setTunerFrequency(qreal freq)
{
    return _src_obj->setTunerFrequency(freq);
}

size_t DataSourceCtrl::getDeviceId()
{
    return _src_obj->getDeviceID();
}

void
DataSourceCtrl::_onQueueIdle() {
  _src_obj->triggerNext();
}

void
DataSourceCtrl::_onQueueStart() {
  _src_obj->queueStarted();
}

void
DataSourceCtrl::_onQueueStop() {
  _src_obj->queueStopped();
}


/* ********************************************************************************************* *
 * Implementation of DataSourceCtrlView
 * ********************************************************************************************* */
DataSourceCtrlView::DataSourceCtrlView(DataSourceCtrl *src_ctrl, QWidget *parent)
  : QWidget(parent), _src_ctrl(src_ctrl)
{
  QLabel *src_sel = new QLabel();
  src_sel->setText("RTL2832");

  _currentSrcCtrl = _src_ctrl->createCtrlView();

  _layout = new QVBoxLayout();
  _layout->addWidget(src_sel, 0);
  _layout->addWidget(_currentSrcCtrl, 1);
  setLayout(_layout);
}

DataSourceCtrlView::~DataSourceCtrlView() {
    // pass...
}

