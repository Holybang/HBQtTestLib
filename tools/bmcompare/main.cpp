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
/*
   *** BMCompare ***

   The bmcompare tool provides quick comparison of benchmark results against a reference.

   Benchmark results are specified as XML files of the same format as those produced by QTestLib.

   A typical use case is to get a quick overview of the effect of a certain optimization:

       - First, run the benchmarks (using QTestLib) on the unoptimized code and dump results to
         XML files (there is typically one file per measurement back-end
         (wall time, callgrind events ...)).
         These files constitute the 'reference' against which other results will be compared.
       - Second, optimize the code one or more times (e.g. you may want to compare multiple
         optimization alternatives at once) each time running through the benchmarks and producing
         a set of XML files.
       - Third, run the bmcompare tool like this to compare the results:

             ./bmcompare -ref ref1.xml ref2.xml ... -cmp cmp1.xml cmp2.xml ... -cmp ...

   The -ref option specifies that the following file names belong to the reference results,
   while each -cmp option specifies that the following file names belong to the next set of
   comparable results.

   The output is dumped to stdout as a table like this:

       <function>        <tag>
           <metric 1>   <value> <value> ...
           <metric 2>   <value> <value> ...
           ...

       <function>        <tag>
           <metric 1>   <value> <value> ...
           <metric 2>   <value> <value> ...
           ...

    Vertically, there is one group per unique function/tag combination. The
    function is the name of the test function that contains the QBENCHMARK
    macro. The tag is the data tag that identifies a row of input data
    specified by the corresponding _data function (i.e. matching the argument
    to QTest::newRow()). The tag will be an empty string in the absence of a
    _data function. If several identical function/tag combinations are
    detected, only the first one is used.

    The rows in a group indicate results for each unique metric found for this
    function/tag combination. If several identical metrics are found for the
    combination, only the first one is used.

    Horizontally, there is (to the right of the metric name) one column per set of
    comparable results, i.e. one column per occurrence of '-cmp' in the command-line
    arguments.

    By default, the value is the absolute percentage of the reference (e.g. 95% means a 5%
    decrease for the metric in question). By passing -diff on the command line, the value is
    presented as a percentage difference instead (e.g. -5% and 5% mean a 5% decrease and increase
    respectively).

    The output is colored using ANSI escape codes iff the QTEST_COLORED environment variable
    is set.
 */

#include <QtCore>
#include <QtXml>

struct MetricResult {
    int value;
    int iterations;
    MetricResult(int value, int iterations) : value(value), iterations(iterations) {}
};

typedef QMap<QString, MetricResult *> MetricResults;

struct BenchmarkResult {
    QString function;
    QString tag;
    int outputPos;
    MetricResults metricResults;
    BenchmarkResult(const QString &function, const QString &tag, int outputPos = -1)
        : function(function), tag(tag), outputPos(outputPos) {}

    struct LessThan {
        bool operator()(const BenchmarkResult *a, const BenchmarkResult *b) {
            return a->outputPos < b->outputPos;
        }
    };
};

typedef QMap<QString, BenchmarkResult *> BenchmarkResults;

static void mergeBenchmarkResults(const QString &xmlFile, BenchmarkResults *bmResults)
{
    QFile f(xmlFile);
    f.open(QIODevice::ReadOnly);
    QByteArray xml = f.readAll();

    QDomDocument doc;

    int line;
    int col;
    QString errorMsg;
    if (doc.setContent(xml, &errorMsg, &line, &col) == false) {
        qDebug() << "dom setContent failed" << line << col << errorMsg;
    }

    int outputPos = 0;

    QDomNodeList testFunctions = doc.elementsByTagName("TestFunction");
    for (int i = 0; i < testFunctions.count(); ++i) {
        QDomElement function = testFunctions.at(i).toElement();
        QString functionName = function.attributeNode("name").value();

        QDomNodeList results = function.elementsByTagName("BenchmarkResult");
        for (int j = 0; j < results.count(); ++j) {    
            QDomElement result = results.at(j).toElement();
            QString tag = result.attributeNode("tag").value();
            QString metric = result.attributeNode("metric").value();
            
            QString valueString = result.attributeNode("value").value();
            QString iterationsString = result.attributeNode("iterations").value();
            const int value = valueString.toInt();
            const int iterations = iterationsString.toInt();

            const QString funcTag = functionName + tag;
            BenchmarkResult *bmResult = bmResults->value(funcTag);
            if (!bmResult) {
                bmResult = new BenchmarkResult(functionName, tag, outputPos++);
                bmResults->insert(funcTag, bmResult);
            }

            MetricResult *metricResult = bmResult->metricResults.value(metric);
            if (!metricResult) {
                metricResult = new MetricResult(value, iterations);
                bmResult->metricResults.insert(metric, metricResult);
            }
        }
    }
}

typedef QList<BenchmarkResults *> BenchmarkResultsList;

enum ValueMode { Identical, Better, Worse, NotFound };

static QString valueString(qreal value, ValueMode valueMode)
{
    const int intWidth = 7;
    const int fracWidth = 3;
    const int cmpPercentageWidth = intWidth + 1 + fracWidth;
    const int symbolWidth = 2;
    QString result;
    QTextStream out(&result);
    out.setRealNumberPrecision(fracWidth);
    out.setRealNumberNotation(QTextStream::FixedNotation);

    if (valueMode == NotFound) {
        out.setFieldWidth(cmpPercentageWidth + symbolWidth);
        out << "(not found)";
    } else {
        out.setFieldWidth(cmpPercentageWidth);
        out << value;
    }

    static bool colored = (!qgetenv("QTEST_COLORED").isEmpty());
    if (colored) {
        enum { None = 0, Red = 31, Green = 32, Yellow = 33 };
        int color = None;
        if (valueMode == Identical)
            color = Yellow;
        else if (valueMode == Better)
            color = Green;
        else if (valueMode == Worse)
            color = Red;
        if (color != None) {
            const int prefix = 1;
            result =
                QString("\033[%1;%2m%3\033[0m").
                arg(prefix).arg(color).arg(result);
        }
    }

    if (valueMode != NotFound) {
        QString symbol;
        QTextStream out2(&symbol);
        out2.setFieldWidth(symbolWidth);
        out2 << "%";
        result += symbol;
    }

    return result;
}

static void printBenchmarkResults(
    const QStringList &refFiles, const QList<QStringList> &cmpFilesList, bool diffMode)
{
    BenchmarkResults *refResults = new BenchmarkResults;
    BenchmarkResultsList cmpResultsList;

    // Merge ...

    foreach (QString refFile, refFiles) {
        mergeBenchmarkResults(refFile, refResults);
    }
    foreach (QStringList cmpFiles, cmpFilesList) {
        BenchmarkResults *cmpResults = new BenchmarkResults;
        foreach (QString cmpFile, cmpFiles) {
            mergeBenchmarkResults(cmpFile, cmpResults);
        }
        cmpResultsList << cmpResults;
    }

    // Print ...

    QList<BenchmarkResult *> sortedRefResults(refResults->values());
    qSort(sortedRefResults.begin(), sortedRefResults.end(), BenchmarkResult::LessThan());

    QTextStream out(stdout);

    foreach (BenchmarkResult *refResult, sortedRefResults) {
        out << "\n";
        out.setFieldWidth(20);
        out.setFieldAlignment(QTextStream::AlignLeft);
        out << (refResult->function + "()");
        out.setFieldWidth(0);
        out << refResult->tag << "\n";
        QStringList metrics = refResult->metricResults.keys();
        metrics.sort();
        const QString funcTag = refResult->function + refResult->tag;

        QList<BenchmarkResult *> cmpResults;
        foreach (BenchmarkResults *cmpResults_, cmpResultsList) {
            cmpResults << cmpResults_->value(funcTag);
        }

        foreach (QString metric, metrics) {
            MetricResult *metricResult = refResult->metricResults.value(metric);
            Q_ASSERT(metricResult->iterations > 0);
            const qreal refValue = metricResult->value / qreal(metricResult->iterations);
            out << "    ";
            out.setFieldWidth(9);
            out.setFieldAlignment(QTextStream::AlignLeft);
            out << metric;

            out.setFieldAlignment(QTextStream::AlignRight);
            foreach (BenchmarkResult *cmpResult, cmpResults) {
                bool found = false;
                if (cmpResult) {

                    MetricResult *cmpMetricResult = cmpResult->metricResults.value(metric);
                    if (cmpMetricResult) {
                        const qreal cmpValue =
                            cmpMetricResult->value / qreal(cmpMetricResult->iterations);
                        qreal cmpPercentage = (cmpValue / refValue) * 100;
                        ValueMode valueMode = Identical;
                        if (cmpPercentage < 100)
                            valueMode = Better;
                        else if (cmpPercentage > 100)
                            valueMode = Worse;
                        if (diffMode)
                            cmpPercentage = cmpPercentage - 100;
                        out.setFieldWidth(0);
                        out << valueString(cmpPercentage, valueMode);
                        found = true;
                    }
                }
                if (!found) {
                    out.setFieldWidth(0);
                    out << valueString(-1, NotFound);;
                }
            }
            out.setFieldWidth(0);
            out << "\n";
        }
    }
}

static void parseArguments(QStringList &refFiles, QList<QStringList> &cmpFilesList, bool &diffMode)
{
    diffMode = false;
    QStringList args = qApp->arguments();
    args.removeFirst();
    QStringList cmpFiles;
    enum { AddRefFile, AddCmpFile, None } state = None;
    foreach (QString arg, args) {
        if (arg == "-ref") {
            if (state == AddCmpFile && !cmpFiles.isEmpty()) {
                cmpFilesList << cmpFiles;
                cmpFiles.clear();
            }
            state = AddRefFile;
        } else if (arg == "-cmp") {
            if (state == AddCmpFile && !cmpFiles.isEmpty()) {
                cmpFilesList << cmpFiles;
                cmpFiles.clear();
            }
            state = AddCmpFile;
        } else if (arg == "-diff") {
            diffMode = true;
        } else {
            if (state == AddRefFile) {
                refFiles << arg;
            } else if (state == AddCmpFile) {
                cmpFiles << arg;
            }
        }
    }
    if (!cmpFiles.isEmpty())
        cmpFilesList << cmpFiles;
}

static void printUsage()
{
    qDebug() << "usage:" << qApp->arguments().first().toStdString().data() <<
        "[-diff] "
        "-ref <ref file 1> [<ref file 2> ...] "
        "-cmp <cmp file 1.1> [<cmp file 1.2> ...] "
        "[-cmp <cmp file 2.1> [<cmp file 2.2> ...] ...]";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QStringList refFiles;
    QList<QStringList> cmpFilesList;
    bool diffMode;

    parseArguments(refFiles, cmpFilesList, diffMode);
    if (refFiles.isEmpty() || cmpFilesList.isEmpty()) {
        printUsage();
        return 1;
    }

    printBenchmarkResults(refFiles, cmpFilesList, diffMode);
    
    return 0;
}
