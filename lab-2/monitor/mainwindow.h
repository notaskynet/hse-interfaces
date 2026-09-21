#pragma once

#include <QElapsedTimer>
#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>

class QComboBox;
class QCustomPlot;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    // Открывает порт сразу при запуске (параметры командной строки).
    void openPort(const QString &name, int baudRate);

private slots:
    void refreshPorts();
    void togglePort();
    void readData();
    void handleError(QSerialPort::SerialPortError error);
    void clearPlot();
    void updatePlot();

private:
    void addSample(int code);
    void updateControls();

    QSerialPort m_port;

    // Время отсчётов считается от открытия порта.
    QElapsedTimer m_clock;
    // График перерисовывается по таймеру, а не на каждый отсчёт.
    QTimer m_replotTimer;
    bool m_dirty = false;
    // Первая строка после открытия порта может быть обрезана: "48" вместо "2048".
    bool m_skipLine = false;
    double m_lastTime = 0;
    int m_badLines = 0;

    QComboBox *m_portBox;
    QComboBox *m_baudBox;
    QPushButton *m_refreshButton;
    QPushButton *m_openButton;
    QLabel *m_valueLabel;
    QCustomPlot *m_plot;
};
