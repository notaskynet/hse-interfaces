#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QSerialPort>

class QComboBox;
class QPlainTextEdit;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    // Перехватывает нажатия клавиш в окне передачи.
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void refreshPorts();
    void togglePort();
    void readData();
    void handleError(QSerialPort::SerialPortError error);
    void changeFormat();
    void clearViews();

private:
    enum Format { Ascii, Bin, Hex };

    QString selectedPortName() const;
    Format currentFormat() const;
    static QString formatData(const QByteArray &data, Format format);
    static void appendText(QPlainTextEdit *view, const QString &text);
    void sendData(const QByteArray &data);
    void updateControls();

    QSerialPort m_port;

    // Сырые байты хранятся отдельно, чтобы при смене формата перерисовать окна.
    QByteArray m_rxData;
    QByteArray m_txData;

    QComboBox *m_portBox;
    QComboBox *m_baudBox;
    QComboBox *m_formatBox;
    QPushButton *m_refreshButton;
    QPushButton *m_openButton;
    QPlainTextEdit *m_rxView;
    QPlainTextEdit *m_txView;
};
