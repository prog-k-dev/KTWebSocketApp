#ifndef SOCKETMESSAGE_H
#define SOCKETMESSAGE_H

#include "WebSocketApp.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#define JSON_ARG(var) #var, (var)
#define GET_JSON_VALUE(var, obj) GetJsonValue(JSON_ARG(var), obj)
#define GET_JSON_INT_VALUE(type, var, obj) GetJsonIntValue<type>(JSON_ARG(var), obj)

#define SET_JSON_VALUE(var, obj) SetJsonValue(JSON_ARG(var), obj)
#define SET_JSON_INT_VALUE(var, obj) SetJsonValue(#var, (int)(var), obj)

enum class UnityDirectoryType : int
{
    Invalid = -1,

    Data,
    StreamingAssets,
    PersistentData,
    TemporaryCache,
}; // enum UnityDirectoryType

//---------------------------------

class SocketMessageBase
{
public:
    SocketMessageBase(const char* messageType)
        : _messageType(messageType)
    {
    }

    virtual ~SocketMessageBase() {
    }

    const std::string& MessageType() const {
        return _messageType;
    }

    static SocketMessageBase* ImportMessage(const QString& val);
    bool ExportMessage(QString& message) const;

protected:
    std::string _messageType;

    virtual bool FromJson(QJsonObject& obj);
    virtual bool ToJson(QJsonObject& obj) const;

    bool GetJsonValue(const char* key, bool& value, QJsonObject& obj) {
        auto json = obj[key];
        if (!json.isBool()) {
            return false;
        }
        value = json.toBool();
        return true;;
    }

    bool GetJsonValue(const char* key, float& value, QJsonObject& obj) {
        auto json = obj[key];
        if (!json.isDouble()) {
            return false;
        }
        value = static_cast<float>(json.toDouble());
        return true;;
    }

    bool GetJsonValue(const char* key, double& value, QJsonObject& obj) {
        auto json = obj[key];
        if (!json.isDouble()) {
            return false;
        }
        value = json.toDouble();
        return true;;
    }

    bool GetJsonValue(const char* key, int& value, QJsonObject& obj) {
        auto json = obj[key];
        if (!json.isDouble()) {
            return false;
        }
        value = json.toInt();
        return true;;
    }

    template<class T>
    bool GetJsonIntValue(const char* key, T& value, QJsonObject& obj) {
        auto json = obj[key];
        if (!json.isDouble()) {
            return false;
        }
        value = static_cast<T>(json.toInt());
        return true;;
    }

    bool GetJsonValue(const char* key, std::string& value, QJsonObject& obj) {
        auto json = obj[key];
        if (!json.isString()) {
            return false;
        }
        value = json.toString().toUtf8().data();
        return true;;
    }

    void SetJsonValue(const char* key, bool value, QJsonObject& obj) const {
        obj[key] = value;
    }
    void SetJsonValue(const char* key, float value, QJsonObject& obj) const {
        obj[key] = static_cast<double>(value);
    }
    void SetJsonValue(const char* key, double value, QJsonObject& obj) const {
        obj[key] = value;
    }
    void SetJsonValue(const char* key, int value, QJsonObject& obj) const {
        obj[key] = value;
    }
    void SetJsonValue(const char* key, int64_t value, QJsonObject& obj) const {
        obj[key] = value;
    }
    void SetJsonValue(const char* key, const std::string& value, QJsonObject& obj) const {
        obj[key] = value.c_str();
    }
}; // class SocketMessageBase

//---------------------------------

class SockeTextMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    SockeTextMessage(const std::string& text)
        : SocketMessageBase(MESSAGE_TYPE)
        , _text(text)
    {
    }

private:
    std::string _text;

    bool ToJson(QJsonObject& obj) const override;
}; // class SockeTextMessage

//---------------------------------

class SocketRequestMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    SocketRequestMessage(const std::string& request)
        : SocketMessageBase(MESSAGE_TYPE)
        , _request(request)
        , _requestId(_nextRequestId++)
    {
    }

    const std::string& Request() const {
        return _request;
    }

    int RequestId() const {
        return _requestId;
    }

    void SetRequest(const std::string& val) {
        _request = val;
    }

    void SetRequestId(int val) {
        _requestId = val;
    }

protected:
    SocketRequestMessage(const char* messageType, const std::string& request)
        : SocketMessageBase(messageType)
        , _request(request)
        , _requestId(_nextRequestId++)
    {
    }

    bool ToJson(QJsonObject& obj) const override;

private:
    static int _nextRequestId;

    std::string _request;
    int _requestId;
}; // class SocketRequestMessage

//---------------------------------

class SocketResponseMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    SocketResponseMessage()
        : SocketMessageBase(MESSAGE_TYPE)
        , _requestId(-1)
    {
    }

    const std::string& Request() const {
        return _request;
    }

    int RequestId() const {
        return _requestId;
    }

    void SetRequest(const std::string& val) {
        _request = val;
    }

    void SetRequestId(int val) {
        _requestId = val;
    }

protected:
    SocketResponseMessage(const char* messageType, const std::string& request)
        : SocketMessageBase(messageType)
        , _request(request)
    {
    }

    bool FromJson(QJsonObject& obj) override;
    bool ToJson(QJsonObject& obj) const override;

private:
    std::string _request;
    int _requestId;
}; // class SocketResponseMessage

//---------------------------------

class SocketScreenShotRequestMessage : public SocketRequestMessage {
public:
    static const char* MESSAGE_TYPE;

    SocketScreenShotRequestMessage(bool stop)
        : SocketRequestMessage(MESSAGE_TYPE, MESSAGE_TYPE)
        , _stop(stop)
        , _interval(-1)
    {
    }

    SocketScreenShotRequestMessage(float interval)
        : SocketRequestMessage(MESSAGE_TYPE, MESSAGE_TYPE)
        , _stop(false)
        , _interval(interval)
    {
    }

private:
    bool _stop;
    float _interval;

    bool ToJson(QJsonObject& obj) const override;
}; // class SocketScreenShotRequestMessage

//---------------------------------

class SocketFileListRequestMessage : public SocketRequestMessage {
public:
    static const char* MESSAGE_TYPE;

    SocketFileListRequestMessage(UnityDirectoryType directoryType, const std::string& targetPath)
        : SocketRequestMessage(MESSAGE_TYPE, MESSAGE_TYPE)
        , _directoryType(directoryType)
        , _targetPath(targetPath)
    {
    }

    UnityDirectoryType DirectoryType() const {
        return _directoryType;
    }

    const std::string& TargetPath() const {
        return _targetPath;
    }

    void SetDirectoryType(UnityDirectoryType val) {
        _directoryType = val;
    }

    void SetTargetPath(const std::string& val) {
        _targetPath = val;
    }

private:
    UnityDirectoryType _directoryType;
    std::string _targetPath;

    bool ToJson(QJsonObject& obj) const override;
}; // class SocketFileListRequestMessage

//---------------------------------

class SocketFileUploadRequestMessage : public SocketRequestMessage {
public:
    static const char* MESSAGE_TYPE;

    SocketFileUploadRequestMessage(UnityDirectoryType directoryType, const std::string& targetPath)
        : SocketRequestMessage(MESSAGE_TYPE, MESSAGE_TYPE)
        , _directoryType(directoryType)
        , _targetPath(targetPath)
    {
    }

    UnityDirectoryType DirectoryType() const {
        return _directoryType;
    }

    const std::string& TargetPath() const {
        return _targetPath;
    }

    void SetDirectoryType(UnityDirectoryType val) {
        _directoryType = val;
    }

    void SetTargetPath(const std::string& val) {
        _targetPath = val;
    }

private:
    UnityDirectoryType _directoryType;
    std::string _targetPath;

    bool ToJson(QJsonObject& obj) const override;
}; // class SocketFileUploadRequestMessage

//---------------------------------

class SocketConnectGameObjectRequestMessage : public SocketRequestMessage {
public:
    static const char* MESSAGE_TYPE;

    SocketConnectGameObjectRequestMessage(const std::string& gameObjectName)
        : SocketRequestMessage(MESSAGE_TYPE, MESSAGE_TYPE)
        , _gameObjectName(gameObjectName)
    {
    }

    const std::string& GameObjectName() const {
        return _gameObjectName;
    }

    void SetGameObjectName(const std::string& val) {
        _gameObjectName = val;
    }

private:
    std::string _gameObjectName;

    bool ToJson(QJsonObject& obj) const override;
}; // class SocketConnectGameObjectRequestMessage

//---------------------------------

class SocketLogMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    enum class LogType : int
    {
        Invalid = -1,

        Error,
        Assert,
        Warning,
        Log,
        Exception
    };

    SocketLogMessage()
        : SocketMessageBase(MESSAGE_TYPE)
        , _logType(LogType::Invalid)
    {
    }

    LogType Type() const {
        return _logType;
    }
    const std::string& Log() const {
        return _log;
    }
    const std::string& Time() const {
        return _time;
    }
    const std::string& StackTrace() const {
        return _stackTrace;
    }

    void SetLogType(LogType val) {
        _logType = val;
    }
    void SetTime(const std::string& val) {
        _time = val;
    }
    void SetLog(const std::string& val) {
        _log = val;
    }
    void SetStackTrace(const std::string& val) {
        _stackTrace = val;
    }

private:
    LogType _logType;
    std::string _time;
    std::string _log;
    std::string _stackTrace;

    bool FromJson(QJsonObject& obj) override;
    bool ToJson(QJsonObject& obj) const override;
}; // class SocketLogMessage

//---------------------------------

class SocketConnectionInformationMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    SocketConnectionInformationMessage() : SocketMessageBase(MESSAGE_TYPE) {
    }

    const std::string& ApplicationName() const {
        return _applicationName;
    }
    const std::string& UUID() const {
        return _uuid;
    }
    const std::string& DeviceName() const {
        return _deviceName;
    }
    const std::string& DeviceModel() const {
        return _deviceModel;
    }

    void SetApplicationName(const std::string& val) {
        _applicationName = val;
    }
    void SetUUID(const std::string& val) {
        _uuid = val;
    }
    void SetDeviceName(const std::string& val) {
        _deviceName = val;
    }
    void SetDeviceModel(const std::string& val) {
        _deviceModel = val;
    }

private:
    int _requestId;
    std::string _applicationName;
    std::string _uuid;
    std::string _deviceName;
    std::string _deviceModel;

    bool FromJson(QJsonObject& obj) override;
    bool ToJson(QJsonObject& obj) const override;
}; // class SocketConnectionInformationMessage

//---------------------------------

class SocketFileListMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    SocketFileListMessage() : SocketMessageBase(MESSAGE_TYPE) {
    }

    const std::string& Directory() const {
        return _directory;
    }

    const std::vector<std::string>& Files() const {
        return _files;
    }

    const std::vector<std::string>& Directories() const {
        return _directories;
    }

private:
    int _requestId;
    std::string _directory;
    std::vector<std::string> _files;
    std::vector<std::string> _directories;

    bool FromJson(QJsonObject& obj) override;
    bool ImportFiles(QJsonObject& obj);
    bool ImportDirectories(QJsonObject& obj);
}; // class SocketFileListMessage

//---------------------------------

class SocketFileMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    SocketFileMessage() : SocketMessageBase(MESSAGE_TYPE) {
    }
    SocketFileMessage(const char* messageType) : SocketMessageBase(messageType) {
    }

    int RequestId() const {
        return _requestId;
    }

    UnityDirectoryType DirectoryType() const {
        return _directoryType;
    }

    const std::string& TargetPath() const {
        return _targetPath;
    }

    const QByteArray& Bytes() const {
        return _bytes;
    }

    void SetDirectoryType(UnityDirectoryType val) {
        _directoryType = val;
    }

    void SetTargetPath(const std::string& val) {
        _targetPath = val;
    }

    bool SetFile(const std::string& dataPath, UnityDirectoryType directoryType = UnityDirectoryType::Invalid);

protected:
    bool FromJson(QJsonObject& obj) override;
    bool ToJson(QJsonObject& obj) const override;

private:
    int _requestId;

    UnityDirectoryType _directoryType;
    std::string _targetPath;

    std::string _data;
    QByteArray _bytes;
}; // class SocketFileMessage

//---------------------------------

class SocketImageDataMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    SocketImageDataMessage() : SocketMessageBase(MESSAGE_TYPE) {
    }
    SocketImageDataMessage(const char* messageType) : SocketMessageBase(messageType) {
    }

    const QByteArray& Bytes() const {
        return _bytes;
    }

    bool SetImage(const std::string& imagePath);

protected:
    bool FromJson(QJsonObject& obj) override;
    bool ToJson(QJsonObject& obj) const override;

private:
    std::string _imageData;
    QByteArray _bytes;
}; // class SocketImageDataMessage

//---------------------------------

class SocketScreenShotMessage : public SocketImageDataMessage {
public:
    static const char* MESSAGE_TYPE;

    SocketScreenShotMessage() : SocketImageDataMessage(MESSAGE_TYPE) {
    }

    const std::string& DateTime() const {
        return _dateTime;
    }

    void SetDateTime(const std::string& val) {
        _dateTime = val;
    }

private:
    int _requestId;
    std::string _dateTime;

    bool FromJson(QJsonObject& obj) override;
}; // class SocketScreenShotMessage

//---------------------------------

class SocketMoveGameObjectMessage : public SocketMessageBase {
public:
    static const char* MESSAGE_TYPE;

    SocketMoveGameObjectMessage(int manipulateTarget, float x = 0, float y = 0, float z = 0)
        : SocketMessageBase(MESSAGE_TYPE)
        , _manipulateTarget(manipulateTarget)
        , _x(x)
        , _y(y)
        , _z(z)
    {
    }

private:
    int _manipulateTarget;
    float _x;
    float _y;
    float _z;

    bool FromJson(QJsonObject& obj) override;
    bool ToJson(QJsonObject& obj) const override;
}; // class SocketMoveGameObjectMessage

#endif // SOCKETMESSAGE_H
