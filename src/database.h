/****************************************************************************
**
** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the QTestLib project on Trolltech Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore>
#include <QtSql>

extern QString resultsTable;
QSqlDatabase openDataBase(const QString &databaseFile = "database");
QSqlDatabase createDataBase(const QString &databaseFile = "database");

void loadXml(const QStringList &fileNames);
void loadXml(const QString &fileName, const QString &context=QString::null);
void loadXml(const QByteArray &xml, const QString &context=QString::null);

void execQuery(QSqlQuery query, bool warnOnFail = true);
void execQuery(const QString &spec, bool warnOnFail = true);
void displayDataBase(const QString &table = QString("Results"));
void printDataBase();
void displayTable(const QString &table);

class TempTable
{
public:
    TempTable(const QString &spec);
    ~TempTable();
    QString name();
private:
    QString m_name;
};

enum ChartType { BarChart, LineChart };
class DataBaseWriter
{
public:
    DataBaseWriter();
    QString databaseFileName;
    QString testTitle;
    QString testName;
    QString testCaseName;
    ChartType chartType;
    QSize chartSize;
    QString chartTitle;
    QString qtVersion;
    bool disable;
    
    void openDatabase();
    void createDatabase();

    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();

    void addResult(const QString &result);
    void addResult(const QString &series , const QString &index, const QString &result, const QString &iterations = QLatin1String("1"));

    QSqlDatabase db;
};


#endif
