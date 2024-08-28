#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCryptographicHash>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , tcpServer(new QTcpServer(this))
    , clientSocket(nullptr)
{
    ui->setupUi(this);

    // Connect the button click signal to the slot
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onButtonClicked);

    // Set up the TCP server
    QHostAddress Address("192.168.5.1");

    int port = 1234;
    if (!tcpServer->listen(Address, port)) {
        qDebug() << "Server could not start:" << tcpServer->errorString();
        return;
    }
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onConnection);
    qDebug() << "Server started and listening on port 1234";
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getCurrentUsername() {   //Gets the current system's username.
    QProcess process;
    process.start("logname");  //"Execute 'logname' to get current username"
    process.waitForFinished();
    QByteArray output = process.readAllStandardOutput();  //Reads process's standard output into QByteArray and stored in output.

    QString username = QString(output.trimmed()); // Remove any extra whitespace/newlines

    return username; //"Return current username"
}

bool MainWindow::detectPendrive() {               //detectPendrive returns true if a pendrive is found, otherwise false.
    QStringList possibleMountPoints = {"/media"}; // Defines potential mount points for removable drives.

    QString username = getCurrentUsername();       //Gets the current system's username.
    if (username.isEmpty()) {                      //Checks if the username is empty.
        qDebug() << "Failed to get current username."; //Logs an error message if no username is found.
        return false;                                  //Stops the function if the username is missing.
    }

    foreach (const QString &mountPoint, possibleMountPoints) {    //Iterates over each mount point.
        QString mountPath = QString("%1/%2").arg(mountPoint, username); //Constructs a path by combining the mount point and username.

        // List all directories in the mount point
        QProcess process;  //created to run the external ls -l command
        process.start("ls", QStringList() << "-l" << mountPath); // Start 'ls -l' command.
        process.waitForFinished(); //waits for the command to complete execution.

        QByteArray output = process.readAllStandardOutput(); //Reads process's standard output into QByteArray and stored in output.
        QString outputString = QString::fromUtf8(output);     //Converts QByteArray to QString.

        // Split the output into lines
        QStringList lines = outputString.split('\n');

        foreach (const QString &line, lines) {
            if (line.startsWith("d")) { // Check if it's a directory
                QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts); //Split line by whitespace, ignore empty.
                if (parts.size() > 8) {
                    QString dirName = parts.at(8);  //If more than 8 parts, get 9th field.
                    QString dirPath = QString("%1/%2").arg(mountPath, dirName);

                    // Store the detected pendrive path
                    pendrivePath = dirPath;
                    //qDebug() << "Detected pendrive at:" << dirPath;
                    return true; // Pendrive detected
                }
            }
        }
    }

    return false; // Pendrive not detected
}

void MainWindow::onButtonClicked() {

    // Detect if a pendrive is connected
    bool pendriveDetected = detectPendrive();

    if (!pendriveDetected) {
        QMessageBox::warning(this, "No Pendrive Detected", "Please insert a pendrive and try again.");
        qDebug() << "No Pendrive Detected: Please insert a pendrive and try again.";
        return;
    } else {
        qDebug() << "Pendrive detected at:" << pendrivePath;
    }

    // Get the current username
    QString username = getCurrentUsername();

    if (clientSocket && clientSocket->isOpen()) {
        const QString commandCode = "0xE034"; // Format for the code

              // Construct the packet data (just the username)
              QString commandParameter = username;
              QString dataForChecksum = commandCode + "|" + username;
              QString checksum = calculateChecksum(dataForChecksum); //calculate the value for commandcode and username.

              // Create the TCP packet with the format [code|username|checksum]
              QString tcpPacket = QString("[%1|%2|%3]").arg(commandCode, commandParameter, checksum);

              qDebug() << "Sending packet:" << tcpPacket;

        // Convert the packet to QByteArray for transmission
       QByteArray data = tcpPacket.toUtf8();

        // Send the packet over the socket
        clientSocket->write(data);
        clientSocket->flush();
    } else {
        qDebug() << "Socket is not open, cannot send username.";
    }

}

void MainWindow::onConnection() {

    if (clientSocket) {
        clientSocket->disconnectFromHost();
        clientSocket->waitForDisconnected();
        delete clientSocket;
        clientSocket = nullptr;
    }

    clientSocket = tcpServer->nextPendingConnection();

    // Convert address to IPv4 format if necessary
    QString clientAddress = clientSocket->peerAddress().toString();
    QHostAddress address = clientSocket->peerAddress();
    if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        address = QHostAddress(address.toIPv4Address());
        clientAddress = address.toString();
    }

    qDebug() << "Client connection from" << clientAddress;

    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
}

void MainWindow::onReadyRead() {
    if (clientSocket) {
        QByteArray data = clientSocket->readAll();
        QString packet = QString::fromUtf8(data).trimmed();
        qDebug() << "Received message from client:" << packet;

        // Ensure the packet starts with '[' and ends with ']'
        if (packet.startsWith('[') && packet.endsWith(']')) {
            QString trimmedPacket = packet.mid(1, packet.length() - 2); // Remove surrounding brackets
            QStringList fields = trimmedPacket.split('|');

            if (fields.size() == 2) { // Expected 2 fields: command code and checksum
                QString commandCode = fields[0];
                QString checksum = fields[1]; // We assume this is just an end marker, no validation needed for now

                // Process the message based on the command code
                if (commandCode == "0xF00D") {
                    qDebug() << "Code indicates success. Processing successful operation.";

                    // Handle success operation here
                    QString username = getCurrentUsername();
                    if (username.isEmpty()) {
                        qDebug() << "Failed to get current username.";
                        return;
                    }

                    // Format the local directory path with the current username
                    QString localDirectoryPath = QString("/home/%1/temp_logs").arg(username);
                    qDebug() << "Local directory path:" << localDirectoryPath;

                    // Proceed with copying the directory to the pendrive
                    if (!pendrivePath.isEmpty()) {
                        copyDirectoryToPendrive(localDirectoryPath, pendrivePath);
                    } else {
                        qDebug() << "Pendrive path is not set.";
                    }

                } else if (commandCode == "0xF00E") {
                    qDebug() << "Code indicates failure. Handling failure.";

                    // Handle failure scenario here (e.g., log an error, notify the user, etc.)
                } else {
                    qDebug() << "Unhandled code from client:" << commandCode;
                }
            } else {
                qDebug() << "Invalid packet format. Expected 2 fields but received:" << fields.size();
            }
        } else {
            qDebug() << "Invalid packet format. Message should start with '[' and end with ']'.";
        }
    } else {
        qDebug() << "Client socket is not valid.";
    }
}

void MainWindow::copyDirectoryToPendrive(const QString& localDirectoryPath, const QString& pendrivePath) {
    QString password = "Shiv@2703!";  // Password of the current system.

    // Format the SCP command to copy the local directory to the pendrive
    QString copyCommand = QString("echo %1 | sudo -S cp -rf %2 %3")
            .arg(password)
            .arg(localDirectoryPath)
            .arg(pendrivePath);

    qDebug() << "Executing copy command:" << copyCommand;

    int result = system(copyCommand.toUtf8().constData());

    if (result != 0) {
        qDebug() << "Failed to copy directory. Exit code:" << result;
    } else {
        qDebug() << "Directory copied successfully from" << localDirectoryPath << "to" << pendrivePath;
    }

    // If the copy succeeds, delete the local directory
          QString deleteCommand = QString("echo %1 | sudo -S rm -rf %2")
                  .arg(password)
                  .arg(localDirectoryPath);

          qDebug() << "Executing delete command:" << deleteCommand;

          int deleteResult = system(deleteCommand.toUtf8().constData());

          if (deleteResult != 0) {
              qDebug() << "Failed to delete local directory. Exit code:" << deleteResult;
          } else {
              qDebug() << "Local directory deleted successfully:" << localDirectoryPath;
          }
}

//Calculate SHA-256 checksum
QString MainWindow::calculateChecksum(const QString& data) const {
    QByteArray byteArray = data.toUtf8();
    QByteArray hash = QCryptographicHash::hash(byteArray, QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

void MainWindow::sendAcknowledgment(QTcpSocket *socket, const QString &message) {
    if (socket && socket->isOpen()) {
        // Create the packet with checksum
        QString Checksum = calculateChecksum(message);
        QString packet = QString("[%1|%2]").arg(message).arg(Checksum);

        // Convert the packet to QByteArray for transmission
        QByteArray data = packet.toUtf8();

        // Log the packet being sent
        qDebug() << "Sending packet:" << packet;

        // Write the data to the socket
        qint64 bytesWritten = socket->write(data);
        socket->flush();

        if (bytesWritten == -1) {
            qDebug() << "Failed to write to socket:" << socket->errorString();
        } else if (bytesWritten < data.size()) {
            qDebug() << "Not all data was written to the socket.";
        }
    } else {
        qDebug() << "Socket is not open.";
    }
}

void MainWindow::receivePacket(QTcpSocket *socket) {
    if (socket && socket->isOpen()) {
        // Read the packet
        QByteArray data = socket->readAll();
        QString receivedData = QString::fromUtf8(data);

        // Extract the message and checksum from the packet
        QStringList parts = receivedData.split('|');
        if (parts.size() < 2) {
            qDebug() << "Invalid packet format.";
            return;
        }

        QString message = parts[0];
        QString receivedChecksum = parts[1];

        // Calculate checksum for the received message
        QString calculatedChecksum = calculateChecksum(message);

        // Validate the checksum
        if (calculatedChecksum == receivedChecksum) {
            qDebug() << "Checksum validation successful.";
            qDebug() << "Received message:" << message;
        } else {
            qDebug() << "Checksum validation failed.";
        }
    } else {
        qDebug() << "Socket is not open.";
    }
}


