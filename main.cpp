/*
FileName	:main.cpp
Authour     :Gururaj B M / Kasturi Rangan.
*/
#include "commissionsetting.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CommissionSetting w;
    w.show();
    return a.exec();
}
