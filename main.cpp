#include "gui/gui.hh"
#include "receiver/receiver.hh"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Install log message handler:
    sdr::Logger::get().addHandler(new sdr::StreamLogHandler(std::cerr, sdr::LOG_DEBUG));

    // Instantiate Receiver
    Receiver receiver;

    // Receiver view
    MainWindow win(&receiver);
    win.show();

    // Stop...
    receiver.stop();

    return a.exec();
}
