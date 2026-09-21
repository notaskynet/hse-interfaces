#include "mainwindow.h"

#include "qcustomplot.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QStatusBar>
#include <QVBoxLayout>

namespace {

// АЦП MCP3204: 12 бит, опорное напряжение 4,096 В - 1 мВ на единицу кода.
constexpr int kMaxCode = 4095;
constexpr double kVoltsPerCode = 4.096 / 4096.0;

// Ширина окна графика, с.
constexpr double kWindowSeconds = 10.0;
// Верхняя граница оси кодов, с запасом над 4095.
constexpr double kAxisMaxCode = 4200.0;
// Длиннее строка от платы быть не может ("4095\r\n"); всё, что больше, - мусор.
constexpr int kMaxLineLength = 16;

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Монитор АЦП (SPI, MCP3204)"));
    resize(900, 550);

    m_portBox = new QComboBox;
    m_portBox->setMinimumWidth(180);
    // Имя можно ввести вручную: виртуальных портов (socat) в списке нет.
    m_portBox->setEditable(true);
    m_refreshButton = new QPushButton(tr("Обновить"));

    m_baudBox = new QComboBox;
    const QList<qint32> baudRates = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
    for (qint32 rate : baudRates)
        m_baudBox->addItem(QString::number(rate), rate);
    m_baudBox->setCurrentIndex(m_baudBox->findData(19200));

    m_openButton = new QPushButton;
    auto *clearButton = new QPushButton(tr("Очистить"));

    m_valueLabel = new QLabel(tr("нет данных"));
    m_valueLabel->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    auto *settingsLayout = new QHBoxLayout;
    settingsLayout->addWidget(new QLabel(tr("Порт:")));
    settingsLayout->addWidget(m_portBox);
    settingsLayout->addWidget(m_refreshButton);
    settingsLayout->addWidget(new QLabel(tr("Скорость:")));
    settingsLayout->addWidget(m_baudBox);
    settingsLayout->addWidget(m_openButton);
    settingsLayout->addStretch();
    settingsLayout->addWidget(m_valueLabel);
    settingsLayout->addWidget(clearButton);

    // Одна кривая и две оси Y: слева код АЦП, справа то же самое в вольтах.
    m_plot = new QCustomPlot;
    m_plot->addGraph();
    m_plot->graph(0)->setPen(QPen(QColor(0, 110, 200), 2));
    m_plot->xAxis->setLabel(tr("Время, с"));
    m_plot->xAxis->setRange(0, kWindowSeconds);
    m_plot->yAxis->setLabel(tr("Код АЦП"));
    m_plot->yAxis->setRange(0, kAxisMaxCode);
    m_plot->yAxis2->setVisible(true);
    m_plot->yAxis2->setLabel(tr("Напряжение, В"));
    m_plot->yAxis2->setRange(0, kAxisMaxCode * kVoltsPerCode);

    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->addLayout(settingsLayout);
    mainLayout->addWidget(m_plot, 1);
    setCentralWidget(central);

    m_replotTimer.setInterval(40);

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::togglePort);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearPlot);
    connect(&m_port, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(&m_port, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(&m_replotTimer, &QTimer::timeout, this, &MainWindow::updatePlot);

    refreshPorts();
    updateControls();
}

void MainWindow::refreshPorts()
{
    const QString previous = m_portBox->currentText();

    m_portBox->clear();
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString title = info.portName();
        if (!info.description().isEmpty())
            title += QString(" (%1)").arg(info.description());
        m_portBox->addItem(title, info.portName());
    }

    const int index = m_portBox->findText(previous);
    if (index >= 0)
        m_portBox->setCurrentIndex(index);
    else if (!previous.isEmpty())
        m_portBox->setEditText(previous);

    updateControls();
}

void MainWindow::openPort(const QString &name, int baudRate)
{
    if (m_port.isOpen())
        togglePort();
    m_portBox->setEditText(name);
    const int index = m_baudBox->findData(baudRate);
    if (index >= 0)
        m_baudBox->setCurrentIndex(index);
    togglePort();
}

void MainWindow::togglePort()
{
    if (m_port.isOpen()) {
        m_port.close();
        m_replotTimer.stop();
        updatePlot();
        statusBar()->showMessage(tr("Порт закрыт"));
        updateControls();
        return;
    }

    // Имя из списка или введённое вручную.
    const QString text = m_portBox->currentText().trimmed();
    const int index = m_portBox->findText(text);
    m_port.setPortName(index >= 0 ? m_portBox->itemData(index).toString() : text);
    m_port.setBaudRate(m_baudBox->currentData().toInt());
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (m_port.open(QIODevice::ReadOnly)) {
        m_port.clear();
        clearPlot();
        m_skipLine = true;
        m_clock.start();
        m_replotTimer.start();
        statusBar()->showMessage(tr("Открыт %1, %2 бод, 8N1")
                                     .arg(m_port.portName())
                                     .arg(m_port.baudRate()));
    } else {
        statusBar()->showMessage(tr("Не удалось открыть %1: %2")
                                     .arg(m_port.portName(), m_port.errorString()));
    }
    updateControls();
}

void MainWindow::readData()
{
    // Данные приходят кусками, поэтому разбираем только целые строки.
    while (m_port.canReadLine()) {
        const QByteArray line = m_port.readLine().trimmed();
        if (m_skipLine) {
            m_skipLine = false;
            continue;
        }
        if (line.isEmpty())
            continue;

        bool ok = false;
        const int code = line.toInt(&ok);
        if (ok && code >= 0 && code <= kMaxCode) {
            addSample(code);
        } else {
            ++m_badLines;
            statusBar()->showMessage(tr("Нераспознанных строк: %1 (последняя: \"%2\")")
                                         .arg(m_badLines)
                                         .arg(QString::fromLatin1(line.left(kMaxLineLength))));
        }
    }

    // Нет перевода строки - скорее всего, не та скорость. Не копим мусор в буфере.
    if (m_port.bytesAvailable() > kMaxLineLength) {
        m_port.readAll();
        m_skipLine = true;
        ++m_badLines;
        statusBar()->showMessage(tr("Нет строк в потоке данных, проверьте скорость обмена"));
    }
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    // ResourceError приходит, когда устройство отключили при открытом порте.
    if (error == QSerialPort::ResourceError) {
        statusBar()->showMessage(tr("Ошибка порта: %1").arg(m_port.errorString()));
        m_port.close();
        m_replotTimer.stop();
        updateControls();
    }
}

void MainWindow::clearPlot()
{
    m_plot->graph(0)->data()->clear();
    m_plot->xAxis->setRange(0, kWindowSeconds);
    m_plot->replot();

    m_valueLabel->setText(tr("нет данных"));
    m_badLines = 0;
    m_lastTime = 0;
    m_dirty = false;
    // Время на графике снова идёт с нуля.
    if (m_port.isOpen())
        m_clock.restart();
}

void MainWindow::updatePlot()
{
    if (!m_dirty)
        return;
    m_dirty = false;

    // Окно последних kWindowSeconds секунд; всё, что левее, больше не нужно.
    if (m_lastTime > kWindowSeconds) {
        m_plot->xAxis->setRange(m_lastTime, kWindowSeconds, Qt::AlignRight);
        m_plot->graph(0)->data()->removeBefore(m_lastTime - kWindowSeconds);
    }
    m_plot->replot();
}

void MainWindow::addSample(int code)
{
    m_lastTime = m_clock.elapsed() / 1000.0;
    m_plot->graph(0)->addData(m_lastTime, code);
    m_dirty = true;

    m_valueLabel->setText(tr("Код: %1 (0x%2)   U = %3 В")
                              .arg(code, 4)
                              .arg(QString::number(code, 16).toUpper().rightJustified(4, '0'))
                              .arg(code * kVoltsPerCode, 0, 'f', 3));
}

void MainWindow::updateControls()
{
    const bool open = m_port.isOpen();
    m_openButton->setText(open ? tr("Закрыть") : tr("Открыть"));
    m_portBox->setEnabled(!open);
    m_baudBox->setEnabled(!open);
    m_refreshButton->setEnabled(!open);
}
