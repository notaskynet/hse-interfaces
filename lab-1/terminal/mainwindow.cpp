#include "mainwindow.h"

#include "historyview.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("UART-терминал"));
    resize(800, 500);

    m_portBox = new QComboBox;
    m_portBox->setMinimumWidth(180);
    // Путь можно ввести вручную: виртуальные порты (pty) в список не попадают.
    m_portBox->setEditable(true);
    m_portBox->setInsertPolicy(QComboBox::NoInsert);
    m_refreshButton = new QPushButton(tr("Обновить"));

    m_baudBox = new QComboBox;
    const QList<qint32> baudRates = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
    for (qint32 rate : baudRates)
        m_baudBox->addItem(QString::number(rate), rate);
    m_baudBox->setCurrentIndex(m_baudBox->findData(19200));

    m_openButton = new QPushButton;

    m_viewFormatBox = createFormatBox();
    auto *clearButton = new QPushButton(tr("Очистить"));

    auto *settingsLayout = new QHBoxLayout;
    settingsLayout->addWidget(new QLabel(tr("Порт:")));
    settingsLayout->addWidget(m_portBox);
    settingsLayout->addWidget(m_refreshButton);
    settingsLayout->addWidget(new QLabel(tr("Скорость:")));
    settingsLayout->addWidget(m_baudBox);
    settingsLayout->addWidget(m_openButton);
    settingsLayout->addStretch();
    settingsLayout->addWidget(new QLabel(tr("Отображение:")));
    settingsLayout->addWidget(m_viewFormatBox);
    settingsLayout->addWidget(clearButton);

    m_rxView = new HistoryView(&m_history, Direction::Rx, HistoryView::Layout::Stream);
    m_txView = new HistoryView(&m_history, Direction::Tx, HistoryView::Layout::Lines);

    auto *rxLayout = new QVBoxLayout;
    rxLayout->addWidget(new QLabel(tr("Принято из порта")));
    rxLayout->addWidget(m_rxView);

    auto *txLayout = new QVBoxLayout;
    txLayout->addWidget(new QLabel(tr("Отправлено в порт")));
    txLayout->addWidget(m_txView);

    auto *viewsLayout = new QHBoxLayout;
    viewsLayout->addLayout(rxLayout);
    viewsLayout->addLayout(txLayout);

    // Строка ввода: текст разбирается в выбранном формате и уходит в порт по Enter.
    m_inputFormatBox = createFormatBox();
    m_input = new QLineEdit;
    m_sendButton = new QPushButton(tr("Отправить"));

    auto *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(new QLabel(tr("Ввод:")));
    inputLayout->addWidget(m_inputFormatBox);
    inputLayout->addWidget(m_input);
    inputLayout->addWidget(m_sendButton);

    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->addLayout(settingsLayout);
    mainLayout->addLayout(viewsLayout);
    mainLayout->addLayout(inputLayout);
    setCentralWidget(central);

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::togglePort);
    connect(clearButton, &QPushButton::clicked, &m_history, &DataHistory::clear);
    connect(m_viewFormatBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::changeViewFormat);
    connect(m_inputFormatBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::changeInputFormat);
    connect(m_input, &QLineEdit::returnPressed, this, &MainWindow::sendInput);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendInput);
    connect(m_portBox, &QComboBox::editTextChanged, this, &MainWindow::updateControls);
    connect(&m_port, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(&m_port, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

    changeViewFormat();
    changeInputFormat();
    refreshPorts();
    updateControls();
}

void MainWindow::refreshPorts()
{
    const QString previous = selectedPortName();

    m_portBox->clear();
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString title = info.portName();
        if (!info.description().isEmpty())
            title += QString(" (%1)").arg(info.description());
        m_portBox->addItem(title, info.portName());
    }

    const int index = m_portBox->findData(previous);
    if (index >= 0)
        m_portBox->setCurrentIndex(index);
    else if (!previous.isEmpty())
        m_portBox->setEditText(previous);   // путь, введённый вручную

    updateControls();
}

void MainWindow::togglePort()
{
    if (m_port.isOpen()) {
        m_port.close();
        statusBar()->showMessage(tr("Порт закрыт"));
        updateControls();
        return;
    }

    m_port.setPortName(selectedPortName());
    m_port.setBaudRate(m_baudBox->currentData().toInt());
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (m_port.open(QIODevice::ReadWrite)) {
        statusBar()->showMessage(tr("Открыт %1, %2 бод, 8N1")
                                     .arg(m_port.portName())
                                     .arg(m_port.baudRate()));
        m_input->setFocus();
    } else {
        statusBar()->showMessage(tr("Не удалось открыть %1: %2")
                                     .arg(m_port.portName(), m_port.errorString()));
    }
    updateControls();
}

void MainWindow::readData()
{
    m_history.append(Direction::Rx, m_port.readAll());
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    // ResourceError приходит, когда устройство отключили при открытом порте.
    if (error == QSerialPort::ResourceError) {
        statusBar()->showMessage(tr("Ошибка порта: %1").arg(m_port.errorString()));
        m_port.close();
        updateControls();
    }
}

void MainWindow::changeViewFormat()
{
    const DataFormat *format = formatOf(m_viewFormatBox);
    m_rxView->setFormat(format);
    m_txView->setFormat(format);
}

void MainWindow::changeInputFormat()
{
    m_input->setPlaceholderText(formatOf(m_inputFormatBox)->inputHint());
    m_input->setFocus();
}

void MainWindow::sendInput()
{
    if (!m_port.isOpen()) {
        statusBar()->showMessage(tr("Порт закрыт: данные не отправлены"));
        return;
    }

    QByteArray data;
    QString error;
    if (!formatOf(m_inputFormatBox)->fromText(m_input->text(), &data, &error)) {
        statusBar()->showMessage(tr("Ошибка ввода: %1").arg(error));
        return;     // текст остаётся в поле, чтобы его можно было исправить
    }
    if (data.isEmpty())
        return;

    if (m_port.write(data) != data.size()) {
        statusBar()->showMessage(tr("Ошибка записи: %1").arg(m_port.errorString()));
        return;
    }

    m_history.append(Direction::Tx, data);
    m_input->clear();
    statusBar()->clearMessage();
}

QComboBox *MainWindow::createFormatBox() const
{
    auto *box = new QComboBox;
    for (int i = 0; i < m_formats.count(); ++i)
        box->addItem(m_formats.at(i)->name(), i);
    return box;
}

const DataFormat *MainWindow::formatOf(const QComboBox *box) const
{
    return m_formats.at(box->currentData().toInt());
}

QString MainWindow::selectedPortName() const
{
    const QString text = m_portBox->currentText();
    const int index = m_portBox->findText(text);
    return index >= 0 ? m_portBox->itemData(index).toString() : text.trimmed();
}

void MainWindow::updateControls()
{
    const bool open = m_port.isOpen();
    m_openButton->setText(open ? tr("Закрыть") : tr("Открыть"));
    m_openButton->setEnabled(open || !selectedPortName().isEmpty());
    m_portBox->setEnabled(!open);
    m_baudBox->setEnabled(!open);
    m_refreshButton->setEnabled(!open);
    m_sendButton->setEnabled(open);
}
