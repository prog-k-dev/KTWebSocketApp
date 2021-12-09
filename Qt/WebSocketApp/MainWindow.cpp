#include "MainWindow.h"
#include "ConnectionDialog.h"
#include "ui_MainWindow.h"

#include <QHostInfo>
#include <QSysInfo>

namespace {
    SocketConnectionInformationMessage MyInformation;
}

//---------------------------------

const SocketConnectionInformationMessage* MainWindow::GetConnectionInformation() {
    return &MyInformation;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->port->setValue(WebSocketApp::SOCKET_PORT_DEFAULT);
    ui->log->setTextColor(WebSocketApp::LOG_NORMAL_COLOR);

    WebSocketApp::AddLogReceiver(this);

    MyInformation.SetApplicationName(QApplication::applicationName().toUtf8().data());
    MyInformation.SetDeviceName(QSysInfo::machineHostName().toUtf8().data());
    MyInformation.SetDeviceModel(QSysInfo::prettyProductName().toUtf8().data());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::CloseConnection(ConnectionDialog* dialog) {
    auto it = std::find(_connections.begin(), _connections.end(), dialog);
    if (it == _connections.end()) {
        return;
    }

    _connections.erase(it);
}

void MainWindow::closeEvent(QCloseEvent* /*event*/) {
    for (auto it : _connections) {
        it->Close();
    }
    _connections.clear();

    WebSocketApp::RemoveLogReceiver(this);
}

ConnectionDialog* MainWindow::GetConnection(const std::string& address, ushort port) {
    for (auto connection : _connections) {
        if (connection->Address() == address && connection->Port() == port) {
            return connection;
        }
    }
    return nullptr;
}

void MainWindow::ReceiveLog(const std::string& log) {
    if (log.empty()) {
        return;
    }

    const QString& qstr = log.c_str();

    bool end = ui->log->textCursor().atEnd();
    if (qstr.at(qstr.length() - 1) != '\n') {
        ui->log->append(qstr);
    } else {
        auto index = qstr.indexOf("\n");
        ui->log->append(qstr.mid(0, index));
    }
    if (end) {
        ui->log->moveCursor(QTextCursor::MoveOperation::End, QTextCursor::MoveAnchor);
    }

#if defined(QT_DEBUG)
    qDebug() << qstr;
#endif
}

void MainWindow::ReceiveColorLog(const std::string& log, const QColor& color) {
    ui->log->setTextColor(color);
    ReceiveLog(log);
    ui->log->setTextColor(WebSocketApp::LOG_NORMAL_COLOR);
}

void MainWindow::ReceiveWarningLog(const std::string& log) {
    ReceiveColorLog(log, WebSocketApp::LOG_WARNING_COLOR);
}

void MainWindow::ReceiveErrorLog(const std::string& log) {
    ReceiveColorLog(log, WebSocketApp::LOG_ERROR_COLOR);
}

void MainWindow::detachLogReceiver() {
}

void MainWindow::on_ConnectButton_clicked()
{
    const std::string address = ui->address->text().toUtf8().data();
    ushort port = (ushort)ui->port->value();
    auto connection = GetConnection(address, port);
    if (connection != nullptr) {
        connection->raise();
        connection->activateWindow();
    } else {
        connection = new ConnectionDialog(address, port, this);
        connection->show();
        _connections.push_back(connection);
    }
}

void MainWindow::on_ClearButton_clicked()
{
    ui->log->clear();
}

