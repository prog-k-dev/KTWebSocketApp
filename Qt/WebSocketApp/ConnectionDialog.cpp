#include "ConnectionDialog.h"
#include "MainWindow.h"
#include "SocketMessage.h"
#include "ui_ConnectionDialog.h"

#include <QDir>
#include <QStandardPaths>
#include <QCloseEvent>

ConnectionDialog::ConnectionDialog(const std::string& address, ushort port, MainWindow *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionDialog)
    , _mainWindow(parent)
    , _socket(nullptr)
    , _address(address)
    , _port(port)
    , _connectFlag(false)
{
    ui->setupUi(this);
    ui->log->setTextColor(WebSocketApp::LOG_NORMAL_COLOR);

    auto buttons = ui->pathButtonGroup->buttons();
    for (int i = 0, count = buttons.count(); i < count; ++i) {
        auto button = buttons[i];
        ui->pathButtonGroup->setId(button, i);
    }

    UpdatePath();
    UpdateConnectFlag();
}

ConnectionDialog::~ConnectionDialog()
{
    Close();
    delete ui;
}

void ConnectionDialog::closeEvent(QCloseEvent */*event*/) {
    _mainWindow->CloseConnection(this);

    Close();
}

bool ConnectionDialog::Open() {
    Close();

    _socket = new QWebSocket();
    connect(_socket, &QWebSocket::connected, this, &ConnectionDialog::onConnected);
    connect(_socket, &QWebSocket::disconnected, this, &ConnectionDialog::onClosed);
    connect(_socket, &QWebSocket::aboutToClose, this, &ConnectionDialog::onAboutToClose);
    connect(_socket, &QWebSocket::stateChanged, this, &ConnectionDialog::onStateChanged);
    connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), [=](QAbstractSocket::SocketError error) {
        onError(error);
    });

    _socket->open(QUrl(_path.c_str()));

    return true;
}

void ConnectionDialog::Close() {
    if (_socket != nullptr) {
        _socket->close();

        disconnect(_socket, &QWebSocket::textMessageReceived, this, &ConnectionDialog::onTextMessageReceived);
        disconnect(_socket, &QWebSocket::binaryMessageReceived, this, &ConnectionDialog::onBinaryMessageReceived);

        delete _socket;
        _socket = nullptr;
    }
}

void ConnectionDialog::UpdateConnectFlag() {
    if (_connectFlag) {
        ui->status->setText(("接続済：" + _path).c_str());
        ui->ConnectButton->setText("切断");
        ui->ConnectButton->setStyleSheet("color:black; background-color:white;");
    } else {
        ui->status->setText(("未接続：" + _path).c_str());
        ui->ConnectButton->setText("接続");
        ui->ConnectButton->setStyleSheet("color:white; background-color:red;");
        setWindowTitle("ソケット接続");
    }
    ui->ScreenShotButton->setEnabled(_connectFlag);
}

void ConnectionDialog::onConnected() {
    SetConnectFlag(true);

    connect(_socket, &QWebSocket::textMessageReceived, this, &ConnectionDialog::onTextMessageReceived);
    connect(_socket, &QWebSocket::binaryMessageReceived, this, &ConnectionDialog::onBinaryMessageReceived);

    SocketRequestMessage message(SocketConnectionInformationMessage::MESSAGE_TYPE);
    SendMessage(message);

    SendMessage(*MainWindow::GetConnectionInformation());
}

void ConnectionDialog::onClosed() {
    SetConnectFlag(false);

    WriteInfoLog(QFORMAT_STR("WebSocket切断: アドレス=%s, ポート=%d", _address.c_str(), _port));
}

void ConnectionDialog::onTextMessageReceived(const QString &src) {
    SocketMessageBase* message = SocketMessageBase::ImportMessage(src);
    AcceptMessage(message);
    if (message != nullptr) {
        delete message;
        message = nullptr;
    }
}

void ConnectionDialog::onBinaryMessageReceived(const QByteArray &message) {
    WriteErrorLog(QFORMAT_STR("バイナリメッセージを受信しましたが、対応していません(length=%d)", message.length()));
}

bool ConnectionDialog::AcceptMessage(SocketMessageBase* message) {
    if (message == nullptr) {
        return false;
    }

    if (message->MessageType() == SocketLogMessage::MESSAGE_TYPE) {
        return AcceptMessage(dynamic_cast<SocketLogMessage*>(message));
    } else if (message->MessageType() == SocketConnectionInformationMessage::MESSAGE_TYPE) {
        return AcceptMessage(dynamic_cast<SocketConnectionInformationMessage*>(message));
    } else if (message->MessageType() == SocketFileListMessage::MESSAGE_TYPE) {
        return AcceptMessage(dynamic_cast<SocketFileListMessage*>(message));
    } else if (message->MessageType() == SocketFileMessage::MESSAGE_TYPE) {
        return AcceptMessage(dynamic_cast<SocketFileMessage*>(message));
    } else if (message->MessageType() == SocketScreenShotMessage::MESSAGE_TYPE) {
        return AcceptMessage(dynamic_cast<SocketScreenShotMessage*>(message));
    } else {
        WriteErrorLog(QFORMAT_STR("不明なテキストメッセージを受信しました：%s", message->MessageType().c_str()));
        return false;
    }

    return false;
}

bool ConnectionDialog::AcceptMessage(SocketLogMessage* message) {
    if (message == nullptr) {
        return false;
    }

    QString log = QFORMAT_STR("[%s] ", message->Time().c_str()) + message->Log().c_str();
    WriteLog(message->Type(), log);
    return true;
}

bool ConnectionDialog::AcceptMessage(SocketConnectionInformationMessage* message) {
    if (message == nullptr) {
        return false;
    }

    auto title = QFORMAT_STR("デバイス[%s(%s)] で動作中のアプリケーション[%s]に接続中", message->DeviceName().c_str(), message->DeviceModel().c_str(), message->ApplicationName().c_str());
    setWindowTitle(title);
    return true;
}

bool ConnectionDialog::AcceptMessage(SocketFileListMessage* message) {
    if (message == nullptr) {
        return false;
    }


    WriteInfoLog(QFORMAT_STR("ファイルリスト: ディレクトリ=%s", message->Directory().c_str()));
    WriteInfoLog(QFORMAT_STR("【ディレクトリ数: %d】", message->Directories().size()));
    for (int i = 0, count = message->Directories().size(); i < count; ++i) {
        const auto& directory = message->Directories()[i];
        WriteInfoLog(QFORMAT_STR("  [%d] %s", i, directory.c_str()));
    }
    WriteInfoLog(QFORMAT_STR("【ファイル数: %d】", message->Files().size()));
    for (int i = 0, count = message->Files().size(); i < count; ++i) {
        const auto& file = message->Files()[i];
        WriteInfoLog(QFORMAT_STR("  [%d] %s", i, file.c_str()));
    }

    return true;
}

bool ConnectionDialog::AcceptMessage(SocketFileMessage* message) {
    if (message == nullptr) {
        return false;
    }

    auto it = _fileRequestMap.find(message->RequestId());
    if (it == _fileRequestMap.end()) {
        WriteErrorLog(QFORMAT_STR("受信したファイルのリクエスト情報が存在しない：RequestID=%d", message->RequestId()));
        return false;
    }

    auto fileName = it->second;
    auto index = fileName.find_first_of('/');
    if (index != std::string::npos) {
        fileName = fileName.substr(index + 1);
    }

    const QString downloadDirectoryPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir downloadDirectory(downloadDirectoryPath);
    QString filePath = downloadDirectory.absoluteFilePath(fileName.c_str());

    std::vector<char> buffer;
    {
        buffer.resize(message->Bytes().count());
        memcpy(&(buffer[0]), message->Bytes().data(), buffer.size());
    }
    if (!WebSocketApp::writeFile(filePath.toLocal8Bit().data(), buffer)) {
        return false;
    }
    WriteInfoLog(QFORMAT_STR("ファイルを受信しました：%s", filePath.toUtf8().data()));

    return true;
}

bool ConnectionDialog::AcceptMessage(SocketScreenShotMessage* message) {
    if (message == nullptr) {
        return false;
    }

    ui->image->SetImage(message->Bytes());
    ui->imageDate->setText(QFORMAT_STR("%s", message->DateTime().c_str()));

    return true;
}

bool ConnectionDialog::SendMessage(const SocketMessageBase& message) {
    if (!IsConnect()) {
        return false;
    }

    QString payload;
    if (!message.ExportMessage(payload)) {
        return false;
    }
    _socket->sendTextMessage(payload);

    return true;
}

void ConnectionDialog::onError(QAbstractSocket::SocketError error) {
    WriteErrorLog(QFORMAT_STR("SocketError: code(%d)", (int)error));
}

void ConnectionDialog::onAboutToClose() {
    WriteInfoLog("onAboutToClose()");
}

void ConnectionDialog::onStateChanged(QAbstractSocket::SocketState state) {
    WriteInfoLog(QFORMAT_STR("onStateChanged(): state=%d", (int)state));

    switch (state) {
    case QAbstractSocket::SocketState::ConnectingState:
        WriteInfoLog(QFORMAT_STR("接続処理中: Address=%s, Port=%d", _address.c_str(), _port));
        break;

    default:
        break;
    }
}

bool ConnectionDialog::WriteLog(SocketLogMessage::LogType logType, const QString& log) {
    switch (logType) {
    case SocketLogMessage::LogType::Log:
        ui->log->setTextColor(WebSocketApp::LOG_INFO_COLOR);
        break;
    case SocketLogMessage::LogType::Warning:
        ui->log->setTextColor(WebSocketApp::LOG_WARNING_COLOR);
        break;
    case SocketLogMessage::LogType::Error:
    case SocketLogMessage::LogType::Exception:
        ui->log->setTextColor(WebSocketApp::LOG_ERROR_COLOR);
        break;
    case SocketLogMessage::LogType::Invalid:
    default:
        break;
    }

    WriteLog(log);
    ui->log->setTextColor(WebSocketApp::LOG_NORMAL_COLOR);

    return true;
}

void ConnectionDialog::WriteLog(const QString& log) {
    if (log.isEmpty()) {
        return;
    }

    bool end = ui->log->textCursor().atEnd();
    if (log.at(log.length() - 1) != '\n') {
        ui->log->append(log);
    } else {
        auto index = log.indexOf("\n");
        ui->log->append(log.mid(0, index));
    }
    if (end) {
        ui->log->moveCursor(QTextCursor::MoveOperation::End, QTextCursor::MoveAnchor);
    }
}

void ConnectionDialog::on_ClearButton_clicked()
{
    ui->log->clear();
}

void ConnectionDialog::on_ConnectButton_clicked()
{
    if (IsConnect()) {
        Close();
    } else {
        Open();
    }
}

void ConnectionDialog::on_ScreenShotButton_clicked()
{
    float interval = (float)(ui->autoUpdate->checkState() == Qt::CheckState::Checked ? ui->interval->value() : -1);
    DEBUG_OUTPUT_INFO_LOG("on_ScreenShotButton_clicked(): interval=%lf", interval);
    SendMessage(SocketScreenShotRequestMessage(interval));
}
void ConnectionDialog::on_stopUpdate_clicked()
{
    SendMessage(SocketScreenShotRequestMessage(true));
}

void ConnectionDialog::on_FileListButton_clicked()
{
    UnityDirectoryType type = static_cast<UnityDirectoryType>(ui->pathButtonGroup->checkedId());
    SendMessage(SocketFileListRequestMessage(type, ui->fileName->text().toUtf8().data()));
}

void ConnectionDialog::on_downloadButton_clicked()
{
    auto fileName = ui->fileName->text().replace('\\', '/');
    if (fileName.isEmpty()) {
        WriteWarningLog("ファイル名を指定した「Unity Pathタイプ」からの相対パスで指定してください");
        return;
    }
    if (fileName != ui->fileName->text()) {
        ui->fileName->setText(fileName);
    }

    UnityDirectoryType type = static_cast<UnityDirectoryType>(ui->pathButtonGroup->checkedId());
    SocketFileUploadRequestMessage message(type, fileName.toUtf8().data());
    SendMessage(message);
    _fileRequestMap[message.RequestId()] = fileName.toUtf8().data();
}

void ConnectionDialog::on_uploadButton_clicked()
{
    auto fileName = ui->fileName->text().replace('\\', '/');
    if (fileName.isEmpty()) {
        WriteWarningLog("ファイル名をこのコンピュータ内のフルパスで指定してください");
        return;
    }
    if (fileName != ui->fileName->text()) {
        ui->fileName->setText(fileName);
    }

    if (!QFile::exists(fileName)) {
        WriteErrorLog(QFORMAT_STR("%sは存在しないかアクセス権がありません", fileName.toUtf8().data()));
        WriteWarningLog("ファイル名をこのコンピュータ内のフルパスで指定してください");
        return;
    }

    UnityDirectoryType type = static_cast<UnityDirectoryType>(ui->pathButtonGroup->checkedId());
    SocketFileMessage message;
    if (!message.SetFile(fileName.toUtf8().data(), type)) {
        return;
    }
    SendMessage(message);
}

void ConnectionDialog::on_sendTextutton_clicked()
{
    if (ui->text->text().isEmpty()) {
        WriteWarningLog("送信するテキストを入力してください");
        return;
    }

    SockeTextMessage message(ui->text->text().toUtf8().data());
    SendMessage(message);
}

void ConnectionDialog::on_gameObjectButton_clicked()
{
    if (ui->gameObjectName->text().isEmpty()) {
        WriteWarningLog("接続対象のGameObjectの名前を入力してください");
        return;
    }

    SocketConnectGameObjectRequestMessage message(ui->gameObjectName->text().toUtf8().data());
    SendMessage(message);
    _manipulateTarget = message.RequestId();
}

void ConnectionDialog::on_upButton_clicked()
{
    SocketMoveGameObjectMessage message(_manipulateTarget, 0, 0.1f, 0);
    SendMessage(message);
}

void ConnectionDialog::on_downButton_clicked()
{
    SocketMoveGameObjectMessage message(_manipulateTarget, 0, -0.1f, 0);
    SendMessage(message);
}

void ConnectionDialog::on_leftButton_clicked()
{
    SocketMoveGameObjectMessage message(_manipulateTarget, -0.1f, 0, 0);
    SendMessage(message);
}

void ConnectionDialog::on_rightButton_clicked()
{
    SocketMoveGameObjectMessage message(_manipulateTarget, 0.1f, 0, 0);
    SendMessage(message);
}
