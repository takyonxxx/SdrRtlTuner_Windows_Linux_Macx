#ifndef MainWindow_H
#define MainWindow_H

#include <QMainWindow>
#include <QMessageBox>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <iomanip>
#include "freqctrl.h"
#include "gui/gui.hh"
#include "receiver/receiver.hh"
#include "receiver/rtldatasource.hh"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:

    float calculatePower(float signalLevel) {
        float powerWatts = pow(10, signalLevel / 20.0f);
        return powerWatts;
    }

    char log_buffer[256];
    QTimer   *meter_timer{};

    std::map<QString, QVariant> devList;

    unsigned int sampleRate;
    unsigned int freqStep;
    unsigned int fftSize;
    unsigned int fftrate;
    qint64       tunerFrequency;
    qint64       tunerFrequencyCorrection;
    int         m_HiCutFreq;
    int         m_LowCutFreq;

    DemodulatorCtrl::Demod currentDemod;
    DataSourceCtrlView* sourceView {};
    DemodulatorCtrlView* demodView {};
    AudioPostProcView* audioView = {};
    RTLCtrlView* rTLCtrlView{};
    QTabWidget *ctrls{};

    float               *d_realFftData;
    float               *d_iirFftData;
    float               *d_pwrFftData;
    float               d_fftAvg;
    float               signal_level;

    void initObjects();
    void appentTextBrowser(const char* );    
    int  setFreqStep(int );
    int  setFftRate(int );
    void setPlotterSettings();
    void loadSettings();
    void saveSettings();

    QString m_sSettingsFile;

private slots:   
    void fftTimeout();
    void onFilterChanged();
    void tunerTimeout();
    void on_push_connect_clicked();
    void on_push_exit_clicked();
    void on_plotter_newDemodFreq(qint64 , qint64 );
    void on_plotter_newFilterFreq(int low, int );
    void on_freqStepSelector_currentIndexChanged(const QString &);
    void on_fftRateSelector_currentIndexChanged(const QString &);
    void on_waterFallColor_currentIndexChanged(int );
    void onReceiverStarted();
    void onReceiverStopped();
    void onFreqCtrl_setFrequency(qint64 freq);
    void onSource_setFrequency(qint64 freq);
    void onSource_setFrequencyCorrection(qint64 ppm);
    void setFrequency(qint64 freq);

protected:
    void changeEvent( QEvent* e );
    Receiver *m_Receiver{};
    DemodulatorCtrl *m_Demodulator{};

private:
    Ui::MainWindow *ui;
};



#endif // MainWindow_H
