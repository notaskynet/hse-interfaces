#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // monitor --port /dev/ttys003 --baud 19200 - сразу открыть порт.
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"port", "Serial port to open on start.", "name"});
    parser.addOption({"baud", "Baud rate.", "rate", "19200"});
    parser.process(app);

    MainWindow window;
    window.show();
    if (parser.isSet("port"))
        window.openPort(parser.value("port"), parser.value("baud").toInt());

    return app.exec();
}
