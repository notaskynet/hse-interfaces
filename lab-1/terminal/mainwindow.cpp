#include "mainwindow.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPlainTextEdit>
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

    m_formatBox = new QComboBox;
    m_formatBox->addItem("ASCII", Ascii);
    m_formatBox->addItem("BIN", Bin);
    m_formatBox->addItem("HEX", Hex);

    auto *clearButton = new QPushButton(tr("Очистить"));

    auto *settingsLayout = new QHBoxLayout;
    settingsLayout->addWidget(new QLabel(tr("Порт:")));
    settingsLayout->addWidget(m_portBox);
    settingsLayout->addWidget(m_refreshButton);
    settingsLayout->addWidget(new QLabel(tr("Скорость:")));
    settingsLayout->addWidget(m_baudBox);
    settingsLayout->addWidget(m_openButton);
    settingsLayout->addStretch();
    settingsLayout->addWidget(new QLabel(tr("Формат:")));
    settingsLayout->addWidget(m_formatBox);
    settingsLayout->addWidget(clearButton);

    const QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_rxView = new QPlainTextEdit;
    m_rxView->setReadOnly(true);
    m_rxView->setFont(monoFont);

    // Окно передачи только отображает данные: сами клавиши обрабатывает eventFilter.
    m_txView = new QPlainTextEdit;
    m_txView->setReadOnly(true);
    m_txView->setFont(monoFont);
    m_txView->setPlaceholderText(tr("Щёлкните сюда и нажимайте клавиши"));
    m_txView->installEventFilter(this);

    auto *rxLayout = new QVBoxLayout;
    rxLayout->addWidget(new QLabel(tr("Принято из порта")));
    rxLayout->addWidget(m_rxView);

    auto *txLayout = new QVBoxLayout;
    txLayout->addWidget(new QLabel(tr("Нажатые клавиши (отправляются в порт)")));
    txLayout->addWidget(m_txView);

    auto *viewsLayout = new QHBoxLayout;
    viewsLayout->addLayout(rxLayout);
    viewsLayout->addLayout(txLayout);

    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->addLayout(settingsLayout);
    mainLayout->addLayout(viewsLayout);
    setCentralWidget(central);

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::togglePort);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearViews);
    connect(m_formatBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::changeFormat);
    connect(m_portBox, &QComboBox::editTextChanged, this, &MainWindow::updateControls);
    connect(&m_port, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(&m_port, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

    refreshPorts();
    updateControls();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_txView && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        const QByteArray data = keyEvent->text().toUtf8();
        if (data.isEmpty())
            return false;       // Shift, Ctrl, стрелки и т. п. - кода символа нет

        sendData(data);
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
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
        m_txView->setFocus();
    } else {
        statusBar()->showMessage(tr("Не удалось открыть %1: %2")
                                     .arg(m_port.portName(), m_port.errorString()));
    }
    updateControls();
}

void MainWindow::readData()
{
    const QByteArray data = m_port.readAll();
    m_rxData.append(data);
    appendText(m_rxView, formatData(data, currentFormat()));
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

void MainWindow::changeFormat()
{
    const Format format = currentFormat();
    m_rxView->setPlainText(formatData(m_rxData, format));
    m_txView->setPlainText(formatData(m_txData, format));
    m_rxView->moveCursor(QTextCursor::End);
    m_txView->moveCursor(QTextCursor::End);
}

void MainWindow::clearViews()
{
    m_rxData.clear();
    m_txData.clear();
    m_rxView->clear();
    m_txView->clear();
}

QString MainWindow::selectedPortName() const
{
    const QString text = m_portBox->currentText();
    const int index = m_portBox->findText(text);
    return index >= 0 ? m_portBox->itemData(index).toString() : text.trimmed();
}

MainWindow::Format MainWindow::currentFormat() const
{
    return static_cast<Format>(m_formatBox->currentData().toInt());
}

QString MainWindow::formatData(const QByteArray &data, Format format)
{
    QString result;
    for (const char c : data) {
        const auto byte = static_cast<unsigned char>(c);
        switch (format) {
        case Ascii:
            if (byte == '\n')
                result += '\n';
            else if (byte == '\r')
                ;               // строки с платы приходят с "\r\n", хватает '\n'
            else if (byte >= 0x20 && byte < 0x7F)
                result += QChar(byte);
            else
                result += '.';  // непечатаемый символ
            break;
        case Bin:
            result += QString::number(byte, 2).rightJustified(8, '0') + ' ';
            break;
        case Hex:
            result += QString::number(byte, 16).rightJustified(2, '0').toUpper() + ' ';
            break;
        }
    }
    return result;
}

void MainWindow::appendText(QPlainTextEdit *view, const QString &text)
{
    view->moveCursor(QTextCursor::End);
    view->insertPlainText(text);
    view->moveCursor(QTextCursor::End);
}

void MainWindow::sendData(const QByteArray &data)
{
    m_txData.append(data);
    appendText(m_txView, formatData(data, currentFormat()));

    if (!m_port.isOpen()) {
        statusBar()->showMessage(tr("Порт закрыт: клавиша показана, но не отправлена"));
        return;
    }
    if (m_port.write(data) != data.size())
        statusBar()->showMessage(tr("Ошибка записи: %1").arg(m_port.errorString()));
}

void MainWindow::updateControls()
{
    const bool open = m_port.isOpen();
    m_openButton->setText(open ? tr("Закрыть") : tr("Открыть"));
    m_openButton->setEnabled(open || !selectedPortName().isEmpty());
    m_portBox->setEnabled(!open);
    m_baudBox->setEnabled(!open);
    m_refreshButton->setEnabled(!open);
}
