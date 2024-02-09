/*
FileName	:commissionsetting.cpp
Purpose     :
Authour     :Gururaj B M & Kasturi Rangan.
*/

#include "commissionsetting.h"
#include "ui_commissionsetting.h"
#include "version.h"
#define GROUPSAVE "IP_PORT_GROUP_SAVE"
#define GROUP "IP_PORT_GROUP"
#define SUDOPASWD "Server_sudo_pswd"
#define KEY "IP_key"
#define DEFAULT_IP_PORT "10.96.46.55:9876"
//#define DEFAULT_PORT "9876"

CommissionSetting::CommissionSetting(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CommissionSetting)
{
    ui->setupUi(this);

    QString Title = QString("Data Logger Commissioning Tool V %1.%2.%3").arg(VERSION_MAJ).arg(VERSION_MIN).arg(VERSION_REV);
    this->setWindowTitle(Title);

    // ui->label_password->hide();
    // ui->lEdt_Passwd->hide();
    // ui->pbtn_Passwd_Save->hide();
    ui->m_lblLP_18->setHidden(true);

    QRegularExpression rx("[0-9]{4,5}");
    QValidator *validator = new QRegularExpressionValidator(rx, this);

    ui->lineEdit_APIPort->setValidator(validator);
    ui->lineEdit_DataPort->setValidator(validator);
    ui->lineEdit_AlarmPort->setValidator(validator);

    QRegularExpression rj("([0-9]{1,3}[.]){3}[0-9]{1,3}[:][0-9]{4,5}");
    QValidator *validator1 = new QRegularExpressionValidator(rj, this);
    ui->comboBox_ConnectedIPs->setValidator(validator1);

    Disablefunc();

    QDateTime CurrentEndDateTime = QDateTime::currentDateTime();
    ui->TimeStamp_Date->setDateTime(CurrentEndDateTime);
    ui->timeEdit_Time->setTime(CurrentEndDateTime.time());

    socket_utc = new QTcpSocket(this);
    m_pingThread = new QThread();
    m_pingTimer = new QTimer();
    m_pingTimer->setInterval(10000);
    m_pingTimer->moveToThread(m_pingThread);
    connect(m_pingThread,SIGNAL(started()),m_pingTimer,SLOT(start()));
    connect(m_pingTimer,SIGNAL(timeout()),this,SLOT(CheckConnection()));

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(updateTime()));

    connect(ui->comboBox_databases,SIGNAL(currentTextChanged(QString)),this,SLOT(onComboTextChanged(QString)));

    ReloadDefaultIps();
    ReloadLastSelectedIPs();
    ChangeTimeZone();
    // Default_Alarm_Port();
}

CommissionSetting::~CommissionSetting()
{
    delete ui;
}
void CommissionSetting::updateTime()
{
    //qDebug() << "here"<<":"<<iSecs;
    if(bCalledApi)
        return;

    if(iSecs%60==59)
    {
        bCalledApi=true;
        // QtConcurrent::run(this,&CommissionSetting::UpdateTimezoneUTC);
        UpdateTimezoneUTC();
    }
    else
    {
        iSecs = (iSecs+1)%60;
        QString utcTime = ui->label_UTC_Time->text();
        if(utcTime != "")
        {
            utcTime = utcTime.left(utcTime.length()-2) + QString::number(iSecs).rightJustified(2, '0');
        }
        QString localeTime = ui->label_Locale_time->text();
        if(localeTime != "")
        {
            localeTime = localeTime.left(localeTime.length()-2) + QString::number(iSecs).rightJustified(2, '0');;
        }
        ui->label_UTC_Time->setText(utcTime);
        ui->label_Locale_time->setText(localeTime);
    }

}
void CommissionSetting::ReloadDefaultIps()
{
    QSettings setting("Finecho_IP_Port_Save","IP_Port_Details_Save");
    setting.beginGroup(GROUPSAVE);
    foreach (const QString &key, setting.childKeys())
    {
        QString value = setting.value(key).toString();
        ui->comboBox_ConnectedIPs->addItem(value);
    }
    setting.endGroup();
}

void CommissionSetting::onComboTextChanged(QString port)
{
    Q_UNUSED(port);

    if(ui->comboBox_databases->currentIndex()==-1)
    {
        //  qDebug() <<"index out of range";
    }else{
        sourceCurrentIndex = ui->comboBox_databases->currentIndex();
        QString source = ui->comboBox_databases->currentText();
       // ui->comboBox_databases->setMinimumWidth(source.length()*12);
    }

    if(Data_Port.size()==Data_Sources.size() && Data_Port.size()>0){

        //QString currentPort = Data_Port.at(sourceCurrentIndex);
        ui->lineEdit_DataPort->setText(Data_Port.at(sourceCurrentIndex));
    }
}

void CommissionSetting::ReloadLastSelectedIPs()
{
    QSettings settings("Finecho_IP_Port","IP_Port_Details");
    settings.beginGroup(GROUP);
    QString value = settings.value(KEY).toString();
    ui->comboBox_ConnectedIPs->setCurrentText(value);
    //ui->comboBox_ConnectedIPs->setMinimumWidth(value.length()*12);
    QStringList strlst = value.split(':');
    int dSize = value.length();
    QString strIP,strPort;
    if(dSize > 0){
        strIP = strlst.at(0);
        strPort = strlst.at(1);
        ConnectToIp_Port(strIP,strPort);
    }
    else
    {
        strIP = "127.0.0.1";
        strPort = "6789";
        ConnectToIp_Port(strIP,strPort);
    }
    settings.endGroup();
}

void CommissionSetting::ConnectToIp_Port(QString strIP, QString strPort)
{
    socket = new QTcpSocket(this);

    socket->connectToHost(strIP, strPort.toInt());

    if(socket->waitForConnected(10000))
    {
        //bool statusConn = socket->ConnectedState;

        m_pingThread->exit();
        QSettings setting("Finecho_IP_Port_Save","IP_Port_Details_Save");
        setting.beginGroup(GROUPSAVE);
        int i=0;
        QStringList lst = setting.allKeys();
        if(lst.length() == 0)
        {
            setting.setValue(strIP, strIP+":"+strPort);
        }
        foreach (const QString &key, setting.childKeys())
        {
            int num = lst.length();
            QString value = setting.value(key).toString();
            if(value != strIP+":"+strPort )
            {
                i++;
            }
            if(num == i)
            {
                setting.setValue(strIP, strIP+":"+strPort);
            }
        }
        QStringList strlst;
        foreach (const QString &key, setting.childKeys())
        {
            QString value = setting.value(key).toString();
            strlst << value;
            ui->comboBox_ConnectedIPs->addItem(value);
        }
        strlst.removeDuplicates();

        ui->comboBox_ConnectedIPs->clear();
        ui->comboBox_ConnectedIPs->addItems(strlst);
        ui->comboBox_ConnectedIPs->setCurrentText(strIP+":"+strPort);

        // ui->label_password->setText(strIP +" SU Credentials");
        setting.endGroup();

        ui->lbl_Connection->setStyleSheet("background-color:green;");
        ui->pushButton_connect->setText("Connected");

        ui->lbl_error_msg->setText("");

        // close the connection
        socket->close();
        //============================
        Str_IPAddress = strIP;
        Str_port = strPort;

        QSettings settings("Finecho_IP_Port","IP_Port_Details");
        settings.beginGroup(GROUP);
        settings.setValue(KEY, strIP+":"+strPort);
        settings.endGroup();

        m_pingThread->start();
        timer->start(1000);
        iSecs = 59;
        Enablefunc();
        showDefaultPorts();

        Default_Alarm_Port();

        //  showFirstTimePassword();


        socket1 = new QTcpSocket(this);
        if(socket1->state() == QAbstractSocket::UnconnectedState)
        {
            socket1->connectToHost(strIP, strPort.toInt());
        }
    }
    else
    {
        m_pingThread->exit();
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_error_msg->setText("Not Connected");
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        // showFirstTimePassword();
    }
}

void CommissionSetting::on_pushButton_connect_clicked()
{
    QString strIPAddress, strIPPort, strIP;
    QStringList strParts;
    bool bFlag=false;

    strIP = ui->comboBox_ConnectedIPs->currentText();
    strParts = strIP.split(":");
    strIPAddress = strParts.at(0);
    if(strParts.count()<2)
        strIPPort = DEFAULT_PORT;
    else
        strIPPort = strParts.at(1);

    if(strIPAddress.split(".").count()==4)
    {
        QHostAddress address(strIPAddress);
        if(QAbstractSocket::IPv4Protocol==address.protocol())
        {
            bFlag= true;
        }
        else if(QAbstractSocket::IPv6Protocol==address.protocol())
        {
            bFlag= true;
        }
        else
        {
            QMessageBox::information(this,"Warning","Not a Valid IP Address");
            bFlag= false;
        }
    }
    else
    {
        QMessageBox::information(this,"Warning","Not a Valid IP Address");
        bFlag= false;
    }

    if(bFlag==true)
    {
        if(ui->comboBox_ConnectedIPs->findText(strIP,Qt::MatchExactly) == -1)
        {
            //Save to local disk here
        }
        ConnectToIp_Port(strIPAddress,strIPPort);
    }
}

void CommissionSetting::on_comboBox_ConnectedIPs_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    Disablefunc();
    m_pingThread->exit();
    ui->pushButton_connect->setText("Connect");

    //    if(ui->radioBtn_Alarm640->isChecked()){
    //        ui->radioBtn_Alarm640->setChecked(false);
    //        qDebug() << "640";
    //    }else{
    //        ui->radioBtn_Alarm1280->setChecked(false);
    //        qDebug() << "1280";
    //    }

    ui->label_Alarm_message->setText("");
    ui->comboBox_databases->clear();
    clearWidgets();
    timer->stop();
    ui->lbl_Connection->setStyleSheet("background-color:red;");
}
void CommissionSetting::UpdateTimezoneUTC()
{
    QJsonObject jsonKeys_serverUTCTime;
    QJsonDocument jsonDoc_serverUTCTime;
    QJsonArray jsonArray_serverUTCTime;

    jsonKeys_serverUTCTime.insert("username","rushikesh");
    jsonKeys_serverUTCTime.insert("passcode","finecho@178");
    jsonKeys_serverUTCTime.insert("authkey","abcd");
    jsonKeys_serverUTCTime.insert("endpoint","showUTCtimeAndLocaltime");
    jsonArray_serverUTCTime.append(jsonKeys_serverUTCTime);
    jsonDoc_serverUTCTime = QJsonDocument(jsonArray_serverUTCTime);
    QByteArray data;
    QByteArray jByte(jsonDoc_serverUTCTime.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));


    socket_utc->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket_utc->waitForConnected(10000))
    {
        socket_utc->write(strLen.toUtf8());
        socket_utc->waitForBytesWritten(30000);

        socket_utc->write(jByte);
        socket_utc->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket_utc->waitForReadyRead(1000);
           // QApplication::setOverrideCursor(Qt::WaitCursor);
            if(socket_utc->state() != QAbstractSocket::ConnectedState){
               // QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket_utc->readAll();
            usleep(50000);
        }

    } else{

        QMessageBox msgBox;
        msgBox.setText("Error: " + socket_utc->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        bCalledApi=false;
        m_pingThread->exit();
        timer->stop();
        return;
    }

    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString utctime;
    QString localtime;
    QString localtimezone;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    utctime = fieldObj.value("utctime").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    localtime = fieldObj.value("localtime").toString();

    fieldSet = fieldDefJSONArray.at(2);
    fieldObj = fieldSet.toObject();
    localtimezone = fieldObj.value("localtimezone").toString();

    socket_utc->close();

    if(utctime != "")
    {
        ui->label_UTC_Time->setText(utctime.trimmed());
        ui->label_Locale_time->setText("[" + localtimezone + "]" + " " + localtime.trimmed());
        iSecs = utctime.trimmed().right(2).toInt();
        ilocSecs = localtime.trimmed().right(2).toInt();
        bCalledApi=false;
    }
    else
    {
        ui->label_UTC_Time->setText("");
        ui->label_Locale_time->setText("");
        bCalledApi=false;
    }
}

void CommissionSetting::CheckConnection()
{
    if(socket1->state() == QAbstractSocket::UnconnectedState)
    {
        socket1->connectToHost(Str_IPAddress, Str_port.toInt());
    }

    if(socket1->state() == QAbstractSocket::ConnectedState)
    {
        ui->lbl_Connection->setStyleSheet("background-color:green;");
        ui->pushButton_connect->setText("Connected");
        ui->lbl_error_msg->setText("");
    }
    else
    {
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();

    }
}
void CommissionSetting::clearWidgets()
{
    ui->label_UTC_Time->clear();
    ui->label_AWLFile->clear();
    ui->label_Locale_time->clear();
    ui->lineEdit_DataPort->clear();
    ui->lineEdit_AlarmPort->clear();
    ui->lineEdit_APIPort->clear();

    if(ui->radioBtn_Alarm640->isChecked()){
        ui->radioBtn_Alarm640->setAutoExclusive(false);
        ui->radioBtn_Alarm640->setChecked(false);
        ui->radioBtn_Alarm640->setAutoExclusive(true);
    }else{
        ui->radioBtn_Alarm1280->setAutoExclusive(false);
        ui->radioBtn_Alarm1280->setChecked(false);
        ui->radioBtn_Alarm1280->setAutoExclusive(true);
    }

}

void CommissionSetting::ChangeTimeZone()
{
    QStringList strLstlines;
    QFile file(qApp->applicationDirPath()+"/timezones.txt");
    if(!file.open(QFile::ReadOnly |
                  QFile::Text))
    {
        qDebug() << " Could not open the file for reading";
        return;
    }
    while (!file.atEnd())
    {
        QByteArray line = file.readLine();
        strLstlines << QString(line).simplified();
    }
    ui->comboBox_Locale->addItems(strLstlines);
    file.close();
}

void CommissionSetting::on_m_PshBtn_UTC_Time_clicked()
{
    QDateTime startDateTime;
    QByteArray data;
    startDateTime = ui->TimeStamp_Date->dateTime();
    QString DTime = startDateTime.toString("yyyy-MM-dd hh:mm:ss");

    QJsonObject jsonKeys_serverUTCTime;
    QJsonDocument jsonDoc_serverUTCTime;
    QJsonArray jsonArray_serverUTCTime;

    QString password;
    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/

    jsonKeys_serverUTCTime.insert("username","rushikesh");
    jsonKeys_serverUTCTime.insert("passcode","finecho@178");
    jsonKeys_serverUTCTime.insert("authkey","abcd");
    jsonKeys_serverUTCTime.insert("endpoint","changeUTCtime");
    // jsonKeys_serverUTCTime.insert("password",password);
    jsonKeys_serverUTCTime.insert("newTime",DTime);
    jsonArray_serverUTCTime.append(jsonKeys_serverUTCTime);
    jsonDoc_serverUTCTime = QJsonDocument(jsonArray_serverUTCTime);
    //qDebug() << "jsonDoc_serverUTCTime===" << jsonDoc_serverUTCTime;
    QByteArray jByte(jsonDoc_serverUTCTime.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));
    //qDebug() << "strLen===" << strLen;
    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);

            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }
    } else
    {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("Description").toString();
    socket->close();

    if(result == ""){
        result = "Failed";
    }
    if(result == "Success"){
        iSecs = 59;
    }

    QString message = QString("Change Server UTC Time: " + result + "\n" + Description);
    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_m_PshBtn_LocaleTime_clicked()
{
    QJsonObject jsonKeys_serverLocaleTime;
    QJsonDocument jsonDoc_serverLocaleTime;
    QJsonArray jsonArray_serverLocaleTime;

    QString timeZone = ui->comboBox_Locale->currentText();
    QString password ;
    /*= decryptData(Str_IPAddress);

    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/
    jsonKeys_serverLocaleTime.insert("username","rushikesh");
    jsonKeys_serverLocaleTime.insert("passcode","finecho@178");
    jsonKeys_serverLocaleTime.insert("authkey","abcd");
    jsonKeys_serverLocaleTime.insert("endpoint","changeTimeZone");
    //jsonKeys_serverLocaleTime.insert("password",password);
    jsonKeys_serverLocaleTime.insert("timezone",timeZone);
    jsonArray_serverLocaleTime.append(jsonKeys_serverLocaleTime);
    jsonDoc_serverLocaleTime = QJsonDocument(jsonArray_serverLocaleTime);

    QByteArray data;
    QByteArray jByte(jsonDoc_serverLocaleTime.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    //  qDebug() << jsonDoc_serverLocaleTime;

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    } else
    {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    //qDebug() << data;
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("Description").toString();

    socket->close();

    if(result == ""){
        result = "Failed";
    }
    if(result == "Success"){
        iSecs = 59;
    }

    QString message = QString("Change Server Locale Time: " + result + "\n" + Description);
    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_toolButton_AWLFile_clicked()
{
    QStringList splitter;
    Awl_filePath  = QFileDialog::getOpenFileName(this,tr("Load From File"),"",tr("All files (*.db *.AWL *.awl *.DB);;"));
    splitter =  Awl_filePath.split("/");
    int nShpCnt = splitter.count() - 1;
    QString Path = splitter.at(nShpCnt);

    ui->label_AWLFile->setText(Path);
    // QString subString = Path.mid(0,24);
}
void CommissionSetting::on_m_PshBtn_AWLFileUpload_clicked()
{
    QStringList strLstlines;
    QJsonDocument jsonDoc,jsonDocNew;
    QByteArray line;
    QFile file(Awl_filePath);
    if (!file.open(QIODevice::ReadOnly))   //|| ui->label_AWLFile->text()=="")
    {
       // QMessageBox msgBox;
       // msgBox.setText(file.errorString());
        QString message = file.errorString();
        QMessageBox::information(this,"Result", message );
       // msgBox.exec();
        return;
    }

    while (!file.atEnd())
    {
        line = file.readLine();
        strLstlines << QString(line).simplified();
    }

    Awl_filePath.clear();
    file.close();

    QJsonObject jsonAPI,jsonKeys;
    QJsonArray json_lineArray,line_array;
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("port",ui->lineEdit_DataPort->text());
    jsonKeys.insert("source",ui->comboBox_databases->currentText());
    jsonKeys.insert("endpoint","awlfile");
    QJsonArray param_lines;

    for (int i=0;i < strLstlines.length();i++)
    {
        param_lines.append(strLstlines.at(i).trimmed());
    }

    jsonKeys.insert("line",QJsonValue(param_lines));
    json_lineArray.append(jsonKeys);
    jsonDoc = QJsonDocument(json_lineArray);

    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());

    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        QJsonParseError json_error;
        QByteArray data;

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);

            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(500000);
        }

        QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
        if (json_error.error != QJsonParseError::NoError)
        {
            qDebug() << "JSON parse error: "<< json_error.errorString();
        }

        QJsonValue fieldSet;
        QJsonObject fieldObj;
        QString result;
        QString Description_af_create,Description_af_update,Description_at_create;
        QString serviceStatus;

        QJsonObject rootObj = loadDoc.object();
        QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();


        fieldSet = fieldDefJSONArray.at(0);
        fieldObj = fieldSet.toObject();
        result = fieldObj.value("result").toString();

        if(result == "success"){
            fieldSet=fieldDefJSONArray.at(4);
            fieldObj=fieldSet.toObject();
            serviceStatus=fieldObj.value("service").toString();
        }

        fieldSet = fieldDefJSONArray.at(1);
        fieldObj = fieldSet.toObject();
        Description_af_create = fieldObj.value("Description_af_create").toString();

        fieldSet = fieldDefJSONArray.at(2);
        fieldObj = fieldSet.toObject();
        Description_af_update = fieldObj.value("Description_af_update").toString();

        fieldSet = fieldDefJSONArray.at(3);
        fieldObj = fieldSet.toObject();
        Description_at_create = fieldObj.value("Description_at_create").toString();

        socket->close();

        QString message = QString("AWL_Configuration : " + result + "\n" + Description_af_create + "\n" + Description_af_update + "\n" +Description_at_create +"\n"+ serviceStatus);
        QMessageBox::information(this,"Result", message );

        socket->close();
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
}
void CommissionSetting::on_comboBox_ConnectedIPs_currentIndexChanged(const QString &arg1)
{
    //    ui->lEdt_Passwd->clear();
    //    QString strIP;
    //    QString add;
    //    strIP = arg1;
    //    QStringList strlst = strIP.split(':');
    //    int dSize = strlst.length();
    //    if(dSize > 0)
    //    {
    //        add = strlst.at(0);
    //    }
    //    ui->label_password->setText(add +" SU Credentials");
    //    QSettings setting("Finecho_SudoPaswd","Finecho_SudoPaswd_Details");
    //    setting.beginGroup(SUDOPASWD);
    //    ui->lEdt_Passwd->setText(setting.value(add).toString());
    //    setting.endGroup();
    Q_UNUSED(arg1);

}

void CommissionSetting::on_m_PshBtn_PLCDataPortSave_clicked()
{
    //    QString dataPort = ui->lineEdit_DataPort->text();
    if (Data_Port.size()>0)  oldPortData = Data_Port.at(sourceCurrentIndex);

    currentPort=ui->lineEdit_DataPort->text();
    if(currentPort == "")
    {
        QString Portmsg = QString("Please enter the port number");
        QMessageBox::information(this,"Erorr",Portmsg );
        return;
    }

    if(currentPort.toInt() < 4096 || currentPort.toInt()>65535){
        QMessageBox msgBox;
        msgBox.setText("Enter the valid Data Port Number \n"
                       "Port Range should be between 4096 and 65535");
        msgBox.exec();
        ui->lineEdit_DataPort->setText(oldPortData);
        return;
    }

    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString password ;
    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","PortChangeForData");
    //jsonKeys.insert("password",password);
    jsonKeys.insert("port",currentPort);
    jsonKeys.insert("oldport",oldPortData);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);

    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);

            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    }else {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;
    QString status ;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    if(result=="success"){
        if(Data_Port.size()>0){
            Data_Port[sourceCurrentIndex]=currentPort;
        }
        fieldSet = fieldDefJSONArray.at(2);
        fieldObj = fieldSet.toObject();
        status = fieldObj.value("service").toString();

    }else if(result=="fail" || result =="failed" || result == "failed_1"){
        ui->lineEdit_DataPort->setText(oldPortData);
    }

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("reason").toString();

    socket->close();

    QString message = QString("Change Data Port: " + result + "\n" + Description +"\n"+ status);

    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_m_PshBtn_PLCAlarmPortSave_clicked()
{
    QString dataPort = ui->lineEdit_AlarmPort->text();

    if(dataPort == "")
    {
        QString Portmsg = QString("Please enter the port number");
        QMessageBox::information(this,"Erorr",Portmsg );
        return;
    }

    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString password;


    if(dataPort.toInt() < 4096 || dataPort.toInt()>65535){
        QMessageBox msgBox;
        msgBox.setText("Enter the valid Alarm Port Number \n"
                       "Port Range should be between 4096 and 65535");
        msgBox.exec();
        ui->lineEdit_AlarmPort->setText(oldPortAlarm);
        return;
    }

    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","PortChangeForAlarms");
    //jsonKeys.insert("password",password);
    jsonKeys.insert("port",dataPort);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);

    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);

            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    }else {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();

    //qDebug() << rootObj;

    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;
    QString status;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    if(result=="fail" || result =="failed"){
        ui->lineEdit_AlarmPort->setText(oldPortAlarm);
    }
    else{
        oldPortAlarm=dataPort;
        fieldSet=fieldDefJSONArray.at(2);
        fieldObj=fieldSet.toObject();
        status = fieldObj.value("service").toString();
    }

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("reason").toString();

    socket->close();

    QString message = QString("Change Alarm Port: " + result + "\n" + Description +"\n"+status);
    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_m_PshBtn_API_Set_clicked()
{
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString apiPort = ui->lineEdit_APIPort->text();
    if(apiPort == "")
    {
        QString Portmsg = QString("Please enter the port number");
        QMessageBox::information(this,"Erorr",Portmsg );
      //  ui->lineEdit_APIPort->setText(oldPortDataloggerAPI);
        return;
    }

    if(apiPort.toInt() < 4096 || apiPort.toInt()>65535){
        QMessageBox msgBox;
        msgBox.setText("Enter the valid  Port Number \n"
                       "Port Range should be between 4096 and 65535");
        msgBox.exec();
        ui->lineEdit_APIPort->setText(oldPortDataloggerAPI);
        return;
    }


    QString password ;
    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","dataloggerapiPortChange");
    jsonKeys.insert("port",apiPort);
    // jsonKeys.insert("password",password);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);
    QByteArray data;

    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    // qDebug() << jsonDoc;

    socket->connectToHost(Str_IPAddress,Str_port.toInt());

    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }


    }else{

        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        socket->close();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();

    // qDebug() << rootObj;

    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;
    QString status;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    if(result=="fail" || result =="failed"){
        ui->lineEdit_APIPort->setText(oldPortDataloggerAPI);
    }
    else{
        oldPortDataloggerAPI=apiPort;
        fieldSet=fieldDefJSONArray.at(2);
        fieldObj=fieldSet.toObject();
        status=fieldObj.value("service").toString();
    }

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("Description").toString();

    socket->close();

    QString message = QString("Datalogger-API-PortChange: " + result + "\n" + Description+"\n"+status);
    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_m_PshBtn_API_Check_clicked()
{
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString apiPort = ui->lineEdit_APIPort->text();
    if(apiPort == "")
    {
        QString Portmsg = QString("Please enter the port number");
        QMessageBox::information(this,"Erorr",Portmsg );
        return;
    }
    QString password ;
    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","checkDataloggerAPIservice");
    // jsonKeys.insert("password",password);
    jsonKeys.insert("port",apiPort);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);
    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    //  qDebug() << jsonDoc;

    socket->connectToHost(Str_IPAddress,Str_port.toInt());

    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }


    } else {
        QMessageBox msgBox;
        // qDebug() << socket->state();
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        socket->close();
        return;
    }

    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();

    //qDebug() << rootObj;

    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("status").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("Description").toString();

    socket->close();

    QString message = QString("API Service Status: " + result + "\n" + Description);
    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_m_PshBtn_API_Reset_clicked()
{
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString password ;
    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/


//    if(ui->lineEdit_APIPort->text() == "6789"){

//        QString Portmsg = QString("Default port already set");
//        QMessageBox::information(this,"Result",Portmsg );
//        return;
//    }

    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","RestoreAndDefault");
    // jsonKeys.insert("password",password);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);
    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());

    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    }else {

        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        socket->close();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;
    QString serviceStatus;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("Description").toString();

    socket->close();

    if(result == "success"){
        ui->lineEdit_APIPort->setText("6789");
        oldPortDataloggerAPI="6789";
        fieldSet=fieldDefJSONArray.at(2);
        fieldObj=fieldSet.toObject();
        serviceStatus=fieldObj.value("service").toString();
    }else if(Description == "Provided port is busy,Please provide different port no."){
        ui->lineEdit_APIPort->setText("6789");
    }

    QString message = QString("Reset port: " + result + "\n" + Description+"\n"+serviceStatus);
    QMessageBox::information(this,"Result", message );
}

QString CommissionSetting::encryptData(QString strPswd)
{
    QByteArray ba;
    ba.append(strPswd);
    return ba.toBase64();
}

QString CommissionSetting::decryptData(QString strIP)
{
    QString DecryptStr;
    QSettings setting("Finecho_SudoPaswd","Finecho_SudoPaswd_Details");
    setting.beginGroup(SUDOPASWD);
    DecryptStr =  setting.value(strIP).toString();
    QByteArray ba;
    ba.append(DecryptStr);
    return QByteArray::fromBase64(ba);
}

/*void CommissionSetting::on_pbtn_Passwd_Save_clicked() // not calling
{
    if(ui->lEdt_Passwd->text() == "")
    {
        QString message = QString(" IP does not have sudo password");
        QMessageBox::information(this,"Erorr", message );
        return;
    }
    QString IPName_Paswd = ui->lEdt_Passwd->text();
    QString IPName = ui->label_password->text().trimmed();
    QStringList strlst = IPName.split(" ");
    int dSize = strlst.length();
    QString strIP,strPort;
    if(dSize > 0)
    {
        strIP = strlst.at(0);
    }
    QString srcString = IPName_Paswd;
    QString encodedString = encryptData(srcString);
    QSettings setting("Finecho_SudoPaswd","Finecho_SudoPaswd_Details");
    setting.beginGroup(SUDOPASWD);
    foreach (const QString &key, setting.childKeys())
    {
        QString value = key;
        if(value == strIP)
        {
            int nflag = 0;
            QMessageBox qmsgMessageBox;
            nflag = qmsgMessageBox.warning(this,"Alert","Do you want Replace " + strIP + " sudo password ?...",QMessageBox::Yes, QMessageBox::No);
            if(nflag == QMessageBox::Yes)
            {
                setting.remove(key);
            }
            else
            {
                return;
            }
        }
    }
    setting.setValue(strIP,encodedString);
    QString message = QString("Sudo password set successfully");
    QMessageBox::information(this,"Success", message );
    setting.endGroup();
}    */

void CommissionSetting::Enablefunc()
{
    ui->m_PshBtn_UTC_Time->setEnabled(true);
    ui->m_PshBtn_LocaleTime->setEnabled(true);
    ui->m_PshBtn_PLCDataPortSave->setEnabled(true);
    ui->m_PshBtn_PLCDataPortArrival->setEnabled(true);
    ui->m_PshBtn_DataPort_Reset->setEnabled(true);
    ui->m_PshBtn_AlarmPort_Reset->setEnabled(true);
    ui->m_PshBtn_PLCAlarmPortArrival->setEnabled(true);
    ui->m_PshBtn_PLCAlarmPortSave->setEnabled(true);
    ui->m_PshBtn_AWLFileUpload->setEnabled(true);
    ui->m_PshBtn_Alarm->setEnabled(true);
    ui->m_PshBtn_API_Set->setEnabled(true);
    ui->m_PshBtn_API_Check->setEnabled(true);
    ui->m_PshBtn_API_Reset->setEnabled(true);
    ui->lineEdit_AlarmPort->setEnabled(true);
    ui->radioBtn_Alarm640->setEnabled(true);
    ui->radioBtn_Alarm1280->setEnabled(true);
    ui->toolButton_AWLFile->setEnabled(true);
    ui->lineEdit_APIPort->setEnabled(true);
    ui->lineEdit_DataPort->setEnabled(true);
    ui->lineEdit_AlarmPort->setEnabled(true);

}
void CommissionSetting::Disablefunc()
{
    ui->m_PshBtn_UTC_Time->setEnabled(false);
    ui->m_PshBtn_LocaleTime->setEnabled(false);
    ui->m_PshBtn_PLCDataPortSave->setEnabled(false);
    ui->m_PshBtn_PLCDataPortArrival->setEnabled(false);
    ui->m_PshBtn_AlarmPort_Reset->setEnabled(false);
    ui->m_PshBtn_DataPort_Reset->setEnabled(false);
    ui->m_PshBtn_PLCAlarmPortArrival->setEnabled(false);
    ui->m_PshBtn_PLCAlarmPortSave->setEnabled(false);
    ui->m_PshBtn_AWLFileUpload->setEnabled(false);
    ui->m_PshBtn_Alarm->setEnabled(false);
    ui->m_PshBtn_API_Set->setEnabled(false);
    ui->m_PshBtn_API_Check->setEnabled(false);
    ui->m_PshBtn_API_Reset->setEnabled(false);
    ui->toolButton_AWLFile->setEnabled(false);
    ui->lineEdit_APIPort->setEnabled(false);
    ui->lineEdit_DataPort->setEnabled(false);
    ui->lineEdit_AlarmPort->setEnabled(false);

    ui->radioBtn_Alarm640->setEnabled(false);
    ui->radioBtn_Alarm1280->setEnabled(false);

}

void CommissionSetting::on_timeEdit_Time_timeChanged(const QTime &time)
{
    ui->TimeStamp_Date->setTime(time);
}

void CommissionSetting::on_m_PshBtn_Alarm_clicked()
{
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QByteArray data;
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    if(ui->radioBtn_Alarm640->isChecked())
    {
        jsonKeys.insert("endpoint","Alarm_Confugration_640");
        jsonKeys.insert("alarm_count","640");
    }
    else
    {
        jsonKeys.insert("endpoint","Alarm_Confugration_1280");
        jsonKeys.insert("alarm_count","1280");
    }
    jsonKeys.insert("source",ui->comboBox_databases->currentText());

    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);

    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    }else {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description_af_create,Description_af_update,Description_at_create;
    QString serviceStatus;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    if(result == "success"){
        fieldSet=fieldDefJSONArray.at(4);
        fieldObj=fieldSet.toObject();
        serviceStatus=fieldObj.value("service").toString();
    }

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description_af_create = fieldObj.value("Description_af_create").toString();

    fieldSet = fieldDefJSONArray.at(2);
    fieldObj = fieldSet.toObject();
    Description_af_update = fieldObj.value("Description_af_update").toString();

    fieldSet = fieldDefJSONArray.at(3);
    fieldObj = fieldSet.toObject();
    Description_at_create = fieldObj.value("Description_at_create").toString();

    socket->close();

    QString message = QString("Alarm_Confugration : " + result + "\n" + Description_af_create + "\n" + Description_af_update + "\n" +Description_at_create+"\n"+serviceStatus);
    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_m_PshBtn_PLCDataPortArrival_clicked()
{
    QString date_string = ui->label_UTC_Time->text().trimmed();
    QDateTime UTCDateTime = QDateTime::fromString(date_string,"dd-MM-yyyy hh:mm:ss");
    QString str_UTCDateTime = UTCDateTime.toString("yyyy-MM-dd hh:mm:ss");

    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString password;

    if(ui->lineEdit_DataPort->text() == "")
    {
        QString Portmsg = QString("Please enter the port number");
        QMessageBox::information(this,"Erorr",Portmsg );
        return;
    }

    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/

    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","PlcDataloginStatus");
    jsonKeys.insert("port",ui->lineEdit_DataPort->text());
    jsonKeys.insert("source",ui->comboBox_databases->currentText());
    jsonKeys.insert("UTCTime",str_UTCDateTime);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);
    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);

            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    }else {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("Description").toString();

    socket->close();

    QString message = QString("PlcDataloginStatus: " + result + "\n" + Description);
    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_m_PshBtn_PLCAlarmPortArrival_clicked()
{
    QString date_string = ui->label_UTC_Time->text().trimmed();
    QDateTime UTCDateTime = QDateTime::fromString(date_string,"dd-MM-yyyy hh:mm:ss");
    QString str_UTCDateTime = UTCDateTime.toString("yyyy-MM-dd hh:mm:ss");
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString password;

    if(ui->lineEdit_AlarmPort->text() == "")
    {
        QString Portmsg = QString("Please enter the port number");
        QMessageBox::information(this,"Erorr",Portmsg );
        return;
    }

    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","AlarmDataloginStatus");
    jsonKeys.insert("port",ui->lineEdit_AlarmPort->text());
    jsonKeys.insert("source",ui->comboBox_databases->currentText());
    jsonKeys.insert("UTCTime",str_UTCDateTime);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);
    QByteArray data;

    //qDebug() << jsonDoc;

    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    }else {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("Description").toString();

    socket->close();

    QString message = QString("AlarmDataloginStatus: " + result + "\n" + Description);
    QMessageBox::information(this,"Result", message );
}
void CommissionSetting::showDefaultPorts()
{
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","showDefaultPorts");
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);

    sourceCurrentIndex=0;

    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);
        int iBytes=0;
        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            iBytes = socket->bytesAvailable();
            if(socket->state() != QAbstractSocket::ConnectedState)
                break;
            data += socket->readAll();
            usleep(50000);
        }
        Q_UNUSED(iBytes);

    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    //qDebug() << loadDoc;
    socket->close();

    QJsonArray fieldPortArray,fieldSourceArray;

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result , Alarm_Port, Service_Port;


    fieldSet = fieldDefJSONArray.at(3);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("status").toString();
    //qDebug() << result;

    fieldSet = fieldDefJSONArray.at(4);
    fieldObj = fieldSet.toObject();
    fieldSourceArray = fieldObj.value("Data_Sources").toArray();

    sourcesCount=fieldSourceArray.size();
    // qDebug() << "count:"<<sourcesCount;

    if(sourcesCount==0){
        QString message = QString("No data sources found..");
        QMessageBox::information(this,"Result", message );
        Disablefunc();
        // ui->m_PshBtn_UTC_Time->setEnabled(true);
        //ui->m_PshBtn_LocaleTime->setEnabled(true);

    }else if(sourcesCount>1){

        ui->lineEdit_AlarmPort->setEnabled(false);
        ui->m_PshBtn_PLCAlarmPortSave->setEnabled(false);
        ui->m_PshBtn_PLCAlarmPortArrival->setEnabled(false);
        ui->m_PshBtn_AlarmPort_Reset->setEnabled(false);

        ui->radioBtn_Alarm640->setEnabled(false);
        ui->radioBtn_Alarm1280->setEnabled(false);
        ui->m_PshBtn_Alarm->setEnabled(false);

        ui->m_PshBtn_DataPort_Reset->setEnabled(false);
    }

    ui->comboBox_databases->clear();

    Data_Sources.clear();

    for (int i=0;i<fieldSourceArray.size();i++) {
        Data_Sources.append(fieldSourceArray.at(i).toString());
        ui->comboBox_databases->addItem(fieldSourceArray.at(i).toString());
    }
    if(ui->comboBox_databases->currentText()!=""){
        QString source = ui->comboBox_databases->currentText();
        ui->comboBox_databases->setMinimumWidth(source.length()*15);
    }
    //qDebug() << Data_Sources;

    Data_Port.clear();
    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    fieldPortArray = fieldObj.value("Data_Port").toArray();

    // qDebug() <<"size="<<fieldPortArray.size();

    for(int j=0;j<fieldPortArray.size();j++){
        Data_Port.append(fieldPortArray.at(j).toString());
    }
    //qDebug() << "DataPort:"<<Data_Port;

    if(Data_Port.size()>0  && ui->comboBox_databases->currentIndex()!= -1) {
        ui->lineEdit_DataPort->setText(Data_Port.at(sourceCurrentIndex));
        oldPortData=Data_Port.at(sourceCurrentIndex);
    }

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Alarm_Port = fieldObj.value("Alarm_Port").toString();
    // qDebug() << "alramPort"<<Alarm_Port;

    if(Alarm_Port == "Alarm service is not running for multiple plc's"){

        QString message = QString("Alarm service is not running for multiple plc's..");
        // QMessageBox::information(this,"Result", message );

        ui->label_Alarm_message->setText(message);

        ui->lineEdit_AlarmPort->setEnabled(false);
        ui->m_PshBtn_PLCAlarmPortSave->setEnabled(false);
        ui->m_PshBtn_PLCAlarmPortArrival->setEnabled(false);
        ui->m_PshBtn_AlarmPort_Reset->setEnabled(false);

        ui->radioBtn_Alarm640->setEnabled(false);
        ui->radioBtn_Alarm1280->setEnabled(false);
        ui->m_PshBtn_Alarm->setEnabled(false);

    }else{

        ui->lineEdit_AlarmPort->setText(Alarm_Port);
        ui->label_Alarm_message->setText("");

    }
    fieldSet = fieldDefJSONArray.at(2);
    fieldObj = fieldSet.toObject();
    Service_Port = fieldObj.value("Service_Port").toString();
    ui->lineEdit_APIPort->setText(Service_Port);
    // qDebug() << "servicePort:"<<Service_Port;

    oldPortAlarm=ui->lineEdit_AlarmPort->text();
    oldPortDataloggerAPI=ui->lineEdit_APIPort->text();

    if(Data_Port.size() != Data_Sources.size()){
        QString message = QString("Sources count not matched with data ports count..");
        QMessageBox::information(this,"Result", message );
    }
}
/*void CommissionSetting::showFirstTimePassword() // not calling
{
    ui->lEdt_Passwd->clear();
    QString strIP;
    QString add;
    strIP = ui->comboBox_ConnectedIPs->currentText();
    QStringList strlst = strIP.split(':');
    int dSize = strlst.length();
    if(dSize > 0)
    {
        add = strlst.at(0);
    }
    ui->label_password->setText(add +" SU Credentials");
    QSettings setting("Finecho_SudoPaswd","Finecho_SudoPaswd_Details");
    setting.beginGroup(SUDOPASWD);
    ui->lEdt_Passwd->setText(setting.value(add).toString());
    setting.endGroup();
}    */

void CommissionSetting::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this,"Commissioning Tool",
                                                                tr("Do you want to Exit?\n"),
                                                                QMessageBox::Yes | QMessageBox::No);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        // event->accept();
        QApplication::quit();
    }
}

void CommissionSetting::on_m_PshBtn_DataPort_Reset_clicked()
{
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString password ;

    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/

    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","RestoreAndDefault_DATAPORT");
    jsonKeys.insert("oldport",ui->lineEdit_DataPort->text());
    // jsonKeys.insert("password",password);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);
    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    // qDebug() << jsonDoc;

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);

            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    }else {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }
    //qDebug() << loadDoc;
    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;
    QString serviceStatus;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("reason").toString();

    socket->close();

    if(result == "success"){
        ui->lineEdit_DataPort->setText("40001");
        Data_Port[sourceCurrentIndex]="40001";
        fieldSet=fieldDefJSONArray.at(2);
        fieldObj=fieldSet.toObject();
        serviceStatus=fieldObj.value("service").toString();
    }else if(Description == "Provided port is busy,Please provide different port no."){
        ui->lineEdit_DataPort->setText("40001");
    }

    QString message = QString("Reset port: " + result + "\n" + Description+"\n"+serviceStatus);
    QMessageBox::information(this,"Result", message );
}

void CommissionSetting::on_m_PshBtn_AlarmPort_Reset_clicked()
{
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString password;

    /*= decryptData(Str_IPAddress);
    QString res,Desc,msg;
    Desc = "Operation Failed";
    if(password == "")
    {
        msg = QString("Please set the sudo password" + res + "\n" + Desc);
        QMessageBox::information(this,"Erorr", msg );
        ui->lEdt_Passwd->clear();
        return;
    }*/



    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","RestoreAndDefault_ALARMPORT");
    // jsonKeys.insert("password",password);
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);
    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));

    socket->connectToHost(Str_IPAddress,Str_port.toInt());
    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            QApplication::setOverrideCursor(Qt::WaitCursor);

            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }

    }else {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        ui->lbl_Connection->setStyleSheet("background-color:red;");
        ui->pushButton_connect->setText("Connect");
        ui->lbl_error_msg->setText("Connection lost");
        m_pingThread->exit();
        timer->stop();
        Disablefunc();
        ui->comboBox_databases->clear();
        clearWidgets();
        return;
    }
    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;
    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;
    QString serviceStatus;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("reason").toString();

    socket->close();

    if(result == "success"){
        ui->lineEdit_AlarmPort->setText("40002");
        oldPortAlarm="40002";
        fieldSet=fieldDefJSONArray.at(2);
        fieldObj=fieldSet.toObject();
        serviceStatus=fieldObj.value("service").toString();
    }else if(Description == "Provided port is busy,Please provide different port no."){
        ui->lineEdit_AlarmPort->setText("40002");
    }

    QString message = QString("Reset port: " + result + "\n" + Description +"\n"+serviceStatus);
    QMessageBox::information(this,"Result", message );
}
QString CommissionSetting::add_padding(QString strLen)
{
    int num = strLen.size();
    if(num == 1){
        strLen.prepend("000000000");
    }else if(num == 2){
        strLen.prepend("00000000");
    }else if(num == 3){
        strLen.prepend("0000000");
    }else if(num == 4){
        strLen.prepend("000000");
    }else if(num == 5){
        strLen.prepend("00000");
    }else if(num == 6){
        strLen.prepend("0000");
    }else if(num == 7){
        strLen.prepend("000");
    }else if(num == 8){
        strLen.prepend("00");
    }else if(num == 9){
        strLen.prepend("0");
    }
    return strLen;
}

void CommissionSetting::Default_Alarm_Port()
{
    QJsonObject jsonKeys;
    QJsonDocument jsonDoc;
    QJsonArray jsonArray;
    QString password;
    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","Default_Alarm_Confugration");
    jsonArray.append(jsonKeys);
    jsonDoc = QJsonDocument(jsonArray);
    QByteArray data;
    QByteArray jByte(jsonDoc.toJson(QJsonDocument::Compact));
    int newNum = jByte.size();
    QString strLen = add_padding(QString::number(newNum));
    // qDebug() << jByte << newNum << strLen;
    socket->connectToHost(Str_IPAddress,Str_port.toInt());

    if(socket->waitForConnected(10000))
    {
        socket->write(strLen.toUtf8());
        socket->waitForBytesWritten(30000);

        socket->write(jByte);
        socket->waitForBytesWritten(30000);

        for(int m=0; m<100; m++)
        {
            socket->waitForReadyRead(1000);
            QApplication::setOverrideCursor(Qt::WaitCursor);
            if(socket->state() != QAbstractSocket::ConnectedState){
                QApplication::setOverrideCursor(Qt::ArrowCursor);
                break;
            }
            data += socket->readAll();
            usleep(50000);
        }
    }

    QJsonValue fieldSet;
    QJsonObject fieldObj;
    QJsonParseError json_error;

    QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));

    if (json_error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error: "<< json_error.errorString();
    }

    QJsonObject rootObj = loadDoc.object();
    QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();
    QString result;
    QString Description;

    fieldSet = fieldDefJSONArray.at(0);
    fieldObj = fieldSet.toObject();
    result = fieldObj.value("result").toString();

    fieldSet = fieldDefJSONArray.at(1);
    fieldObj = fieldSet.toObject();
    Description = fieldObj.value("Description").toString();

    socket->close();

    if(Description == "Alarm_Confugration_1280")
    {
        ui->radioBtn_Alarm1280->setChecked(true);
    }
    else if(Description == "Alarm_Confugration_640")
    {
        ui->radioBtn_Alarm640->setChecked(true);
    }
    else
    {
        ui->radioBtn_Alarm1280->setChecked(false);
        ui->radioBtn_Alarm640->setChecked(false);
        // ui->radioBtn_Alarm640->setDisabled(true);
        // ui->radioBtn_Alarm1280->setDisabled(true);
    }

/*  if(Description == "Alarm_Confugration_1280" || Description == "Alarm_Confugration_640")
    {
        ui->m_PshBtn_AlarmPort_Reset->setEnabled(true);
        ui->m_PshBtn_PLCAlarmPortSave->setEnabled(true);
        ui->m_PshBtn_Alarm->setEnabled(true);
    }
    else
    {
        ui->m_PshBtn_AlarmPort_Reset->setEnabled(false);
        ui->m_PshBtn_PLCAlarmPortSave->setEnabled(false);
        ui->m_PshBtn_Alarm->setEnabled(false);
    }                                                       */
}

void CommissionSetting::on_toolButton_Version_clicked()
{
    m_appVersions = new AppVersions(Str_IPAddress,Str_port);
    m_appVersions->show();
    m_appVersions->activateWindow();

    //===============disable the main window==============
    this->setEnabled(false);

    m_appVersions->setAttribute(Qt::WA_DeleteOnClose,true);

    connect(m_appVersions , &QWidget::destroyed, this,[=]()->void{
            this->setEnabled(true);});

}
