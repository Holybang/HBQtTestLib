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
#include <QtCore>
#include <QtSql>
#include <database.h>
#include <reportgenerator.h>
 
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        qDebug() << "Usage: generatereport xml-file [xml-file2 xml-file3 ...]";
        return 0;
    }

    QStringList files;
    for (int i = 1; i < argc; i++) {
        QString file = QString::fromLocal8Bit(argv[i]);
        files += file;
        qDebug() << "Reading xml from" << file;
    }

    QSqlDatabase db = createDataBase(":memory:");

    loadXml(files);

    ReportGenerator reportGenerator;
    reportGenerator.writeReports();
    db.close();
}

