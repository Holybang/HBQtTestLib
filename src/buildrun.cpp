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
#include "buildrun.h"
#include "benchmark.h"


int runExecutable(const QString &workdir, const QString &executable, const QStringList &arguments = QStringList())
{
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    if (workdir != QString())
        p.setWorkingDirectory(workdir);
    
    if (arguments == QStringList())
        p.start(executable);
    else
        p.start(executable, arguments);

    if (p.waitForFinished(-1) == false) {
        qDebug() << "run" << executable << "failed" << p.error();
        return -1;
    }
  //  qDebug() << p.exitCode();
    return p.exitCode();
}

RunStatus runExecutableEx(const QString &workdir, const QString &executable, const QStringList &arguments = QStringList())
{
    QProcess p;
    RunStatus status;
//    qDebug() << workdir << executable << arguments;
    p.setProcessChannelMode(QProcess::MergedChannels);
//    QDir previousWoringDirectory = QDir::current();

    if (workdir != QString())
     //   QDir::setCurrent(workdir);
        p.setWorkingDirectory(workdir);
    
    if (arguments == QStringList())
        p.start(executable);
    else
        p.start(executable, arguments);

    if (p.waitForFinished(-1) == false) {
        status.error = RunStatus::NotFoundError;
        status.output = "Running " + executable.toAscii() + " failed with error string: " + p.errorString().toAscii();
//        QDir::setCurrent(previousWoringDirectory.path());
        return status;
    }
    status.output = p.readAll();
    int code = p.exitCode();
    if (code != 0) {
//        qDebug() << "fail",    
        status.error = RunStatus::RuntimeError;
    }

//    QDir::setCurrent(previousWoringDirectory.path());
    return status;
}



QByteArray pipeExecutable(const QString &workdir, const QString &executable, const QStringList &arguments = QStringList())
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    if (workdir != QString())
        p.setWorkingDirectory(workdir);
    p.start(executable, arguments);
    if (p.waitForFinished(-1) == false) {
        qDebug() << "run" << executable << "failed" << p.error();
    }
    return p.readAll();
}

// returns false on timeout
bool forwardExecutable(const QString &workdir, const QString &executable, int timeout, const QStringList &arguments, const QString &pathAddition)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);

    if (pathAddition != QString()) {
        QStringList env = QProcess::systemEnvironment();
        env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=" + pathAddition + ";\\1");
        p.setEnvironment(env);
    }
    
    if (workdir != QString())
        p.setWorkingDirectory(workdir);
    p.start(executable, arguments);

    if (p.waitForStarted(500) == false) {
        qDebug() << QString("run " + executable + " failed to start");
        return true;
    }

    if (p.waitForFinished(timeout) == false) {
        if (p.error() == QProcess::Timedout) {
            p.kill();
            p.waitForFinished(-1);
            return false;
        }
    }

    return true;
}

bool runBenchmark(const QString &qtDir, const QString &workdir, const QString &executable, int timeout, const QStringList &arguments)
{
    QString qtDirCopy = qtDir;
    if (qtDirCopy.isEmpty() == false)
        qtDirCopy += "/bin";
    return forwardExecutable(workdir, executable, timeout, arguments, qtDirCopy);
}

QHash<QString, QString> systemEnvitonment()
{
    QHash<QString, QString> keyValues;
    foreach(QString line, QProcess().systemEnvironment()) {
        QStringList parts = line.split("=");
        keyValues.insert(parts.at(0), parts.at(1));
    }
    return keyValues;
}


RunStatus qmake(const QString &path, const QString &qmakeBinaryPath, const QString &target)
{
    QStringList arguments;
    if (target != QString()) {
        arguments += "-after";
        arguments += "TARGET=" + target;
    }

    return runExecutableEx(path, qmakeBinaryPath, arguments);
}

RunStatus runMake(const QString &path, const QString &argument)
{
//    qDebug() << "make in" << path << "args" << argument;
    QStringList arguments;
    if (argument != QString())
        arguments.append(argument);

#ifdef Q_WS_WIN
    // ### do this the right way :)

    QStringList possibleMakePaths = QStringList() 
        << "C:/Programfiler/Microsoft Visual Studio 8/VC/bin/nmake.exe"
        << "C:/Program Files/Microsoft Visual Studio 8/VC/bin/nmake.exe"
    ;
    
    foreach (QString nmake, possibleMakePaths) {
        if (QFile::exists(nmake))
            return runExecutableEx(path, nmake, QStringList() << argument);
    }
    RunStatus status;
    status.error = RunStatus::NotFoundError;
    status.output = "Cold not find qmake";
    return status;
 #else
    return (runExecutableEx(path, "/usr/bin/make", arguments));
#endif
}

RunStatus buildTest(const QString &path, const QString &qmakePath, const QString &target)
{
//    qDebug() << "building test in path" << path  << "with qmake" << qmakePath;

    RunStatus status = qmake(path, qmakePath, target);
   if (status.ok() == false)
        return status;

    runMake(path, "clean");

    return runMake(path);
}

QByteArray runTest(const QString &path, const QString &executable, const QString &arg)
{
    QStringList args;
    if (arg != QString())
        args.append(arg);

    return runTest(path, executable, args);
}

QByteArray runTest(const QString &path, const QString &executable, const QStringList &args)
{
    qDebug() << "running test" << path << executable;
    return pipeExecutable(path, path + "/" + executable, args);
}

QByteArray pipeP4sync(const QString &path)
{
    return pipeExecutable(path, "p4", QStringList() << QString("sync") << QString("..."));
}

void p4sync(const QString &path)
{
    pipeExecutable(path, "p4", QStringList() << QString("sync") << QString("..."));
}

RunStatus buildBenchmark(const QString &benchmarkPath, const QString &qtPath)
{
    Benchmark b;
    return b.buildBenchmark(qtPath, benchmarkPath, QString());
}

void runBenchmark(const QString &benchmarkPath, const QStringList &options)
{
    QString partialName = QDir(benchmarkPath).dirName();
    QString testExecutable = findExecutable(benchmarkPath, partialName);
    QString testExecutablePath = benchmarkPath + "/" + testExecutable;
    runBenchmark(QString(), benchmarkPath, testExecutablePath, -1 , options);
}

