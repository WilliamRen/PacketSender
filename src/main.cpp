/*
 * This file is part of Packet Sender
 *
 * Licensed GPL v2
 * http://PacketSender.com/
 *
 * Copyright Dan Nagle
 *
 */

#include <QtWidgets/QApplication>
#include <QDir>
#include <QDesktopServices>
#include <QCommandLineParser>
#include <QHostInfo>

#include "mainwindow.h"
#define DEBUGMODE 0

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QFile logfile(LOGFILE);
    Q_UNUSED(context);
    if(!logfile.isOpen())
    {
        logfile.open(QFile::Append);
    } else {
//        logfile.flush();
    }
    switch (type) {
      default:
        logfile.write("\n");
        logfile.write(msg.toLatin1());
        break;
      case QtFatalMsg:
         logfile.flush();
         abort();
      }
}



void myMessageOutputDisable(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{

    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

#define OUTVAR(var)  o<< "\n" << # var << ":" << var ;
#define OUTIF()  if(!quiet) o<< "\n"
#define OUTPUT() outBuilder = outBuilder.trimmed(); outBuilder.append("\n"); out << outBuilder


int main(int argc, char *argv[])
{
    int debugMode = DEBUGMODE;


    if(debugMode)
    {

    } else {
        qInstallMessageHandler(myMessageOutputDisable);

    }

    QDEBUG() << "number of arguments:" << argc;
    QApplication a(argc, argv);
    QStringList args = a.arguments();
    QDEBUGVAR(args);

    qRegisterMetaType<Packet>();


    QFile file(":/packetsender.css");
    if(file.open(QFile::ReadOnly)) {
       QString StyleSheet = QLatin1String(file.readAll());
     //  qDebug() << "stylesheet: " << StyleSheet;
       a.setStyleSheet(StyleSheet);
    }


    if(args.size() > 1) {
        QDEBUG() << "Running command line mode.";
        QSettings settings(SETTINGSFILE, QSettings::IniFormat);
        int udpPort = settings.value("udpPort", 55056).toInt();
        int tcpPort = settings.value("tcpPort", 55056).toInt();

        Packet sendPacket;
        sendPacket.init();
        QString outBuilder;

        QTextStream o(&outBuilder);


        QTextStream out(stdout);

        QDate vDate = QDate::fromString(QString(__DATE__).simplified(), "MMM d yyyy");
        QCoreApplication::setApplicationName("Packet Sender");
        QCoreApplication::setApplicationVersion("version " + vDate.toString("yyyy-MM-dd"));

        QCommandLineParser parser;
        parser.setApplicationDescription("Packet Sender is a Network TCP and UDP Test Utility by Dan Nagle\nSee http://PacketSender.com/ for more information.");
        parser.addHelpOption();
        parser.addVersionOption();

        // A boolean option with a single name (-p)
        QCommandLineOption quietOption(QStringList() << "q" << "quiet", "Quiet mode. Only output received data.");
        parser.addOption(quietOption);


        // A boolean option with multiple names (-f, --force)
        QCommandLineOption hexOption(QStringList() << "x" << "hex", "Parse data as hex (default).");
        parser.addOption(hexOption);

        // A boolean option with multiple names (-f, --force)
        QCommandLineOption asciiOption(QStringList() << "a" << "ascii", "Parse data as mixed-ascii (like the GUI).");
        parser.addOption(asciiOption);

        // A boolean option with multiple names (-f, --force)
        QCommandLineOption pureAsciiOption(QStringList() << "A" << "ASCII", "Parse data as pure ascii (no \\xx translation).");
        parser.addOption(pureAsciiOption);


        // An option with a value
        QCommandLineOption waitOption(QStringList() << "w" << "wait",
                "Wait up to <milliseconds> for a response after sending. Zero means do not wait (Default).",
                "milliseconds");
        parser.addOption(waitOption);


        // An option with a value
        QCommandLineOption bindPortOption(QStringList() << "b" << "bind",
                "Bind port. Default is 0 (dynamic).",
                "port");
        parser.addOption(bindPortOption);


        // A boolean option with multiple names (-f, --force)
        QCommandLineOption tcpOption(QStringList() << "t" << "tcp", "Send TCP (default).");
        parser.addOption(tcpOption);

        // A boolean option with multiple names (-f, --force)
        QCommandLineOption udpOption(QStringList() << "u" << "udp", "Send UDP.");
        parser.addOption(udpOption);

        // An option with a value
        QCommandLineOption nameOption(QStringList() << "n" << "name",
                                      "Send previously saved packet named <name>. Other options overrides saved packet parameters.",
                                      "name");
        parser.addOption(nameOption);


        parser.addPositionalArgument("address", "Destination address. Optional for saved packet.");
        parser.addPositionalArgument("port", "Destination port. Optional for saved packet.");
        parser.addPositionalArgument("data", "Data to send. Optional for saved packet.");


        // Process the actual command line arguments given by the user
        parser.process(a);

        const QStringList args = parser.positionalArguments();

        bool quiet = parser.isSet(quietOption);
        bool hex = parser.isSet(hexOption);
        bool mixedascii = parser.isSet(asciiOption);
        bool ascii = parser.isSet(pureAsciiOption);
        unsigned int wait = parser.value(waitOption).toUInt();
        unsigned int bind = parser.value(bindPortOption).toUInt();
        bool tcp = parser.isSet(tcpOption);
        bool udp = parser.isSet(udpOption);
        QString name = parser.value(nameOption);

        QString address = "";
        unsigned int port = 0;

        int argssize = args.size();
        QString data, dataString;
        data.clear();
        dataString.clear();
        if(argssize >= 1) {
            address = args[0];
        }
        if(argssize >= 2) {
            port = args[1].toUInt();
        }
        if(argssize >= 3) {
            data = (args[2]);
        }

        //check for invalid options..

        if(argssize > 3) {
            OUTIF() << "Warning: Extra parameters detected. Try surrounding your data with quotes.";
        }

        if(hex && mixedascii) {
            OUTIF() << "Warning: both hex and pure ascii set. Defaulting to hex.";
            mixedascii = false;
        }

        if(hex && ascii) {
            OUTIF() << "Warning: both hex and pure ascii set. Defaulting to hex.";
            ascii = false;
        }

        if(mixedascii && ascii) {
            OUTIF() << "Warning: both mixed ascii and pure ascii set. Defaulting to pure ascii.";
            mixedascii = false;
        }

        if(tcp && udp) {
            OUTIF() << "Warning: both TCP and UDP set. Defaulting to TCP.";
            udp = false;
        }

        //bind is now default 0

        if(!bind && parser.isSet(bindPortOption)) {
            OUTIF() << "Warning: Binding to port zero is dynamic.";
        }


        if(!port && name.isEmpty()) {
            OUTIF() << "Warning: Sending to port zero.";
        }

        //set default choices
        if(!hex && !ascii && !mixedascii) {
            hex = true;
        }

        if(!tcp && !udp) {
            tcp = true;
        }

        //Create the packet to send.
        if(!name.isEmpty()) {
            sendPacket = Packet::fetchFromDB(name);
            if(sendPacket.name.isEmpty()) {
                OUTIF() << "Error: Saved packet \""<< name <<"\" not found.";
                OUTPUT();
                return -1;
            } else {
                if(data.isEmpty()) {
                    data  = sendPacket.hexString;
                    hex = true;
                    ascii = false;
                    mixedascii = false;
                }
                if(!port) {
                    port  = sendPacket.port;
                }
                if(address.isEmpty()) {
                    address  = sendPacket.toIP;
                }
                if(!parser.isSet(tcpOption) && !parser.isSet(udpOption)) {

                    if(sendPacket.tcpOrUdp.toUpper() == "TCP") {
                        tcp=true;
                        udp = false;
                    } else {
                        tcp=false;
                        udp = true;
                    }

                }

            }

        }

        if(!parser.isSet(bindPortOption)) {        
            bind = 0;
        }



        QDEBUGVAR(argssize);
        QDEBUGVAR(quiet);
        QDEBUGVAR(hex);
        QDEBUGVAR(mixedascii);
        QDEBUGVAR(ascii);
        QDEBUGVAR(address);
        QDEBUGVAR(port);
        QDEBUGVAR(wait);
        QDEBUGVAR(bind);
        QDEBUGVAR(tcp);
        QDEBUGVAR(udp);
        QDEBUGVAR(name);
        QDEBUGVAR(data);


        //NOW LETS DO THIS!

        if(ascii) { //pure ascii
            dataString = Packet::byteArrayToHex(data.toLatin1());
        }

        if(hex) { //hex
            dataString = Packet::byteArrayToHex(Packet::HEXtoByteArray(data));
        }
        if(mixedascii) { //mixed ascii
            dataString = Packet::ASCIITohex(data);
        }

        if(dataString.isEmpty()) {
            OUTIF() << "Warning: No data to send. Is your formatting correct?";
        }

        QHostAddress addy;
        if(!addy.setAddress(address)) {
            QHostInfo info = QHostInfo::fromName(address);
            if (info.error() != QHostInfo::NoError)
            {
                OUTIF() << "Error: Could not resolve address:" + address;
                OUTPUT();
                return -1;
            } else {
                addy = info.addresses().at(0);
                address = addy.toString();
            }
        }


        QByteArray sendData = sendPacket.HEXtoByteArray(dataString);
        QByteArray recvData;
        recvData.clear();
        int bytesWriten = 0;
        int bytesRead = 0;


        if(tcp) {
            QTcpSocket sock;
            sock.bind(QHostAddress::Any, bind);
            sock.connectToHost(addy, port);
            sock.waitForConnected(1000);
            if(sock.state() == QAbstractSocket::ConnectedState)
            {
                OUTIF() << "TCP (" <<sock.localPort() <<")://" << address << ":" << port << " " << dataString;
                bytesWriten = sock.write(sendData);
                sock.waitForBytesWritten(1000);
                //OUTIF() << "Sent:" << Packet::byteArrayToHex(sendData);
                if(wait) {
                    sock.waitForReadyRead(wait);
                    recvData = sock.readAll();
                    bytesRead = recvData.size();
                    QString hexString = Packet::byteArrayToHex(recvData);
                    if(quiet) {
                        o << "\n" << hexString;
                    } else {
                        o << "\nResponse HEX:" << hexString;
                        o << "\nResponse ASCII:" << Packet::hexToASCII(hexString);
                    }
                }
                sock.disconnectFromHost();
                sock.waitForDisconnected(1000);
                sock.close();

                OUTPUT();
                return bytesWriten;


            } else {
                OUTIF() << "Error: Failed to connect to " << address;

                OUTPUT();
                return -1;
            }


        } else {
            QUdpSocket sock;
            if(!sock.bind(QHostAddress::Any, bind)) {
                OUTIF() << "Error: Could not bind to " << bind;

                OUTPUT();
                return -1;
            }
            OUTIF() << "UDP (" <<sock.localPort() <<")://" << address << ":" << port << " " << dataString;

            bytesWriten = sock.writeDatagram(sendData, addy, port);
            //OUTIF() << "Wrote " << bytesWriten << " bytes";
            sock.waitForBytesWritten(1000);

            if(wait) {
                sock.waitForReadyRead(wait);

                if(sock.hasPendingDatagrams()) {
                    QHostAddress sender;
                    quint16 senderPort;
                    recvData.resize(sock.pendingDatagramSize());

                    sock.readDatagram(recvData.data(), recvData.size(),
                                            &sender, &senderPort);

                    QString hexString = Packet::byteArrayToHex(recvData);
                    if(quiet) {
                        o << "\n" << hexString;
                    } else {
                        o << "\nResponse HEX:" << hexString;
                        o << "\nResponse ASCII:" << Packet::hexToASCII(hexString);
                    }
                }
            }

            sock.close();

            OUTPUT();
            return bytesWriten;

        }





        OUTPUT();
        return 0;
    }


    MainWindow w;


    w.show();

    return a.exec();
}
