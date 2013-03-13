#ifndef BUILDRUN_H
#define BUILDRUN_H

#include <QtCore>
class RunStatus
{
public:
    RunStatus()
    : exitCode(0), error(NoError) { }
    enum Error { NoError, NotFoundError, RuntimeError };
    int exitCode;
    Error error;
    QByteArray output;
    bool ok() { return error == NoError; }
};


void runtests(const QString &basePath);
void syncTest(const QString &path);
RunStatus runMake(const QString &path, const QString &argument = QString());
RunStatus buildTest(const QString &path, const QString &qmakePath, const QString &target = QString());
QByteArray runTest(const QString &path, const QString &executable, const QString &arg = QString());
QByteArray runTest(const QString &path, const QString &executable, const QStringList &args);
RunStatus qmake(const QString &path, const QString &qmakeBinaryPath, const QString &target = QString());
QHash<QString, QString> systemEnvitonment();

bool forwardExecutable(const QString &workdir, const QString &executable, int timeout, const QStringList &arguments = QStringList(), const QString &pathAddition = QString());
bool runBenchmark(const QString &qtPath, const QString &workdir, const QString &executable, int timeout = -1, const QStringList &arguments = QStringList());
QByteArray pipeP4sync(const QString &path);
bool p4sync(const QString &path, int change);
void p4sync(const QString &path);

RunStatus buildBenchmark(const QString &benchmarkPath, const QString &qtPath);
void runBenchmark(const QString &benchmarkPath, const QStringList &options);

#endif
