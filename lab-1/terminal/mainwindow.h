#pragma once

#include "datahistory.h"
#include "formatregistry.h"

#include <QMainWindow>
#include <QSerialPort>

class HistoryView;
class QComboBox;
class QLineEdit;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshPorts();
    void togglePort();
    void readData();
    void handleError(QSerialPort::SerialPortError error);
    void changeViewFormat();
    void changeInputFormat();
    void sendInput();

private:
    QComboBox *createFormatBox() const;
    const DataFormat *formatOf(const QComboBox *box) const;
    QString selectedPortName() const;
    void updateControls();

    QSerialPort m_port;
    FormatRegistry m_formats = FormatRegistry::createDefault();
    DataHistory m_history;

    QComboBox *m_portBox;
    QComboBox *m_baudBox;
    QComboBox *m_viewFormatBox;
    QComboBox *m_inputFormatBox;
    QPushButton *m_refreshButton;
    QPushButton *m_openButton;
    QPushButton *m_sendButton;
    QLineEdit *m_input;
    HistoryView *m_rxView;
    HistoryView *m_txView;
};
