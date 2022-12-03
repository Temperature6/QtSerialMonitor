#include "mainwindow.h"
#include <QWindow>
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //窗口初始化
    ui->adjustable_splitter->setStretchFactor(0, 1); //设置水平分裂器比例
    serial = new QSerialPort;
    ScanPort();
    SetComPort();
    //ui->statusBar->setStyleSheet("QStatusBar{background-color:#3a9aeb;}");
    for (int i = 0; i < 9; i++)
    {
        ui->BaudRate->addItem(QString::number(baud_val[i]));
    }
    //Json解析
    QFile file("config.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString config_str = file.readAll();
    file.close();
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(config_str.toUtf8(),&parseJsonErr);

    if(!(parseJsonErr.error == QJsonParseError::NoError))
    {
        ui->set_flow_ctrl->setCurrentIndex(FC_index);
        ui->set_data_bits->setCurrentIndex(DB_index);
        ui->set_parity_bits->setCurrentIndex(PA_index);
        ui->set_stop_bits->setCurrentIndex(SB_index);
        ui->font_size->setValue(15);
        ui->clear_aftersend->setChecked(true);
        ui->is_send_return->setChecked(false);
        ui->isShowTime->setChecked(false);
        ui->BaudRate->setCurrentText("9600");
        ui->duplicate_allowed->setChecked(false);
        ui->isTop->setChecked(false);
    }
    else
    {
        config = document.object();
        ui->set_flow_ctrl->setCurrentIndex(config["FlowContral"].toInt());
        ui->set_data_bits->setCurrentIndex(config["DataBits"].toInt());
        ui->set_parity_bits->setCurrentIndex(config["Parity"].toInt());
        ui->set_stop_bits->setCurrentIndex(config["StopBits"].toInt());
        ui->font_size->setValue(config["FontSize"].toInt());
        ui->clear_aftersend->setChecked(config["ClearAfterSend"].toBool());
        ui->is_send_return->setChecked(config["SendReturn"].toBool());
        ui->isShowTime->setChecked(config["isShowTime"].toBool());
        ui->BaudRate->setCurrentText(config["BaudRate"].toString());
        ui->duplicate_allowed->setChecked(config["DuplicateAllowed"].toBool());
        add_blank = config["AddBlank"].toInt();
        ui->isTop->setChecked(config["isTop"].toBool());
    }
    init = 1;
    SetFontSize();
    ConfigPort();
    BaudRate();

    if (ui->isTop->isChecked())
    {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    else
    {
        setWindowFlags(windowFlags());
    }

    init = 0;
    ui->statusBar->showMessage("就绪");
    //链接槽和信号
    connect(ui->refrush, &QPushButton::clicked, this, &MainWindow::ScanPort);
    connect(ui->Send_btn, &QPushButton::clicked, this, &MainWindow::SendData);
    connect(ui->port_list, &QComboBox::currentTextChanged, this, &MainWindow::SetComPort);
    connect(serial,&QSerialPort::readyRead,this,&MainWindow::ReceiveData);
    connect(ui->operate_port, &QAction::triggered, this, &MainWindow::OperatePort);
    connect(ui->msg, &QTextEdit::textChanged, this, &MainWindow::SeeBottom);
    connect(ui->BaudRate, &QComboBox::currentTextChanged, this, &MainWindow::BaudRate);
    connect(ui->save_msg, &QAction::triggered, this, &MainWindow::SaveMessage);
    connect(ui->operate_port_btn, &QPushButton::clicked, this, &MainWindow::OperatePort);
    connect(ui->font_size, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::SetFontSize);
    connect(ui->set_data_bits, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, &MainWindow::ConfigPort);
    connect(ui->set_flow_ctrl, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, &MainWindow::ConfigPort);
    connect(ui->set_stop_bits, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, &MainWindow::ConfigPort);
    connect(ui->set_parity_bits, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, &MainWindow::ConfigPort);
    connect(ui->isTop, &QCheckBox::clicked, this, &MainWindow::ChangeTopState);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ScanPort()
{
    ui->statusBar->setStyleSheet("QStatusBar{background-color:#3a9aeb;}");
    ui->port_list->clear();
    m_serialPortName.clear();
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        m_serialPortName << info.portName();
    }
    ui->port_list->addItems(m_serialPortName);
    if (ui->port_list->count() != 0)
    {
        ui->operate_port_btn->setEnabled(true);
        ui->operate_port->setEnabled(true);
    }
}

void MainWindow::SendData()
{
    QString data = ui->message->text();
    QString data_cpy = data;
    if (!add_blank)
        data_cpy.remove(QRegExp("\\s")); //去除字符串内的全部空格,注释此行,添加全是空格的项
    if (!data_cpy.isEmpty())
    {
        if (ui->duplicate_allowed->isChecked())
        {
            int found = 0;
            for(int i = 0; i < ui->msg_history->count(); i++)
            {
                if (ui->msg_history->item(i)->text() == data)
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                ui->msg_history->addItem(data);
                ui->msg_history->scrollToBottom();
            }
        }
        else
        {
            ui->msg_history->addItem(data);
            ui->msg_history->scrollToBottom();
        }

    }
    if (serial->isOpen())
    {
        if (ui->is_send_return->isChecked())
        {
            data += "\r\n";
        }
        serial->write(data.toUtf8().data());
    }
    else
    {
        ui->statusBar->showMessage("发送失败,串口未打开");
    }
    if (ui->clear_aftersend->isChecked())
    {
        ui->message->clear();
    }
    return;
}

void MainWindow::ReceiveData()
{
    QString data_str;
    data_str += (QString)serial->readAll();
    if (isInsertN && ui->isShowTime->isChecked())
    {
        isInsertN = 0;
        data_str.insert(0, QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + " -> ");
    }
    if (ui->isShowTime->isChecked())
    {
        //data_str = QDateTime::currentDateTime().toString("|hh:mm:ss.zzz") + " -> ";
        for (int i = 0; i < data_str.length(); i++)
        {
            if (data_str[i] == '\n' && i != data_str.length() - 1)
            {
                data_str.insert(i + 1, QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + " -> ");
            }
            else if (data_str[i] == '\n' && i == data_str.length() - 1)
            {
                isInsertN = 1;
            }
        }
    }
    ui->msg->insertPlainText(data_str);
}

void MainWindow::SetComPort()
{
    if (ui->port_list->count() == 0)
    {
        ui->statusBar->setStyleSheet("QStatusBar{background-color:#3a9aeb;}");
        ui->operate_port_btn->setEnabled(false);
        ui->operate_port->setEnabled(false);
        ui->operate_port_btn->setText("没有串口");
        ui->operate_port->setText("没有串口");
        return;
    }
    serial->close();
    serial->setPortName(ui->port_list->currentText());
    ui->statusBar->showMessage("当前串口为:" + ui->port_list->currentText());
    serial->open(QIODevice::ReadWrite);
    if (serial->isOpen())
    {
        ui->statusBar->setStyleSheet("QStatusBar{background-color:#35ac00;}");
        ui->operate_port->setIcon(QIcon(":/pic/port_close"));
        ui->operate_port->setText("关闭串口");
        ui->operate_port_btn->setIcon(QIcon(":/pic/port_close"));
        ui->operate_port_btn->setText("关闭串口");
        //serial->open(QIODevice::ReadWrite);
    }
    else
    {
        ui->statusBar->setStyleSheet("QStatusBar{background-color:#d7001e;}");
        ui->operate_port->setIcon(QIcon(":/pic/port_open"));
        ui->operate_port->setText("打开串口");
        ui->operate_port_btn->setIcon(QIcon(":/pic/port_open"));
        ui->operate_port_btn->setText("打开串口");
    }
    return;
}

void MainWindow::OperatePort()
{
    if (serial->isOpen())
    {
        serial->close();
        ui->statusBar->setStyleSheet("QStatusBar{background-color:#d7001e;}");
        ui->operate_port->setText("打开串口");
        ui->operate_port->setIcon(QIcon(":/pic/port_open"));
        ui->operate_port_btn->setText("打开串口");
        ui->operate_port_btn->setIcon(QIcon(":/pic/port_open"));
        ui->statusBar->showMessage("串口 " + ui->port_list->currentText() + " 已关闭");
    }
    else
    {
        if (serial->open(QIODevice::ReadWrite))
        {
            ui->statusBar->setStyleSheet("QStatusBar{background-color:#35ac00;}");
            ui->operate_port->setText("关闭串口");
            ui->operate_port->setIcon(QIcon(":/pic/port_close"));
            ui->operate_port_btn->setText("关闭串口");
            ui->operate_port_btn->setIcon(QIcon(":/pic/port_close"));
            ui->statusBar->showMessage("当前串口为:" + ui->port_list->currentText());
        }
        else
        {
            ui->statusBar->setStyleSheet("QStatusBar{background-color:#3a9aeb;}");
            ui->statusBar->showMessage("打开串口失败");
        }
    }
    return;
}

void MainWindow::SeeBottom()
{
    ui->msg->moveCursor(QTextCursor::End);
}

void MainWindow::BaudRate()
{
    serial->setBaudRate(ui->BaudRate->currentText().toInt(),QSerialPort::AllDirections);
}

void MainWindow::SaveMessage()
{
    QString filename = QFileDialog::getSaveFileName(this, "保存文件", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), "文本文档(*.txt)");
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        return;
    }
    else
    {
        QTextStream stream(&file);
        stream << ui->msg->toPlainText();
        stream.flush();
        file.close();
        ui->statusBar->showMessage("文件保存到 "+ filename, 2000);
    }
}

void MainWindow::SetFontSize()
{
    QFont font = ui->msg->font();
    size_msg = ui->font_size->value();
    font.setPixelSize(size_msg);
    ui->msg_history->setFont(font);
    ui->msg->setFont(font);
    ui->message->setFont(font);
    return;
}

void MainWindow::ConfigPort()
{
    if (sender() == ui->set_data_bits || init)
    {
        DB_index = ui->set_data_bits->currentIndex();
        if (serial->setDataBits((QSerialPort::DataBits)data_bits[DB_index]))
        {
            DB_index_old = DB_index;
            ui->statusBar->showMessage("设置数据位:" + ui->set_data_bits->currentText());
        }
        else
        {
            ui->set_data_bits->setCurrentIndex(DB_index_old);
            ui->statusBar->showMessage("设置数据位失败");
        }
    }
    if (sender() == ui->set_flow_ctrl || init)
    {
        FC_index = ui->set_flow_ctrl->currentIndex();
        if (serial->setFlowControl((QSerialPort::FlowControl)flow_control[FC_index]))
        {
            FC_index_old = FC_index;
            ui->statusBar->showMessage("设置流控:" + ui->set_flow_ctrl->currentText());
        }
        else
        {
            ui->set_flow_ctrl->setCurrentIndex(FC_index_old);
            ui->statusBar->showMessage("设置流控失败");
        }

    }
    if (sender() == ui->set_parity_bits || init)
    {
        PA_index = ui->set_parity_bits->currentIndex();
        if (serial->setParity((QSerialPort::Parity)parity[PA_index]))
        {
            PA_index_old = PA_index;
            ui->statusBar->showMessage("设置校验位:" + ui->set_parity_bits->currentText());
        }
        else
        {
            ui->set_parity_bits->setCurrentIndex(PA_index_old);
            ui->statusBar->showMessage("设置校验位失败");
        }
    }
    if (sender() == ui->set_stop_bits || init)
    {
        SB_index = ui->set_stop_bits->currentIndex();
        if (serial->setStopBits((QSerialPort::StopBits)stop_bits[SB_index]))
        {
            SB_index_old = SB_index;
            ui->statusBar->showMessage("设置停止位:" + ui->set_stop_bits->currentText());
        }
        else
        {
            ui->set_stop_bits->setCurrentIndex(SB_index_old);
            ui->statusBar->showMessage("设置停止位失败");
        }
    }
}

void MainWindow::on_msg_history_itemDoubleClicked(QListWidgetItem *item)
{
    if (item == NULL)
        return;
    else
    {
        ui->message->setText(item->text());
    }
}

void MainWindow::ChangeTopState()
{
    if (ui->isTop->isChecked())
    {
        QWidget* pWidget = new QWidget();
        QWindow* pWin = this->windowHandle();
        pWin->setFlags(pWidget->windowFlags() | Qt::WindowStaysOnTopHint);
        delete pWidget;
    }
    else
    {
        QWidget* pWidget = new QWidget();
        QWindow* pWin = this->windowHandle();
        pWin->setFlags(pWidget->windowFlags());
        delete pWidget;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    config["FlowContral"] = ui->set_flow_ctrl->currentIndex();
    config["DataBits"] = ui->set_data_bits->currentIndex();
    config["Parity"] = ui->set_parity_bits->currentIndex();
    config["StopBits"] = ui->set_stop_bits->currentIndex();
    config["FontSize"] = ui->font_size->value();
    config["ClearAfterSend"] = ui->clear_aftersend->isChecked();
    config["SendReturn"] = ui->is_send_return->isChecked();
    config["isShowTime"] = ui->isShowTime->isChecked();
    config["BaudRate"] = ui->BaudRate->currentText();
    config["DuplicateAllowed"] = ui->duplicate_allowed->isChecked();
    config["AddBlank"] = add_blank;
    config["isTop"] = ui->isTop->isChecked();

    QString config_str = QString(QJsonDocument(config).toJson(QJsonDocument::Indented));
    QFile config_file("config.json");
    config_file.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream config_file_stream(&config_file);
    config_file_stream.setCodec("utf-8");
    config_file_stream << config_str;
    config_file.close();
    this->close();
}
