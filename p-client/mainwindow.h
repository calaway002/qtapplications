#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QDebug>
#include <QStringList>
#include <QDir>
#include <QFileInfoList>
#include <QMainWindow>

const QString START_EXECUTION_CODE = "0xE034";
const QString SUCCESSFULLY_COPIED_FILES = "0xF00D";
const QString FAILED_TO_COPY = "0xF00E";

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void parsePacket(const QString& packet);


private slots:
        void readServerData();
        void GetServerUsername(const QString &serverUsername);
        void start_execution();
        QString getCurrentUsername();
        void sendLogsToServer();
        void sendPacket(QTcpSocket *socket, const QString &message);
        QString calculateChecksum(const QString &data) const;

private:
    Ui::MainWindow *ui;
    QTcpSocket *tcpSocket;
    QString serverAddress;
    quint16 serverPort;

    QString serverUsername;
    bool usernameReceived;
    bool copySuccessful;
    QProcess findProcess;
    QProcess scpProcess;

};

#endif // MAINWINDOW_H
