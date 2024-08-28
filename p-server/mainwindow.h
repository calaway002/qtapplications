#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QPushButton>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QDate>
#include <QTime>

// Constants for packet codes
const QString START_EXECUTION = "[0xE034]";
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

private slots:
    void onButtonClicked();
    void onConnection();
    void onReadyRead();
    void copyDirectoryToPendrive(const QString& localDirectoryPath, const QString& pendrivePath);
    void sendAcknowledgment(QTcpSocket *socket, const QString &message);
    void receivePacket(QTcpSocket *socket);
    QString calculateChecksum(const QString &data) const;

private:
    QString packetInfo(const QString& deviceId, const QString& code, const QString& serverUsername, const QString& commandLine) const;
    bool detectPendrive();
    QString getCurrentUsername();

    Ui::MainWindow *ui;
    QTcpServer *tcpServer;
    QTcpSocket *clientSocket;
    QString pendrivePath;
};

#endif // MAINWINDOW_H
