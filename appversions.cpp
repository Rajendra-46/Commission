#include "appversions.h"
#include "ui_appversions.h"

AppVersions::AppVersions(const QString &IPAddress, const QString &Port,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AppVersions)
{
    ui->setupUi(this);
    this->setFixedSize(this->width(),this->height());
    this->setWindowTitle("App Versions");
    Str_IPAddress = IPAddress;
    Str_port = Port;
    socket = new QTcpSocket(this);
    get_versions();
}

AppVersions::~AppVersions()
{
    delete ui;
}
void AppVersions::get_versions()
{
    QJsonObject jsonKeys;
    QJsonArray jsonArray;
    QJsonParseError json_error;
    QByteArray data;
    QJsonDocument jsonDoc;

    jsonKeys.insert("username","rushikesh");
    jsonKeys.insert("passcode","finecho@178");
    jsonKeys.insert("authkey","abcd");
    jsonKeys.insert("endpoint","get_versions");
    jsonArray.append(jsonKeys);

    jsonDoc = QJsonDocument(jsonArray);
    //qDebug() << "jsonDoc===" << jsonDoc;
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
        for(int m=0; m<10; m++){
            socket->waitForReadyRead(20000);
            iBytes = socket->bytesAvailable();
            if(iBytes==0)
                break;

            data += socket->readAll();
            usleep(1000);
        }

        QJsonDocument loadDoc(QJsonDocument::fromJson(data, &json_error));
        if (json_error.error != QJsonParseError::NoError)
        {
            qDebug() << "JSON parse error: "<< json_error.errorString();
        }

        QJsonObject rootObj = loadDoc.object();
        QJsonArray fieldDefJSONArray = rootObj.value("field").toArray();

        QJsonValue fieldSet;
        QJsonObject fieldObj;
        QStringList strLstKeys;

        for(int i=0; i< fieldDefJSONArray.size(); i++)
        {
            fieldSet = fieldDefJSONArray.at(i);
            fieldObj = fieldSet.toObject();
            strLstKeys = fieldObj.keys();
        }
        foreach(const QString& key, fieldObj.keys())
        {
            QJsonValue value = fieldObj.value(key);
            QString str_Vnum =  value.toArray().at(0).toString();
            QString str_Date =  value.toArray().at(1).toString();
            QString str_Rnote = value.toArray().at(2).toString();
            if(key == "plcdatacollector")
            {
                ui->pc_Vnum->setText(str_Vnum);
                ui->pc_Date->setText(str_Date);
                ui->pc_Rnote->setText(str_Rnote);
            }else if (key == "alarmdatacollector") {
                ui->adc_Vnum->setText(str_Vnum);
                ui->adc_Date->setText(str_Date);
                ui->adc_Rnote->setText(str_Rnote);
            }else if (key == "dataloggerapi") {
                ui->s_Vnum->setText(str_Vnum);
                ui->s_Date->setText(str_Date);
                ui->s_Rnote->setText(str_Rnote);
            }else if (key == "kpiloggerapi") {
                ui->kpi_Vnum->setText(str_Vnum);
                ui->kpi_Date->setText(str_Date);
                ui->kpi_Rnote->setText(str_Rnote);
            }else if (key == "kpidatacollectioncron") {
                ui->kpicron_Vnum->setText(str_Vnum);
                ui->kpicron_Date->setText(str_Date);
                ui->kpicron_Rnote->setText(str_Rnote);
            }else{
                ui->cs_Vnum->setText(str_Vnum);
                ui->cs_Date->setText(str_Date);
                ui->cs_Rnote->setText(str_Rnote);
            }
        }
        socket->close();
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("Error: " + socket->errorString() + "\nPlease Check connectivity to server");
        msgBox.exec();
        return;
    }
}
QString AppVersions::add_padding(QString strLen)
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
