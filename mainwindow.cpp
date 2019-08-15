#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sdr/logger.hh"
#include <QSplitter>

MainWindow *MainWindow::theInstance_;
using namespace  std;

MainWindow::MainWindow(Receiver *receiver, QWidget *parent) :
    QMainWindow(parent),
    m_Receiver(receiver),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<sdr::RawBuffer>("RawBuffer");
    theInstance_ = this;

    sampleRate      = static_cast<qint64>(DEFAULT_SAMPLE_RATE);
    fftSize         = DEFAULT_FFT_SIZE;
    fftrate         = DEFAULT_FFT_RATE;
    freqStep        = DEFAULT_FREQ_STEP;
    demodGain       = DEFAULT_AUDIO_GAIN;
    m_HiCutFreq     = KHZ(100);
    m_LowCutFreq    = -KHZ(100);
    signal_level = 0;

    d_realFftData = new float[MAX_FFT_SIZE];
    d_pwrFftData = new float[MAX_FFT_SIZE]();
    d_iirFftData = new float[MAX_FFT_SIZE];
    for (int i = 0; i < MAX_FFT_SIZE; i++)
        d_iirFftData[i] = RESET_FFT_FACTOR;  // dBFS

    /* meter timer */
    meter_timer = new QTimer(this);
    connect(meter_timer, &QTimer::timeout, this, &MainWindow::tunerTimeout);

    if (!m_Receiver) { exit(1); }

    m_Demodulator = m_Receiver->demod();
    m_Demodulator->setFilterWidth(2*m_HiCutFreq);
    fftrate = static_cast<unsigned int>(m_Demodulator->rrate());

    QObject::connect(m_Demodulator, &DemodulatorCtrl::spectrumUpdated, this, &MainWindow::fftTimeout);
    QObject::connect(m_Demodulator, &DemodulatorCtrl::filterChanged, this, &MainWindow::onFilterChanged);

    initObjects();
    initSpectrumGraph();
    setPlotterSettings();

    if (m_Receiver->isRunning()) { ui->push_connect->setChecked(true); ui->push_connect->setText("Stop"); }
    else { ui->push_connect->setChecked(false); ui->push_connect->setText("Start"); }

    QObject::connect(m_Receiver, SIGNAL(started()), SLOT(onReceiverStarted()));
    QObject::connect(m_Receiver, SIGNAL(stopped()), SLOT(onReceiverStopped()));

    QTabWidget *ctrls = new QTabWidget();
    ctrls->addTab(m_Receiver->createSourceCtrlView(), "Source");
    ctrls->addTab(m_Receiver->createDemodCtrlView(), "Demodulator");
    ctrls->addTab(m_Receiver->createAudioCtrlView(), "Audio");
    ctrls->addTab(ui->frame_controls, "Settings");
    ui->gridLayoutSource->addWidget(ctrls);
}

MainWindow *MainWindow::instance()
{
    return theInstance_;
}

MainWindow::~MainWindow()
{
    delete [] d_realFftData;
    delete [] d_iirFftData;
    delete [] d_pwrFftData;
    delete ui;
}

void MainWindow::on_push_connect_clicked()
{
    if(ui->push_connect->text() == "Start")
    {
        if (m_Receiver->isRunning()) { return; }
        m_Receiver->start();
    }
    else
    {
        if (!m_Receiver->isRunning()) { return; }
        m_Receiver->stop();
    }
}

void MainWindow::setPlotterSettings()
{
    ui->plotter->setSampleRate(sampleRate);
    ui->plotter->setFftRate(fftrate);
    ui->waterFallColor->setCurrentIndex(COLPAL_MIX);
    setFreqStep(freqStep);
    setFftRate(fftrate);    
}

void MainWindow::onReceiverStarted() {

    sdr::Logger::get().log(sdr::LogMessage(sdr::LOG_INFO, "Receiver started."));
    ui->text_terminal->clear();

    sampleRate      = static_cast<unsigned int>(m_Demodulator->sampleRate());
    fftSize         = static_cast<unsigned int>(m_Demodulator->fftSize());
    demodGain       = static_cast<unsigned int>(m_Demodulator->gain());
    m_HiCutFreq     = m_Demodulator->filterUpper();
    m_LowCutFreq    = m_Demodulator->filterLower();

    setPlotterSettings();
    setFrequency(m_Receiver->tunerFrequency());    

    meter_timer->start(100);
    ui->push_connect->setText("Stop"); ui->push_connect->setEnabled(true);
}

void MainWindow::onReceiverStopped()
{
    sdr::Logger::get().log(sdr::LogMessage(sdr::LOG_INFO, "Receiver stopped."));
    ui->text_terminal->clear();

    meter_timer->stop();

    QString info;
    info.append("Receiver stopped.\n");
    appentTextBrowser(info.toStdString().c_str());

    ui->push_connect->setText("Start"); ui->push_connect->setEnabled(true);
}

void MainWindow::on_push_exit_clicked()
{
    exit(0);
}

void MainWindow::on_fftRateSelector_currentIndexChanged(const QString &arg1)
{
    int value = arg1.toInt();
    fftrate = value;
    m_Demodulator->setRrate(fftrate);
    ui->plotter->setFftRate(fftrate);
}

void MainWindow::on_freqStepSelector_currentIndexChanged(const QString &arg1)
{
    freqStep = arg1.toInt();
    ui->plotter->setWheelConstant(KHZ(freqStep));
}

/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_newDemodFreq(qint64 freq, qint64 delta)
{
    m_Receiver->setTunerFrequency(freq);
}

void MainWindow::setFrequency(qint64 freq)
{  
    if(!m_Receiver->setTunerFrequency(freq))
    {
        snprintf(log_buffer, sizeof(log_buffer),
                 "Could not Set Frequency-> %.1f MHz",(float)freq/1000000.0f);

        appentTextBrowser(log_buffer);
    }
}


/* CPlotter::NewfilterFreq() is emitted or bookmark activated */
void MainWindow::on_plotter_newFilterFreq(int low, int high)
{
    m_LowCutFreq = low;
    m_HiCutFreq = high;

    m_Demodulator->setFilterWidth(2*m_HiCutFreq, false);

    snprintf(log_buffer, sizeof(log_buffer),
             "Set Filter-> low: %.1f Khz - high: %.1f Khz",
             m_Demodulator->filterLower()/1000.0f, m_Demodulator->filterUpper()/1000.0f);

    appentTextBrowser(log_buffer);

}

///OBJECT FUNCTIONS

void MainWindow::initObjects()
{
    QString const& pingURL = QString("xset -dpms");
    int status = system(pingURL.toStdString().c_str());
    if (-1 != status)
    {
        printf("Monitor auto turn off, Disabled.\n");
    }
    ui->push_exit->setStyleSheet("color: white;background-color: CadetBlue;");
    ui->push_connect->setStyleSheet("color: white;background-color: CadetBlue;");
    ui->text_terminal->setStyleSheet("font: 12px; color: #00cccc; background-color: #001a1a;");

    ui->filterFreq->setup(0, 0 ,1000e3, 1, FCTL_UNIT_KHZ);
    ui->filterFreq->setDigitColor(QColor("#FF5733"));
    ui->filterFreq->setFrequency(0);

    ui->freqCtrl->setup(0, 0, 999e8, 1, FCTL_UNIT_MHZ);
    ui->freqCtrl->setDigitColor(QColor("#FFC300"));
    ui->freqCtrl->setFrequency(static_cast<qint64>(DEFAULT_FREQUENCY));
}


void MainWindow::initSpectrumGraph()
{
    /* set up FFT */

    ui->plotter->setTooltipsEnabled(true);

    ui->plotter->setSampleRate(sampleRate);
    ui->plotter->setSpanFreq(static_cast<quint32>(sampleRate));
    ui->plotter->setCenterFreq(static_cast<quint64>(DEFAULT_FREQUENCY));

    ui->plotter->setFftRange(-140.0f, 20.0f);
    ui->plotter->setPandapterRange(-140.f, 20.f);
    ui->plotter->setHiLowCutFrequencies(m_LowCutFreq, m_HiCutFreq);
    ui->plotter->setDemodRanges(m_LowCutFreq, -KHZ(5), KHZ(5),m_HiCutFreq, true);

    ui->plotter->setFreqUnits(1000);
    ui->plotter->setPercent2DScreen(50);
    ui->plotter->setFilterBoxEnabled(true);
    ui->plotter->setCenterLineEnabled(true);
    ui->plotter->setClickResolution(1);

    ui->plotter->setFftPlotColor(QColor("#CEECF5"));
    ui->plotter->setWheelConstant(KHZ(5));

    //ui->plotter->setPeakDetection(true ,2);
    ui->plotter->setFftFill(true);
}

void MainWindow::appentTextBrowser(const char* stringBuffer)
{
    ui->text_terminal->append(QString::fromUtf8(stringBuffer));
    ui->text_terminal->moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
}

int MainWindow::setFreqStep(int step)
{
    int idx = -1;
    QString size_str = QString::number(step);

    idx = ui->freqStepSelector->findText(size_str, Qt::MatchExactly);
    if(idx != -1)
        ui->freqStepSelector->setCurrentIndex(idx);

    return 0;
}

int MainWindow::setFftRate(int rate)
{
    int idx = -1;
    QString size_str = QString::number(rate);

    idx = ui->fftRateSelector->findText(size_str, Qt::MatchExactly);
    if(idx != -1)
        ui->fftRateSelector->setCurrentIndex(idx);

    return 0;
}

void MainWindow::tunerTimeout()
{
    ui->text_terminal->clear();

    sampleRate      = static_cast<unsigned int>(m_Demodulator->sampleRate());
    fftSize         = static_cast<unsigned int>(m_Demodulator->fftSize());
    demodGain       = m_Demodulator->gain();
    m_HiCutFreq     = m_Demodulator->filterUpper();
    m_LowCutFreq    = m_Demodulator->filterLower();
    auto tunerFreq = m_Receiver->tunerFrequency();

    ui->freqCtrl->setFrequency(tunerFreq);
    ui->plotter->setCenterFreq(tunerFreq);
    ui->plotter->setSampleRate(sampleRate);    
    ui->sMeter->setLevel(signal_level);
    ui->filterFreq->setFrequency(m_HiCutFreq);
   // ui->plotter->setHiLowCutFrequencies(m_LowCutFreq, m_HiCutFreq);
   // ui->plotter->setDemodRanges(m_LowCutFreq, -KHZ(5), KHZ(5),m_HiCutFreq, true);

    auto deviceId = m_Receiver->getDeviceID();
    auto deviceName = rtlsdr_get_device_name(deviceId);

    QString info;
    info.append(QString(deviceName) + " Receiver started.\n");
    info.append("-> Tuner Frequency: " + QString::number(tunerFreq / 1000000.0, 'f', 1) + " MHz\n");
    info.append("-> Sample Rate: " + QString::number(sampleRate / 1000000.0, 'f', 1) + " MS/s\n");
    info.append("-> Fft Size: " + QString::number(fftSize) + "\n");
    info.append("-> Gain: " + QString::number(demodGain, 'f', 1)  + " dB\n");
    info.append("-> HiCutFreq: " + QString::number(m_HiCutFreq / 1000.0, 'f', 1) + " Khz\n");
    info.append("-> LowCutFreq: " + QString::number(m_LowCutFreq / 1000.0, 'f', 1) + " Khz\n");
    info.append("-> FFT Refresh Rate: " + QString::number(fftrate) + " Hz");
    appentTextBrowser(info.toStdString().c_str());
}

void MainWindow::onFilterChanged()
{
    m_HiCutFreq     = m_Demodulator->filterUpper();
    m_LowCutFreq    = m_Demodulator->filterLower();
    ui->plotter->setHiLowCutFrequencies(m_LowCutFreq, m_HiCutFreq);
    ui->plotter->setDemodRanges(m_LowCutFreq, -KHZ(5), KHZ(5),m_HiCutFreq, true);
}

void MainWindow::fftTimeout()
{
    unsigned int            fftsize;
    unsigned int            i;
    float                   pwr;
    float                   pwr_scale;
    std::complex<float>     pt;

    //75 is default
    d_fftAvg = static_cast<float>(1.0 - 1.0e-2 * 90);

    fftsize = static_cast<unsigned int>(m_Demodulator->fftSize());
    if(fftsize > MAX_FFT_SIZE)
        fftsize = MAX_FFT_SIZE;

    auto d_fftData = m_Demodulator->spectrum();

    if (fftsize == 0)
    {
        return;
    }

    // NB: without cast to float the multiplication will overflow at 64k
    // and pwr_scale will be inf
    pwr_scale = static_cast<float>(1.0 / (fftsize * fftsize));

    for (i = 0; i < fftsize; i++)
    {
        if (i < fftsize/2)
        {
            pt = d_fftData[fftsize/2+i];
        }
        else
        {
            pt = d_fftData[i-fftsize/2];
        }

        /* calculate power in dBFS */
        pwr = pwr_scale * (pt.imag() * pt.imag() + pt.real() * pt.real());

        /* calculate signal level in dBFS */
        signal_level = 15.f * log10(pwr + 1.0e-20f);
        d_realFftData[i] = signal_level;

        /* FFT averaging */
        d_iirFftData[i] += d_fftAvg * (d_realFftData[i] - d_iirFftData[i]);
    }

    ui->plotter->setNewFttData(d_iirFftData, d_realFftData, static_cast<int>(fftsize));
}

void MainWindow::on_waterFallColor_currentIndexChanged(int index)
{
    ui->plotter->setWaterfallPalette(index);
}
