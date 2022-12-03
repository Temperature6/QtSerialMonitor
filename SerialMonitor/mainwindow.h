#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QListWidgetItem>
#include <QStringList>
#include <QSerialPortInfo>
#include <QDebug>
#include <QByteArray>
#include <QDateTime>
#include <QFileDialog>
#include <QStandardPaths>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QArrayData>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void ScanPort();
    void SendData();
    void SetComPort();
    void ReceiveData();
    void OperatePort();
    void SeeBottom();
    void BaudRate();
    void SaveMessage();
    void SetFontSize();
    void ConfigPort();
    void ChangeTopState();
    void closeEvent(QCloseEvent *event);
    ~MainWindow();
private slots:
    void on_msg_history_itemDoubleClicked(QListWidgetItem *item);

private:
    QJsonObject config;
    QSerialPort* serial;
    QStringList m_serialPortName;
    Ui::MainWindow *ui;
    int isInsertN = 0;
    int init = 0;
    int add_blank = 0;
    int size_msg = 10;
    int baud_val[9] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 74800, 115200};

    int flow_control[3] = {
        QSerialPort::NoFlowControl,     //无流控
        QSerialPort::HardwareControl,   //硬件流控
        QSerialPort::SoftwareControl    //软件流控
    };
    int FC_index = 0;
    int FC_index_old = FC_index;

    int data_bits[4] = {
        QSerialPort::Data5,             //5位数据位
        QSerialPort::Data6,             //6位数据位
        QSerialPort::Data7,             //7位数据位
        QSerialPort::Data8              //8位数据位
    };
    int DB_index = 3;
    int DB_index_old = DB_index;

    int parity[5] = {
        QSerialPort::NoParity,          //无校验位
        QSerialPort::EvenParity,        //偶校验位
        QSerialPort::OddParity,         //奇校验位
        QSerialPort::SpaceParity,       //校验位始终为0
        QSerialPort::MarkParity         //校验位始终为1
    };
    int PA_index = 0;
    int PA_index_old = PA_index;

    int stop_bits[3] = {
        QSerialPort::OneStop,           //1位停止位
        QSerialPort::OneAndHalfStop,    //1.5位停止位
        QSerialPort::TwoStop            //2位停止位
    };
    int SB_index = 0;
    int SB_index_old = SB_index;
};

#endif // MAINWINDOW_H
