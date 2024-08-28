#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCryptographicHash>

// Constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), tcpSocket(new QTcpSocket(this)), serverAddress("192.168.5.1"), serverPort(1234) {
    ui->setupUi(this);

    // Connect the signal and slot for reading data from the server
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::readServerData);

    // Connect to the server
    tcpSocket->connectToHost(serverAddress, serverPort);
    if (!tcpSocket->waitForConnected(3000)) {
        qDebug() << "Error connecting to server:" << tcpSocket->errorString();
    } else {
        qDebug() << "Connected to server";
    }
}

// Destructor
MainWindow::~MainWindow() {
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->disconnectFromHost();
        if (!tcpSocket->waitForDisconnected(3000)) {
            qDebug() << "Error while disconnecting:" << tcpSocket->errorString();
        }
    } else if (tcpSocket->state() == QAbstractSocket::ClosingState) {
        tcpSocket->waitForDisconnected();
    }
    delete tcpSocket;
}

//Calculate SHA-256 checksum
QString MainWindow::calculateChecksum(const QString& data) const {
    QByteArray byteArray = data.toUtf8();
    QByteArray hash = QCryptographicHash::hash(byteArray, QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

void MainWindow::parsePacket(const QString& packet) {

    QString trimmedPacket = packet.mid(1, packet.length() - 2); //Extract substring from packet, excluding first and last characters.

    // Split packet into fields by '|'
    QStringList fields = trimmedPacket.split('|');

    // Initialize variables to store extracted values
    QString commandCode;
    QString serverUsername;
    QString receivedChecksum;

    // Ensure the packet has the expected number of fields
    if (fields.size() == 3) {
        commandCode = fields[0].trimmed();
        serverUsername = fields[1].trimmed();
        receivedChecksum = fields[2].trimmed();
    } else {
        qDebug() << "Invalid packet format.";
        return;
    }

    // Output the parsed data for verification
    qDebug() << "Code:" << commandCode;
    qDebug() << "Server Username:" << serverUsername;
    qDebug() << "Checksum:" << receivedChecksum;

    // Construct the data string for checksum verification
    QString dataForChecksum = commandCode + "|" + serverUsername;
    QString calculatedChecksum = calculateChecksum(dataForChecksum);

    // Verify the checksum
    if (calculatedChecksum == receivedChecksum) {
        qDebug() << "Checksum verified successfully.";
        qDebug() << " code:" << commandCode;

        // Handle the command based on the code
        if (commandCode == "0xE034") {
            qDebug() << "Received START_EXECUTION command. Executing...";

            // Store the server username
            GetServerUsername(serverUsername);
            //qDebug() << "Received just  debugggggg...";
            usernameReceived = true;

            // Execute start_execution if the username has been received
            if (usernameReceived) {
                qDebug() << "Username received. Calling start_execution..."<< serverUsername;
                start_execution();
            }
        }
    } else {
        qDebug() << "Checksum mismatch! Packet may be corrupted.";
    }
}

void MainWindow::readServerData() {
    if (tcpSocket->bytesAvailable() > 0) {
        QByteArray data = tcpSocket->readAll();
        QString content = QString::fromUtf8(data).trimmed();

        // Log the received data
        qDebug() << "Client received data:" << data;
        qDebug() << "Client received content:" << content;

        // Pass the received content to parsePacket for parsing and checksum verification
        parsePacket(content);
    } else {
        qDebug() << "No data available to read.";
    }
}

//This function processes and stores a server username.
void MainWindow::GetServerUsername(const QString &username) {
    if (!username.isEmpty()) {
        this->serverUsername = username; //Stores the received username in the class member serverUsername.
        usernameReceived = true;
        qDebug() << "Server username received and stored:" << this->serverUsername;
    } else {
        qDebug() << "Received an empty username. Not storing.";
    }
}

//This function retrieves and returns the current system username.
QString MainWindow::getCurrentUsername() {
    QProcess process;
    process.start("logname");  //command to get the current username.
    process.waitForFinished();
    QByteArray output = process.readAllStandardOutput();
    return QString(output).trimmed(); // Remove any extra whitespace/newlines
}

void MainWindow::start_execution() {
    qDebug() << "Executing start_execution...";
    sendLogsToServer();
}

void MainWindow::sendLogsToServer() {
    QString username = getCurrentUsername();
    QString CurrentSysPassword = "Sach@151084!";

    // Check if the username was retrieved successfully
    if (username.isEmpty()) {
        qDebug() << "Failed to get current username. Exiting.";
        return;
    } else {
        qDebug() << "Current system  username:" << username;
    }

    QString baseLogsDirectoryPath = QString("/media/%1/Logs/SVDGS-Logs").arg(username); //Path for directory to get the Logs.
    QStringList logDirectories = { "coreLog", "eventLogs", "QtLog" }; // Directories to find the Logs.

    // Create a temporary directory to store the found files
    QString tempDirectory = QString("/home/%1/temp_logs").arg(username);  //Temperary Directory for get all the Logs files.
    QString createDirCommand = QString("mkdir -p %1").arg(tempDirectory);  //Create the Tempaerary Directory
    system(createDirCommand.toUtf8().constData());

    QString findCommand;
    for (const QString& logDir : logDirectories) {
        findCommand += QString("find %1/%2 -type f -mtime -7 -print && ") //Find last 7-days files and print.
                       .arg(baseLogsDirectoryPath)
                       .arg(logDir);
    }

    findCommand.chop(3); //Remove last three characters

    QProcess FindProcess;
    FindProcess.start("bash", QStringList() << "-c" << findCommand); //ists files or directories based on the findCommand criteria.

    //Check execution status and log errors if findCommand fails.
    if (!FindProcess.waitForFinished()) {
        qDebug() << "Failed to execute find command:" << FindProcess.errorString();
        qDebug() << "Error output:" << FindProcess.readAllStandardError();
        return;
    }

    QString output = FindProcess.readAllStandardOutput().trimmed(); //Read and trim process output.

    //Verify if output is empty or has content.
    if (output.isEmpty()) {
        qDebug() << "find command produced no output.";
    } else {
        qDebug() << "find command executed successfully. Output:";

        // Split the output into a list of files
        QStringList files = output.split('\n', QString::SkipEmptyParts);

        // Iterate over the found files and copy each one to the temp directory
        foreach (const QString &file, files) {
            QString copyCommand = QString("echo %1 | sudo -S cp -p '%2' '%3/'")
                    .arg(CurrentSysPassword)
                    .arg(file)
                    .arg(tempDirectory);

            int copyResult = system(copyCommand.toUtf8().constData());

            //Check if copyCommand succeeded and log result.
            if (copyResult == 0) {
                qDebug() << "Copied file to temp directory:" << file;
            } else {
                qDebug() << "Failed to copy file:" << file;
            }
        }

        //Set remote host, password, and username.
      QString remoteHost = "192.168.5.1";
      QString remotePassword = "Shiv@2703!";
      QString remoteUsername = serverUsername;

    // the remote path including the remote username
    QString remotepath = QString("/home/%1").arg(remoteUsername);

    //Build scp command with parameters.
    QString scpCommand = QString("echo '%1' | sudo -S sshpass -p '%2' scp -rp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null '%3' %4@%5:%6")
            .arg(CurrentSysPassword)
            .arg(remotePassword)
            .arg(tempDirectory)
            .arg(remoteUsername)
            .arg(remoteHost)
            .arg(remotepath);

    qDebug() << "Executing SCP command:" << scpCommand;

    int scpResult = system(scpCommand.toUtf8().constData()); //Execute scpCommand and store result.

    //Handle success, then clean up temporary directory.
    if (scpResult == 0) {
        qDebug() << "Successfully copied directory to remote path:" << remotepath;
        sendPacket(tcpSocket, SUCCESSFULLY_COPIED_FILES); //Send success packet.

        // Clean up: Remove the temporary directory after successful copying
        QString cleanupCommand = QString("rm -rf %1").arg(tempDirectory);
        int cleanupResult = system(cleanupCommand.toUtf8().constData());

        if (cleanupResult != 0) {
            qDebug() << "Failed to remove temp directory:" << tempDirectory;
        } else {
            qDebug() << "Temporary directory removed:" << tempDirectory;
        }

    } else {
        qDebug() << "Failed to copy directory to remote path.";
        sendPacket(tcpSocket, FAILED_TO_COPY); //Send failed packet.

    }

}
}

void MainWindow::sendPacket(QTcpSocket *socket, const QString &message) {
    if (socket && socket->isOpen()) {
        // Create a packet with checksum
        QString Checksum = calculateChecksum(message);
        QString packet = QString("[%1|%2]").arg(message, Checksum); // Assuming the format is [message|checksum]
        QByteArray data = packet.toUtf8(); //Convert packet to UTF-8 QByteArray.

        qint64 bytesWritten = socket->write(data); //Write data to socket and store result.
        socket->flush();  //immediate data transmission.

        //Check write status: log failure or incomplete data.
        if (bytesWritten == -1) {
            qDebug() << "Failed to write to socket:" << socket->errorString();
        } else if (bytesWritten < data.size()) {
            qDebug() << "Not all data was written to the socket.";
        }
    } else {
        qDebug() << "Socket is not open.";
    }
}

