/*
FileName	:commissionsetting.h
Authour     :Gururaj B M / Kasturi Rangan.
*/
#ifndef COMMISSIONSETTING_H
#define COMMISSIONSETTING_H

#include <QWidget>
#include <QTcpSocket>
#include <QJsonParseError>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <unistd.h>
#include <QStandardItemModel>
#include <QDebug>
#include <QFileDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QtGui>
#include <QTextStream>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QtConcurrent/QtConcurrent>
#include <QTimer>
#include <QSettings>
#include <QHostAddress>
#include <QCloseEvent>
#include <QDateTime>
#include <QValidator>
#include <appversions.h>

QT_BEGIN_NAMESPACE
namespace Ui { class CommissionSetting; }
QT_END_NAMESPACE

class CommissionSetting : public QWidget
{
    Q_OBJECT

public:
    CommissionSetting(QWidget *parent = nullptr);
    ~CommissionSetting();
    QString Awl_filePath,ExlfilePath;
    QTcpSocket *socket,*socket1,*socket_utc;
    QThread *m_pingThread;
    QTimer *m_pingTimer;
    QTimer *timer;
    QString Str_IPAddress,Str_port;
    void Connection_status();
    void ReloadDefaultIps();
    void ReloadLastSelectedIPs();
    void ConnectToIp_Port(QString,QString);
    void clearWidgets();
    void clearText();
    void ChangeTimeZone();
    void Default_Alarm_Port();
    QString encryptData(QString);
    QString decryptData(QString);
    void Enablefunc();
    void Disablefunc();
    void showDefaultPorts();
    int iSecs = 59;
    int ilocSecs = 59;
    bool bCalledApi = false;
    bool blocCalledApi = false;
    void showFirstTimePassword();
    void closeEvent(QCloseEvent *event);
    QString add_padding(QString);
    AppVersions *m_appVersions;

    QString currentPort;
    QString oldPortData;
    QString oldPortAlarm;
    QString oldPortDataloggerAPI;

    QVector<QString> Data_Port;
    QVector<QString> Data_Sources;

    int sourcesCount;
    int sourceCurrentIndex;

private slots:
    void on_pushButton_connect_clicked();
    void CheckConnection();
    void updateTime();
    void UpdateTimezoneUTC();
    void on_comboBox_ConnectedIPs_currentTextChanged(const QString &arg1);
    void on_m_PshBtn_UTC_Time_clicked();
    void on_m_PshBtn_LocaleTime_clicked();
    void on_toolButton_AWLFile_clicked();
    void on_m_PshBtn_AWLFileUpload_clicked();
    void on_m_PshBtn_PLCDataPortSave_clicked();
    void on_m_PshBtn_PLCAlarmPortSave_clicked();
    void on_m_PshBtn_API_Set_clicked();
    void on_m_PshBtn_API_Check_clicked();
    void on_m_PshBtn_API_Reset_clicked();
    //void on_pbtn_Passwd_Save_clicked();
    void on_timeEdit_Time_timeChanged(const QTime &time);

    void on_m_PshBtn_Alarm_clicked();

    void on_m_PshBtn_PLCDataPortArrival_clicked();

    void on_m_PshBtn_PLCAlarmPortArrival_clicked();

    void on_comboBox_ConnectedIPs_currentIndexChanged(const QString &arg1);

    void on_m_PshBtn_DataPort_Reset_clicked();

    void on_m_PshBtn_AlarmPort_Reset_clicked();

    void on_toolButton_Version_clicked();

    void onComboTextChanged(QString);


private:
    Ui::CommissionSetting *ui;
  //  QComboBox *comboBoxDataBases;

};
#endif // COMMISSIONSETTING_H
