#include "rtldatasource.hh"
#include "receiver.hh"

#include <QComboBox>
#include <QFormLayout>
#include <QToolButton>
#include <QPushButton>
#include <QDebug>

using namespace sdr;


/* ******************************************************************************************** *
 * Implementation of RTLDataSourceConfig
 * ******************************************************************************************** */
RTLDataSourceConfig::RTLDataSourceConfig()
    : _config(Configuration::get())
{
    // pass...
}

RTLDataSourceConfig::~RTLDataSourceConfig() {
    // pass...
}

double
RTLDataSourceConfig::frequency() const {
    return _config.value("RTLDataSource/frequency", 433.9e6).toDouble();
}

void
RTLDataSourceConfig::storeFrequency(double f) {
    _config.setValue("RTLDataSource/frequency", f);
}

double
RTLDataSourceConfig::sampleRate() const {
    return _config.value("RTLDataSource/sampleRate", 2.00e6).toDouble();
}

void
RTLDataSourceConfig::storeSampleRate(double f) {
    _config.setValue("RTLDataSource/sampleRate", f);
}



/* ******************************************************************************************** *
 * Implementation of RTLDataSource
 * ******************************************************************************************** */
RTLDataSource::RTLDataSource(QObject *parent)
    : DataSource(parent), deviceID(0), _device(nullptr), _balance(),  _config()
{
    try {
        _device = new RTLSource(_config.frequency(), _config.sampleRate());
    } catch (sdr::SDRError &err) {
        sdr::LogMessage msg(sdr::LOG_WARNING);
        msg << "Can not open RTL2832 device: " << err.what();
        sdr::Logger::get().log(msg);
    }

    _to_int16 = new AutoCast< std::complex<int16_t> >();
    if (nullptr != _device) {
        _device->connect(_to_int16, true);
    }
    _to_int16->connect(&_balance, true);
}

RTLDataSource::~RTLDataSource() {
    if (_device) { delete _device; }
    delete _to_int16;
}

QWidget *
RTLDataSource::createCtrlView() {
    return new RTLCtrlView(this);
}

Source *
RTLDataSource::source() {
    return &_balance;
}

bool
RTLDataSource::isActive() const {
    return 0 != _device;
}

double
RTLDataSource::frequency() const {
    return _device->frequency();
}

void
RTLDataSource::setFrequency(double freq) {
    // Set frequency of the device
    _device->setFrequency(freq);
    // and store it in the config
    _config.storeFrequency(freq);
}

double
RTLDataSource::sampleRate() const {
    return _device->sampleRate();
}

void
RTLDataSource::setSampleRate(double rate) {    
    bool is_running = sdr::Queue::get().isRunning();
    //if (is_running) { sdr::Queue::get().stop(); }
    if (is_running)
    {
        _device->stop();
        _device->setSampleRate(rate);
        _device->start();
    }
    //if (is_running) { sdr::Queue::get().start(); }
}

bool
RTLDataSource::agcEnabled() const {
    return _device->agcEnabled();
}

void
RTLDataSource::enableAGC(bool enable) {
    _device->enableAGC(enable);
}

double
RTLDataSource::gain() const {
    return _device->gain();
}

void
RTLDataSource::setGain(double gain) {
    return _device->setGain(gain);
}

const std::vector<double> &
RTLDataSource::gainFactors() const {
    return _device->gainFactors();
}

double
RTLDataSource::IQBalance() const {
    return _balance.balance();
}

void
RTLDataSource::setIQBalance(double balance) {
    _balance.setBalance(balance);
}

size_t
RTLDataSource::numDevices() {
    return sdr::RTLSource::numDevices();
}

std::string
RTLDataSource::deviceName(size_t idx) {
    return sdr::RTLSource::deviceName(idx);
}

size_t RTLDataSource::getDeviceID() const
{
    return deviceID;
}

bool
RTLDataSource::setDevice(size_t idx) {
    // Check for correct idx
    if (idx >= numDevices()) { return false; }
    // Stop queue if running
    bool is_running = sdr::Queue::get().isRunning();
    if (is_running) { sdr::Queue::get().stop(); }
    // Delete device (if it exists)
    if (_device) {
        _device->disconnect(_to_int16);
        delete _device; _device=nullptr;
    }
    // Try to start device
    try {
        _device = new RTLSource(DEFAULT_FREQUENCY, DEFAULT_SAMPLE_RATE, idx);
        _device->connect(_to_int16, true);
        deviceID = idx;

        // restart queue if it was running
        if (is_running) { sdr::Queue::get().start(); }
        return true;

    } catch (sdr::SDRError &err) {
        sdr::LogMessage msg(sdr::LOG_WARNING);
        msg << err.what();
        sdr::Logger::get().log(msg);         
    }

    return false;
}

void
RTLDataSource::queueStarted() {
    if (_device) { _device->start(); }
}

void
RTLDataSource::queueStopped() {
    if (_device) { _device->stop(); }
}

double
RTLDataSource::tunerFrequency() const {
    if (isActive()) {
        return frequency();
    }
    return 0.0;
}

bool
RTLDataSource::setTunerFrequency(qreal freq)
{
    if (_device)
    {
        setFrequency(freq);
        return true;
    }
    return false;
}

/* ******************************************************************************************** *
 * Implementation of RTLDataSource
 * ******************************************************************************************** */
RTLCtrlView::RTLCtrlView(RTLDataSource *source, QWidget *parent)
    : QWidget(parent), _source(source)
{
    _infoMessage = new QLabel("No RTL2832 device.");
    if (_source->isActive()) {
        _infoMessage->setVisible(false);
    }

    // Populate device list:
    _devices = new QComboBox();

    for (size_t i=0; i<RTLDataSource::numDevices(); i++)
    {
        _devices->addItem(RTLDataSource::deviceName(i).c_str());
    }

    // Frequency
    _freq = new QLineEdit();
    /*QDoubleValidator *freq_val = new QDoubleValidator();
    freq_val->setBottom(0);
    _freq->setValidator(freq_val);*/

    // set frequencies
    QAction *setFreqAction = new QAction(QIcon::fromTheme("document-save"), "Set",this);
    QObject::connect(setFreqAction, SIGNAL(triggered()), this, SLOT(onSetFrequency()));
    QToolButton *setFreqButton = new QToolButton();
    setFreqButton->setDefaultAction(setFreqAction);

    // Sample rate:
    _sampleRates = new QComboBox();
    _sampleRates->addItem("2 MS/s", 2e6);
    _sampleRates->addItem("1.5 MS/s", 1.5e6);
    _sampleRates->addItem("1 MS/s", 1e6);

    _gain = new QComboBox();
    _agc = new QCheckBox();

    QFormLayout *layout = new QFormLayout();

    QVBoxLayout *deviceLayout = new QVBoxLayout();
    deviceLayout->addWidget(_devices);
    deviceLayout->addWidget(_infoMessage);
    layout->addRow("Device", deviceLayout);

    QHBoxLayout *freqLayout = new QHBoxLayout();
    freqLayout->addWidget(_freq, 1); freqLayout->addWidget(setFreqButton, 0);
    layout->addRow("Frequency (MHz)", freqLayout);

    layout->addRow("Sample rate", _sampleRates);
    layout->addRow("Gain", _gain);
    layout->addRow("AGC", _agc);
    setLayout(layout);

    QObject::connect(_devices, SIGNAL(currentIndexChanged(int)), this, SLOT(onDeviceSelected(int)));
    QObject::connect(_sampleRates, SIGNAL(currentIndexChanged(int)), this, SLOT(onSampleRateSelected(int)));
    QObject::connect(_gain, SIGNAL(currentIndexChanged(int)), this, SLOT(onGainChanged(int)));
    QObject::connect(_agc, SIGNAL(toggled(bool)), this, SLOT(onAGCToggled(bool)));


    /*for (size_t i=0; i<_devices->count(); i++)
    {
        char maker[256];      // manufacturer
        char prodname[256];   // product name
        char sn[256];         // serial number

        rtlsdr_get_device_usb_strings(i, maker, prodname, sn);
        QString sProdname(prodname);
        QString sMaker(maker);
        QString sSn(sn);
        if(!sProdname.contains("\u0003") && !sProdname.replace(" ", "").isEmpty())
        {
            _devices->setCurrentIndex(i);
            onDeviceSelected(i);
        }
    }*/

    if(_source->isActive())
    {
        char maker[256];      // manufacturer
        char prodname[256];   // product name
        char sn[256];         // serial number
        auto deviceId = _devices->currentIndex();

        rtlsdr_get_device_usb_strings(deviceId, maker, prodname, sn);
        QString sProdname(prodname);
        QString sMaker(maker);
        QString sSn(sn);

        // Enable all controlls:
        _freq->setEnabled(true);
        _sampleRates->setEnabled(true);
        _gain->setEnabled(true);
        _agc->setEnabled(true);
        _infoMessage->setText(sMaker + " " + QString(RTLDataSource::deviceName(deviceId).c_str()));

        double f = _source->frequency();
        _freq->setText(QString::number(f/1000000, 'f', 2));

        for (size_t i=0; i<_source->gainFactors().size(); i++) {
            _gain->addItem(QString("%1 dB").arg(_source->gainFactors()[i]/10), _source->gainFactors()[i]);
        }

        _agc->setChecked(_source->agcEnabled());
        if (_source->agcEnabled()) { _gain->setEnabled(false); }

        double rate = _sampleRates->itemData(0).toDouble();
        _source->setSampleRate(rate);
    }
    else if (! _source->isActive())
    {
        _freq->setEnabled(false);
        _sampleRates->setEnabled(false);
        _gain->setEnabled(false);
        _agc->setEnabled(false);
        _infoMessage->setText("Invalid RTL2832 device.");
    }
}

RTLCtrlView::~RTLCtrlView() {
    // pass...
}

void RTLCtrlView::update()
{
    double f = _source->frequency();
    _freq->setText(QString::number(f/1000000, 'f', 2));
}

void
RTLCtrlView::onDeviceSelected(int idx)
{
    if(!_source->setDevice(idx))
    {
        _infoMessage->setText("Can not open RTL2832 USB device");
    }
    else
    {
        _infoMessage->setText(QString(RTLDataSource::deviceName(idx).c_str()));
    }
}

void
RTLCtrlView::onSetFrequency() {
    double freq = _freq->text().toDouble();
    if (! _source->isActive()) { return; }
    _source->setFrequency(freq*1000000);
    emit source_setFrequency(freq*1000000);
    std::cerr << "Set frequency " << _source->tunerFrequency() << std::endl;
}

void
RTLCtrlView::onSampleRateSelected(int idx) {
    double rate = _sampleRates->itemData(idx).toDouble();
    if (_source->isActive()) {
        _source->setSampleRate(rate);
    }
}

void
RTLCtrlView::onGainChanged(int idx) {
    if (_source->isActive() && !_source->agcEnabled()) {
        double gain = _gain->itemData(idx).toDouble();
        _source->setGain(gain);
    }
}

void
RTLCtrlView::onAGCToggled(bool enabled) {
    if (! _source->isActive()) { return; }
    if (enabled) {
        _gain->setEnabled(false);
        _source->enableAGC(true);
    } else {
        _gain->setEnabled(true);
        _source->enableAGC(false);
    }
}
