#ifndef APPVERSIONS_H
#define APPVERSIONS_H

#include <QWidget>
#include <QTcpSocket>
#include <QJsonParseError>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <unistd.h>
#include <QDebug>

namespace Ui {
class AppVersions;
}

class AppVersions : public QWidget
{
    Q_OBJECT

public:
    explicit AppVersions(const QString & IPAddress = QString(),const QString & Port = QString(),QWidget *parent = nullptr);
    ~AppVersions();
    QString Str_IPAddress,Str_port;
    QTcpSocket *socket;
    void get_versions();
    QString add_padding(QString);

private:
    Ui::AppVersions *ui;
};

#endif // APPVERSIONS_H
