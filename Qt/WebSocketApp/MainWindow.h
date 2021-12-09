#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "WebSocketApp.h"
#include <QMainWindow>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ConnectionDialog;
class SocketConnectionInformationMessage;

class MainWindow : public QMainWindow, WebSocketApp::ILogReceiver
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static const SocketConnectionInformationMessage* GetConnectionInformation();
    void CloseConnection(ConnectionDialog* dialog);

private slots:
    void on_ConnectButton_clicked();
    void on_ClearButton_clicked();

private:
    Ui::MainWindow *ui;

    std::vector<ConnectionDialog*> _connections;

    ConnectionDialog* GetConnection(const std::string& address, ushort port);
    void ReceiveLog(const std::string& log) override;
    void ReceiveColorLog(const std::string& log, const QColor& color) override;
    void ReceiveWarningLog(const std::string& log) override;
    void ReceiveErrorLog(const std::string& log) override;
    void detachLogReceiver() override;

    void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
