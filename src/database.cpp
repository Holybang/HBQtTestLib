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
#include "database.h"
#include <QtGui>
#include <QtXml>
#include <QtWidgets/QTableView>

// Database schema definition and open/create functions

QString resultsTable = QString("(TestName varchar, TestCaseName varchar, Series varchar, Idx varchar, ") + 
                       QString("Result varchar, ChartType varchar, Title varchar, ChartWidth varchar, ") + 
                       QString("ChartHeight varchar, TestTitle varchar, QtVersion varchar, Iterations varchar") +
                       QString(")");

void execQuery(QSqlQuery query, bool warnOnFail)
{
    bool ok = query.exec();
    if (!ok && warnOnFail) {
        qDebug() << "FAIL:" << query.lastQuery() << query.lastError().text();
    }
}

void execQuery(const QString &spec, bool warnOnFail)
{
    QSqlQuery query;
    query.prepare(spec);
    execQuery(query, warnOnFail);
}

QSqlDatabase openDataBase(const QString &databaseFile)
{
//    qDebug() << "open data base";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(databaseFile);
    bool ok = db.open(); 
    if (!ok)
        qDebug() << "FAIL: could not open database";
    return db;
}

QSqlDatabase createDataBase(const QString &databaseFile)
{
//    qDebug() << "create data base";
    QSqlDatabase db = openDataBase(databaseFile);

    execQuery("DROP TABLE Results", false);
    execQuery("CREATE TABLE Results " + resultsTable);

    return db;
}

struct Tag
{
    Tag(QString key, QString value)
    : key(key.trimmed()), value(value.trimmed())
    {
    
    }

    QString key;
    QString value;
};

QList<Tag> parseTag(const QString &tag)
{
    // Format: key1=value ; key2=value
    //         key1=value key2=value
    //         value--value
    
    QList<Tag> keyValues;

    QString keyValuePairSeparator("");
    if (tag.contains(";"))
        keyValuePairSeparator = ';';
    if (tag.contains("--"))
        keyValuePairSeparator = "--";

    foreach (QString keyValue, tag.split(keyValuePairSeparator)) {
        if (keyValue.contains("=")) {
            QStringList parts = keyValue.split("=");
            keyValues.append(Tag(parts.at(0), parts.at(1)));
        } else {
            keyValues.append(Tag(QString(), keyValue)); // no key, just a value.
        }
    }

    return keyValues;
}

void loadXml(const QStringList &fileNames)
{
    foreach(const QString &fileName, fileNames) {
        QFileInfo fi( fileName );
        loadXml(fileName, fi.fileName());
    }
}

void loadXml(const QString &fileName, const QString &context)
{
    QFile f(fileName);
    f.open(QIODevice::ReadOnly);
    loadXml(f.readAll(), context);
}

void loadXml(const QByteArray &xml, const QString& context)
{
    QDomDocument doc;

    int line;
    int col;
    QString errorMsg;
    if (doc.setContent(xml, &errorMsg, &line, &col) == false) {
        qDebug() << "dom setContent failed" << line << col << errorMsg;
    }
    
    // Grab "Value" from <Environment><QtVersion>Value</QtVersion></Environment>
    QString qtVersion = doc.elementsByTagName("Environment").at(0).toElement().elementsByTagName("QtVersion")
                        .at(0).toElement().childNodes().at(0).nodeValue();
    QString testCase = doc.elementsByTagName("TestCase").at(0).toElement().attributeNode("name").value();
        
//    qDebug() << "qt version" << qtVersion;
//    qDebug() << "test case" << testCase;

    DataBaseWriter writer;
    writer.testName = testCase; // testCaseName and testName is mixed up in the database writer class
    writer.qtVersion = qtVersion;
    
    QDomNodeList testFunctions = doc.elementsByTagName("TestFunction");
    for (int i = 0; i < testFunctions.count(); ++i) {
        QDomElement function = testFunctions.at(i).toElement();
        QString functionName = function.attributeNode("name").value();
        writer.testCaseName = functionName; // testCaseName and testName is mixed up in the database writer class
        
//        qDebug() << "fn" << functionName;

        QDomNodeList results = function.elementsByTagName("BenchmarkResult");
        for (int j = 0; j < results.count(); ++j) {    
            QDomElement result = results.at(j).toElement();
            QString tag = result.attributeNode("tag").value();

//            if (!context.isEmpty())
//                tag += QString(" (%1)").arg(context);

            QString series;
            QString index;

            // By convention, "--" separates series and indexes in tags.
            if (tag.contains("--")) {
                QStringList parts = tag.split("--");
                series = parts.at(0);
                index = parts.at(1);
            } else {
                series = tag;
            }

            QString resultString = result.attributeNode("value").value();
            QString iterationCount = result.attributeNode("iterations").value();
            double resultNumber = resultString.toDouble() / iterationCount.toDouble();
            writer.addResult(series, index, QString::number(resultNumber), iterationCount);
//            qDebug() << "result" << series  << index << tag << resultString << iterationCount;
        }
    }
}

void displayTable(const QString &table)
{
    QSqlTableModel *model = new QSqlTableModel();
    model->setTable(table);
    model->select();
    QTableView *view = new QTableView();
    view->setModel(model);
    view->show();
}

void printDataBase()
{
   QSqlQuery query;
   query.prepare("SELECT TestName, TestCaseName, Result FROM Results;");
   bool ok  = query.exec(); 
   qDebug() <<  "printDataBase ok?" <<  ok;

   query.next();
   qDebug() << "";
   qDebug() << "Benchmark" << query.value(0).toString();
   query.previous();

   while (query.next()) {
       //  QString country = query.value(fieldNo).toString();
       //  doSomething(country);
       qDebug() << "result for" << query.value(1).toString() << query.value(2).toString();
   } 
}

// TempTable implementation

static int tempTableIdentifier = 0;
TempTable::TempTable(const QString &spec)
{
    m_name = "TempTable" + QString::number(tempTableIdentifier++);
    execQuery("CREATE TEMP TABLE " + m_name + " " + spec);
}

TempTable::~TempTable()
{
    // ref count and drop it?
}

QString TempTable::name()
{
    return m_name;
}

// DataBaseWriter implementation

DataBaseWriter::DataBaseWriter()
{
    disable = false;
    chartSize = QSize(800, 400);
    databaseFileName = ":memory:";
    qtVersion = QT_VERSION_STR;
}

void DataBaseWriter::openDatabase()
{
    db = openDataBase(databaseFileName);
}

void DataBaseWriter::createDatabase()
{
    db = createDataBase(databaseFileName);
}

void DataBaseWriter::beginTransaction()
{
    if (db.transaction() == false) {
        qDebug() << db.lastError();
        qFatal("no transaction support");
    }
}

void DataBaseWriter::commitTransaction()
{
    db.commit();
}

void DataBaseWriter::rollbackTransaction()
{
    db.rollback();
}

void DataBaseWriter::addResult(const QString &result)
{
	return addResult(QString(), QString(), result);
}

void DataBaseWriter::addResult(const QString &series, const QString &index, const QString &result, const QString &iterations)
{
    if (disable)
        return;

     QSqlQuery query;

     query.prepare("INSERT INTO Results (TestName, TestCaseName, Series, Idx, Result, ChartWidth, ChartHeight, Title, TestTitle, ChartType, QtVersion, Iterations) "
                    "VALUES (:TestName, :TestCaseName, :Series, :Idx, :Result, :ChartWidth, :ChartHeight, :Title, :TestTitle, :ChartType, :QtVersion, :Iterations)");
     query.bindValue(":TestName", testName);
     query.bindValue(":TestCaseName", testCaseName);
     query.bindValue(":Series", series);
     query.bindValue(":Idx", index);
     query.bindValue(":Result", result);
     query.bindValue(":ChartWidth", chartSize.width());
     query.bindValue(":ChartHeight", chartSize.height());
     query.bindValue(":Title", chartTitle);
     query.bindValue(":TestTitle", testTitle);
     query.bindValue(":QtVersion", qtVersion);
     query.bindValue(":Iterations", iterations);


    if (chartType == LineChart)
        query.bindValue(":ChartType", "LineChart");
    else
        query.bindValue(":ChartType", "BarChart");
    execQuery(query);
}
