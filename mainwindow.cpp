#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sdr/logger.hh"

#include <QSplitter>

using namespace  std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    setGeometry(0, 0, 800, 600);

    sampleRate      = static_cast<qint64>(DEFAULT_SAMPLE_RATE);
    fftSize         = DEFAULT_FFT_SIZE;
    fftrate         = DEFAULT_FFT_RATE;
    freqStep        = DEFAULT_FREQ_STEP;
    tunerFrequency  = DEFAULT_FREQUENCY;
    currentDemod    = DemodulatorCtrl::DEMOD_WFM;

    m_sSettingsFile = QCoreApplication::applicationDirPath() + "/settings.ini";
    if (QFile(m_sSettingsFile).exists())
        loadSettings();

    signal_level = 0;

    d_realFftData = new float[MAX_FFT_SIZE];
    d_pwrFftData = new float[MAX_FFT_SIZE]();
    d_iirFftData = new float[MAX_FFT_SIZE];
    for (int i = 0; i < MAX_FFT_SIZE; i++)
        d_iirFftData[i] = RESET_FFT_FACTOR;  // dBFS

    // Install log message handler:
    sdr::Logger::get().addHandler(new sdr::StreamLogHandler(std::cerr, sdr::LOG_INFO));

    m_Receiver = new Receiver();
    if (!m_Receiver) QApplication::quit();

    QObject::connect(m_Receiver, &Receiver::started, this, &MainWindow::onReceiverStarted);
    QObject::connect(m_Receiver, &Receiver::stopped, this, &MainWindow::onReceiverStopped);

    m_Demodulator = m_Receiver->demod();
    if (!m_Demodulator) QApplication::quit();

    m_Demodulator->setFilterWidth(2*m_HiCutFreq, true);

    QObject::connect(m_Demodulator, &DemodulatorCtrl::spectrumUpdated, this, &MainWindow::fftTimeout);
    QObject::connect(m_Demodulator, &DemodulatorCtrl::filterChanged, this, &MainWindow::onFilterChanged);    

    if (m_Receiver->isRunning()) { ui->push_connect->setChecked(true); ui->push_connect->setText("Stop"); }
    else { ui->push_connect->setChecked(false); ui->push_connect->setText("Start"); }

    sourceView = (DataSourceCtrlView*)m_Receiver->createSourceCtrlView();
    demodView = (DemodulatorCtrlView*)m_Receiver->createDemodCtrlView();
    audioView = (AudioPostProcView*)m_Receiver->createAudioCtrlView();

    rTLCtrlView =(RTLCtrlView*) sourceView->currentSrcCtrl();
    QObject::connect(rTLCtrlView, &RTLCtrlView::source_setFrequency, this, &MainWindow::onSource_setFrequency);

    demodView->setDemodIndex(currentDemod);
    m_Demodulator->setDemod(currentDemod);
    m_Demodulator->setRrate(fftrate);
    setFrequency(tunerFrequency);

    initObjects();
    setPlotterSettings();

    ctrls = new QTabWidget();
    ctrls->addTab(sourceView, "Source");
    ctrls->addTab(demodView, "Demodulator");
    ctrls->addTab(audioView, "Audio");
    ctrls->addTab(ui->frame_controls, "Settings");
    ctrls->setMinimumWidth(400);
    ui->gridLayoutSource->addWidget(ctrls);
    ctrls->setEnabled(false);

    setFftRate(fftrate);
    setFreqStep(freqStep);

    auto deviceId   = m_Receiver->getDeviceID();
    auto deviceName = rtlsdr_get_device_name(deviceId);
    auto tunerFreq  = m_Receiver->tunerFrequency();
    auto sampleRate = m_Receiver->sampleRate();

    QString info;
    info.append("-> Receiver: " + QString(deviceName) + "\n");
    info.append("-> Tuner Frequency: " + QString::number(tunerFreq / 1000000.0, 'f', 2) + " MHz\n");
    info.append("-> Sample Rate: " + QString::number(sampleRate / 1000000.0, 'f', 2) + " MS/s\n");
    info.append("-> Fft Size: " + QString::number(fftSize) + "\n");
    info.append("-> HiCutFreq: " + QString::number(m_HiCutFreq / 1000.0, 'f', 2) + " Khz\n");
    info.append("-> LowCutFreq: " + QString::number(m_LowCutFreq / 1000.0, 'f', 2) + " Khz\n");
    info.append("-> FFT Refresh Rate: " + QString::number(fftrate) + " Hz");
    appentTextBrowser(info.toStdString().c_str());

    // meter timer
    meter_timer = new QTimer(this);
    connect(meter_timer, &QTimer::timeout, this, &MainWindow::tunerTimeout);   
}

MainWindow::~MainWindow()
{
    qDebug() << "exiting...";

    if(audioOutputThread)audioOutputThread->stop();
    if(m_Receiver)m_Receiver->stop();

    if(d_realFftData)delete [] d_realFftData;
    if(d_iirFftData)delete [] d_iirFftData;
    if(d_pwrFftData)delete [] d_pwrFftData;

    delete ui;
}

void MainWindow::changeEvent( QEvent* e )
{
    QRect rect = qApp->primaryScreen()->geometry();
    int width = rect.width();

    initObjects();

    if( e->type() == QEvent::WindowStateChange )
    {       
        QWindowStateChangeEvent* event = static_cast< QWindowStateChangeEvent* >( e );

        if( event->oldState() == Qt::WindowNoState && this->windowState() == Qt::WindowMaximized )
        {
            ui->filterFreq->setMaximumWidth(width/4);
            ui->sMeter->setMaximumWidth(width/4);
        }
    }

    ui->freqCtrl->update();
    ui->filterFreq->update();

}

void MainWindow::initObjects()
{
    ui->push_exit->setStyleSheet("font-size: 18pt; font-weight: bold; color: white;background-color: #8F3A3A; padding: 6px; spacing: 6px");
    ui->push_connect->setStyleSheet("font-size: 18pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    ui->text_terminal->setStyleSheet("font: 10pt; color: #00cccc; background-color: #001a1a;");    

    ui->filterFreq->setG_constant(1.5);
    ui->filterFreq->Setup(6, 0 ,1000e3, 1, UNITS_KHZ);
    ui->filterFreq->SetDigitColor(QColor("#FF5733"));
    ui->filterFreq->SetFrequency(0);

    ui->freqCtrl->setG_constant(2.5);
    ui->freqCtrl->Setup(11, 0, 2200e6, 1, UNITS_MHZ);
    ui->freqCtrl->SetDigitColor(QColor("#FFC300"));
    ui->freqCtrl->SetFrequency(tunerFrequency);
    connect(ui->freqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(onFreqCtrl_setFrequency(qint64)));

    ui->filterFreq->setMaximumWidth(250);
    ui->sMeter->setMaximumWidth(250);

    ui->filterFreq->setMaximumHeight(60);
    ui->sMeter->setMaximumHeight(60);
    ui->freqCtrl->setMaximumHeight(125);

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

void MainWindow::loadSettings()
{
    QSettings settings(m_sSettingsFile, QSettings::IniFormat);
    currentDemod = static_cast<DemodulatorCtrl::Demod>(settings.value("currentDemod", "").toString().toUShort());
    tunerFrequency = settings.value("tunerFrequency", "").toString().toULong();
    fftrate = settings.value("fftrate", "").toString().toUInt();
    freqStep = settings.value("freqStep", "").toString().toUInt();
}

void MainWindow::saveSettings()
{
    QSettings settings(m_sSettingsFile, QSettings::IniFormat);
    settings.setValue("currentDemod", QString::number(currentDemod));
    settings.setValue("tunerFrequency", QString::number(tunerFrequency));
    settings.setValue("fftrate", QString::number(fftrate));
    settings.setValue("freqStep", QString::number(freqStep));
}

void MainWindow::on_push_exit_clicked()
{
    QApplication::quit();
}

void MainWindow::on_push_connect_clicked()
{
    if(ui->push_connect->text() == "Start")
    {
        if (m_Receiver->isRunning()) { return; }
        m_Receiver->start();
        ctrls->setEnabled(true);
    }
    else
    {
        if (!m_Receiver->isRunning()) { return; }
        m_Receiver->stop();
        ctrls->setEnabled(false);
    }
}

void MainWindow::setPlotterSettings()
{
    ui->plotter->setSampleRate(sampleRate);
    ui->plotter->setFftRate(fftrate);
    ui->waterFallColor->setCurrentIndex(COLPAL_MIX);
}

void MainWindow::onReceiverStarted() {

    sdr::Logger::get().log(sdr::LogMessage(sdr::LOG_INFO, "Receiver started."));
    ui->text_terminal->clear();

    setPlotterSettings();

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

void MainWindow::onFreqCtrl_setFrequency(qint64 freq)
{
    setFrequency(freq);
    if(rTLCtrlView)
    {
        rTLCtrlView->update();
    }
    saveSettings();
}

void MainWindow::onSource_setFrequency(qint64 freq)
{    
    ui->freqCtrl->SetFrequency(freq, false);
    saveSettings();
}

void MainWindow::on_fftRateSelector_currentIndexChanged(const QString &arg1)
{
    fftrate = arg1.toInt();
    m_Demodulator->setRrate(fftrate);
    ui->plotter->setFftRate(fftrate);
    saveSettings();
}

void MainWindow::on_freqStepSelector_currentIndexChanged(const QString &arg1)
{
    freqStep = arg1.toInt();
    ui->plotter->setWheelConstant(KHZ(freqStep));
    saveSettings();
}

/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_newDemodFreq(qint64 freq, qint64 delta)
{
    setFrequency(freq);
    ui->freqCtrl->SetFrequency(freq, false);
    if(rTLCtrlView)
    {
        rTLCtrlView->update();
    }
    saveSettings();
}

void MainWindow::setFrequency(qint64 freq)
{
    if(!m_Receiver->setTunerFrequency(freq))
    {
        snprintf(log_buffer, sizeof(log_buffer),
                 "Could not Set Frequency-> %.1f MHz",(float)freq/1000000.0f);

        appentTextBrowser(log_buffer);
    }
    saveSettings();
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
    sampleRate      = static_cast<unsigned int>(m_Demodulator->sampleRate());
    fftSize         = static_cast<unsigned int>(m_Demodulator->fftSize());
    m_HiCutFreq     = m_Demodulator->filterUpper();
    m_LowCutFreq    = m_Demodulator->filterLower();
    tunerFrequency  = m_Receiver->tunerFrequency();
    currentDemod    = m_Demodulator->getDemod();

    ui->plotter->setCenterFreq(tunerFrequency);
    ui->plotter->setSampleRate(sampleRate);
    ui->sMeter->setLevel(signal_level);
    ui->filterFreq->SetFrequency(m_HiCutFreq);
    ui->plotter->setHiLowCutFrequencies(m_LowCutFreq, m_HiCutFreq);   

    auto deviceId = m_Receiver->getDeviceID();
    auto deviceName = rtlsdr_get_device_name(deviceId);
    auto tunerFreq = m_Receiver->tunerFrequency();
    auto sampleRate = m_Receiver->sampleRate();

    ui->text_terminal->clear();

    QString info;
    info.append("-> Receiver: " + QString(deviceName) + "\n");
    info.append("-> Tuner Frequency: " + QString::number(tunerFreq / 1000000.0, 'f', 2) + " MHz\n");
    info.append("-> Sample Rate: " + QString::number(sampleRate / 1000000.0, 'f', 2) + " MS/s\n");
    info.append("-> Fft Size: " + QString::number(fftSize) + "\n");
    info.append("-> HiCutFreq: " + QString::number(m_HiCutFreq / 1000.0, 'f', 2) + " Khz\n");
    info.append("-> LowCutFreq: " + QString::number(m_LowCutFreq / 1000.0, 'f', 2) + " Khz\n");
    info.append("-> FFT Refresh Rate: " + QString::number(fftrate) + " Hz");
    appentTextBrowser(info.toStdString().c_str());

    saveSettings();
}

void MainWindow::onFilterChanged()
{
    qDebug() << "onFilterChanged";
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
    qDebug() << "on_waterFallColor_currentIndexChanged";
    ui->plotter->setWaterfallPalette(index);
}
