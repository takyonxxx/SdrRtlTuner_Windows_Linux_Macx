#include "audiopostproc.hh"
#include <QLineEdit>
#include <QDoubleValidator>
#include <QCheckBox>
#include <QFormLayout>

using namespace sdr;

AudioPostProc::AudioPostProc(QObject *parent)
    : QObject(parent), Sink<int16_t>()
{
    // Assemble processing chain
    _sub_sample = new SubSample<int16_t>(22050.0);
    _low_pass   = new FIRLowPass<int16_t>(10, 5e3);
    _low_pass->enable(false);
    _audio_spectrum = new gui::Spectrum(50, 256, this);

    // Connect all
    _sub_sample->connect(_low_pass, true);
    _low_pass->connect(_audio_spectrum);

    audioOutputThread = new AudioOutputThread(this);
    audioOutputThread->start();
}

AudioPostProc::~AudioPostProc() {
    delete _low_pass;
}

void
AudioPostProc::config(const Config &src_cfg) {
    // Forward to low pass
    _sub_sample->config(src_cfg);
}


void
AudioPostProc::process(const Buffer<int16_t> &buffer, bool allow_overwrite) {
    if(audioOutputThread)
        audioOutputThread->writeBuffer(_sub_sample->getSndBuffer(buffer));
}

bool
AudioPostProc::lowPassEnabled() const {
    return _low_pass->enabled();
}

void
AudioPostProc::enableLowPass(bool enable) {
    _low_pass->enable(enable);
}

double
AudioPostProc::lowPassFreq() const {
    return _low_pass->freq();
}

void
AudioPostProc::setLowPassFreq(double freq) {
    _low_pass->setFreq(freq);
}

size_t
AudioPostProc::lowPassOrder() const {
    return _low_pass->order();
}

void
AudioPostProc::setLowPassOrder(size_t order) {
    _low_pass->setOrder(order);
}

gui::Spectrum *
AudioPostProc::spectrum() const {
    return _audio_spectrum;
}



/* ******************************************************************************************** *
 * Implementation of View
 * ******************************************************************************************** */
AudioPostProcView::AudioPostProcView(AudioPostProc *proc, QWidget *parent)
    : QWidget(parent), _proc(proc)
{
    QCheckBox *lp_enable = new QCheckBox("Enable");
    lp_enable->setChecked(proc->lowPassEnabled());

    _lp_freq = new QLineEdit(QString("%1").arg(proc->lowPassFreq()));
    QDoubleValidator *lpf_val = new QDoubleValidator();
    lpf_val->setBottom(0); _lp_freq->setValidator(lpf_val);

    _lp_order = new QSpinBox();
    _lp_order->setRange(0, 1024); _lp_order->setValue(_proc->lowPassOrder());
    _lp_order->setSingleStep(5);

    if (! proc->lowPassEnabled()) {
        _lp_freq->setEnabled(false);
        _lp_order->setEnabled(false);
    }

    // Create spectrum view:
    _spectrum = new gui::SpectrumView(_proc->spectrum());
    _spectrum->setNumXTicks(5);
    _spectrum->setMinimumHeight(100);

    QObject::connect(_lp_freq, SIGNAL(textEdited(QString)), this, SLOT(onSetLowPassFreq(QString)));
    QObject::connect(_lp_order, SIGNAL(valueChanged(int)), this, SLOT(onSetLowPassOrder(int)));
    QObject::connect(lp_enable, SIGNAL(toggled(bool)), this, SLOT(onLowPassToggled(bool)));

    // Layout
    QVBoxLayout *vlayout = new QVBoxLayout();
    QHBoxLayout* hlayout = new QHBoxLayout();

    QLabel *_lb_freq = new QLabel("Low Pass (Hz)");
    QLabel *_lb_order = new QLabel("Order");

    hlayout->addWidget(_lb_freq);
    hlayout->addWidget(_lp_freq);
    hlayout->addWidget(_lb_order);
    hlayout->addWidget(_lp_order);
    hlayout->addWidget(lp_enable);

    QFormLayout *table = new QFormLayout();
    vlayout->addLayout(table, 0);
    vlayout->addLayout(hlayout, 1);

    vlayout->addWidget(_spectrum, 1);
    this->setLayout(vlayout);
}

AudioPostProcView::~AudioPostProcView() {
    // pass...
}

void
AudioPostProcView::onLowPassToggled(bool enable) {
    _proc->enableLowPass(enable);
    _lp_freq->setEnabled(enable);
    _lp_order->setEnabled(enable);
}

void
AudioPostProcView::onSetLowPassFreq(QString value) {
    bool ok; double freq = value.toDouble(&ok);
    if (ok) {
        _proc->setLowPassFreq(freq);
    }
}

void
AudioPostProcView::onSetLowPassOrder(int value) {
    if (value < 1) { _lp_order->setValue(1); }
    _proc->setLowPassOrder((size_t) value);
}
