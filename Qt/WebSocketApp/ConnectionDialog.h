#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include "WebSocketApp.h"
#include "SocketMessage.h"

#include <QDialog>
#include <QWebSocket>
#include <QGraphicsScene>

namespace Ui {
class ConnectionDialog;
}

class MainWindow;

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(const std::string& address, ushort port, MainWindow *parent);
    ~ConnectionDialog();

    bool IsConnect() const {
        return _connectFlag && _socket != nullptr;
    }

    const std::string& Address() const {
        return _address;
    }
    void SetAddress(const std::string& address) {
        _address = address;
        UpdatePath();
    }

    ushort Port() const {
        return _port;
    }
    void SetPort(ushort port) {
        _port = port;
        UpdatePath();
    }

    bool Open();
    bool Open(const std::string& address, ushort port) {
        SetAddress(address);
        SetPort(port);
        return Open();
    }

    void Close();

private slots:
    void onConnected();
    void onAboutToClose();
    void onClosed();
    void onError(QAbstractSocket::SocketError error);
    void onStateChanged(QAbstractSocket::SocketState state);
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);

    void on_ClearButton_clicked();
    void on_ConnectButton_clicked();

    void on_ScreenShotButton_clicked();

    void on_FileListButton_clicked();

    void on_downloadButton_clicked();

    void on_uploadButton_clicked();

    void on_sendTextutton_clicked();

    void on_gameObjectButton_clicked();

    void on_upButton_clicked();

    void on_downButton_clicked();

    void on_leftButton_clicked();

    void on_rightButton_clicked();

    void on_stopUpdate_clicked();

private:
    Ui::ConnectionDialog *ui;

    MainWindow* _mainWindow;
    QWebSocket *_socket;
    std::string _address;
    ushort _port;
    bool _connectFlag;
    std::string _path;
    int _manipulateTarget;

    std::map<int, std::string> _fileRequestMap;

    const std::string& UpdatePath() {
        _path = WebSocketApp::StringBuilder::Format("ws://%s:%d%s", _address.c_str(), _port, WebSocketApp::SOCKET_PATH);
        return _path;
    }

    void SetConnectFlag(bool flag) {
        _connectFlag = flag;
        UpdateConnectFlag();
    }
    void UpdateConnectFlag();

    bool AcceptMessage(SocketMessageBase* message);
    bool AcceptMessage(SocketLogMessage* message);
    bool AcceptMessage(SocketConnectionInformationMessage* message);
    bool AcceptMessage(SocketFileListMessage* message);
    bool AcceptMessage(SocketFileMessage* message);
    bool AcceptMessage(SocketScreenShotMessage* message);
    void WriteInfoLog(const QString& log) {
        WriteLog(SocketLogMessage::LogType::Log, log);
    }
    void WriteWarningLog(const QString& log) {
        WriteLog(SocketLogMessage::LogType::Warning, log);
    }
    void WriteErrorLog(const QString& log) {
        WriteLog(SocketLogMessage::LogType::Error, log);
    }
    bool WriteLog(SocketLogMessage::LogType logType, const QString& log);
    void WriteLog(const QString& log);

    bool SendMessage(const SocketMessageBase& message);

    void closeEvent(QCloseEvent *event) override;
};

#endif // CONNECTIONDIALOG_H
