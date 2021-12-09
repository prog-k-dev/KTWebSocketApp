#include "SocketMessage.h"

const char* SockeTextMessage::MESSAGE_TYPE = "SockeTextMessage";
const char* SocketConnectionInformationMessage::MESSAGE_TYPE = "SocketConnectionInformationMessage";
const char* SocketMoveGameObjectMessage::MESSAGE_TYPE = "SocketMoveGameObjectMessage";
const char* SocketRequestMessage::MESSAGE_TYPE = "SocketRequestMessage";
const char* SocketResponseMessage::MESSAGE_TYPE = "SocketResponseMessage";
const char* SocketScreenShotRequestMessage::MESSAGE_TYPE = "SocketScreenShotRequestMessage";
const char* SocketFileListRequestMessage::MESSAGE_TYPE = "SocketFileListRequestMessage";
const char* SocketFileUploadRequestMessage::MESSAGE_TYPE = "SocketFileUploadRequestMessage";
const char* SocketConnectGameObjectRequestMessage::MESSAGE_TYPE = "SocketConnectGameObjectRequestMessage";
const char* SocketLogMessage::MESSAGE_TYPE = "SocketLogMessage";
const char* SocketFileListMessage::MESSAGE_TYPE = "SocketFileListMessage";
const char* SocketFileMessage::MESSAGE_TYPE = "SocketFileMessage";
const char* SocketImageDataMessage::MESSAGE_TYPE = "SocketImageDataMessage";
const char* SocketScreenShotMessage::MESSAGE_TYPE = "SocketScreenShotMessage";


SocketMessageBase* SocketMessageBase::ImportMessage(const QString& val) {
    auto index = val.indexOf(',');
    if (index == -1) {
        return nullptr;
    }

    const auto& typeKey = val.mid(0, index);
    if (index+1 >= val.length()) {
        return nullptr;
    }
    ++index;

    bool compressed = (val[index] == 'c');
    index += 2;

    QByteArray decodedArray;
    {
        const auto& base64Encoded = val.mid(index);
        if (!compressed) {
            decodedArray = QByteArray::fromBase64(base64Encoded.toUtf8());
        } else {
            auto compressedArray = QByteArray::fromBase64(base64Encoded.toUtf8());

            std::vector<char> decompressed;
            auto decompressedLength = WebSocketApp::DecompressGZip(compressedArray.data(), compressedArray.length(), decompressed);
            if (decompressedLength == -1) {
                OUTPUT_ERROR_LOG("%sの解凍に失敗", typeKey.toUtf8().data());
                return nullptr;
            }
            decodedArray = QByteArray(&(decompressed[0]), decompressed.size());
        }
    }
    QJsonDocument document = QJsonDocument::fromJson(decodedArray);

    SocketMessageBase* message = nullptr;
    {
        if (typeKey == SocketLogMessage::MESSAGE_TYPE) {
            message = new SocketLogMessage();
        } else if (typeKey == SocketConnectionInformationMessage::MESSAGE_TYPE) {
            message = new SocketConnectionInformationMessage();
        } else if (typeKey == SocketFileListMessage::MESSAGE_TYPE) {
            message = new SocketFileListMessage();
        } else if (typeKey == SocketFileMessage::MESSAGE_TYPE) {
            message = new SocketFileMessage();
        } else if (typeKey == SocketScreenShotMessage::MESSAGE_TYPE) {
            message = new SocketScreenShotMessage();
        }
    }
    if (message == nullptr) {
        return nullptr;
    }

    auto obj = document.object();
    if (!message->FromJson(obj)) {
        delete message;
        return nullptr;
    }

    return message;
}

bool SocketMessageBase::ExportMessage(QString& message) const {
    QJsonObject obj;
    if (!ToJson(obj)) {
        return false;
    }

    message = QFORMAT_STR("%s,", _messageType.c_str());

    if (!obj.empty()) {
        QJsonDocument document(obj);

        QByteArray array = document.toJson(QJsonDocument::Compact);
        {
            std::vector<char> compressed;
            if (WebSocketApp::CompressGZip(array.data(), array.length(), compressed) <= 0) {
                message += "-,";
            } else if ((int)compressed.size() >= array.length()) {
                message += "-,";
            } else {
                message += "c,";
                array = QByteArray(&(compressed[0]), compressed.size());
            }
        }
        message += array.toBase64();
    }
    return true;
}

bool SocketMessageBase::ToJson(QJsonObject& obj) const {
    SET_JSON_VALUE(_messageType, obj);
    return true;
}

bool SocketMessageBase::FromJson(QJsonObject& obj) {
    if (!GET_JSON_VALUE(_messageType, obj)) {
        return false;
    }
    return true;
}

//---------------------------------

bool SockeTextMessage::ToJson(QJsonObject& obj) const {
    if (!SocketMessageBase::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_text, obj);

    return true;
}

//---------------------------------

int SocketRequestMessage::_nextRequestId = 0;

bool SocketRequestMessage::ToJson(QJsonObject& obj) const {
    if (!SocketMessageBase::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_request, obj);
    SET_JSON_VALUE(_requestId, obj);

    return true;
}

//---------------------------------

bool SocketScreenShotRequestMessage::ToJson(QJsonObject& obj) const {
    if (!SocketRequestMessage::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_stop, obj);
    SET_JSON_VALUE(_interval, obj);
    return true;
}

//---------------------------------

bool SocketFileListRequestMessage::ToJson(QJsonObject& obj) const {
    if (!SocketRequestMessage::ToJson(obj)) {
        return false;;
    }

    SET_JSON_INT_VALUE(_directoryType, obj);
    SET_JSON_VALUE(_targetPath, obj);
    return true;
}

//---------------------------------

bool SocketFileUploadRequestMessage::ToJson(QJsonObject& obj) const {
    if (!SocketRequestMessage::ToJson(obj)) {
        return false;;
    }

    SET_JSON_INT_VALUE(_directoryType, obj);
    SET_JSON_VALUE(_targetPath, obj);
    return true;
}

//---------------------------------

bool SocketConnectGameObjectRequestMessage::ToJson(QJsonObject& obj) const {
    if (!SocketRequestMessage::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_gameObjectName, obj);
    return true;
}

//---------------------------------

bool SocketResponseMessage::ToJson(QJsonObject& obj) const {
    if (!SocketMessageBase::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_request, obj);
    SET_JSON_VALUE(_requestId, obj);

    return true;
}

bool SocketResponseMessage::FromJson(QJsonObject& obj) {
    if (!SocketMessageBase::FromJson(obj)) {
        return false;;
    }

    if (!GET_JSON_VALUE(_request, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_requestId, obj)) {
        return false;
    }

    return true;
}

//---------------------------------

bool SocketLogMessage::FromJson(QJsonObject& obj) {
    if (!SocketMessageBase::FromJson(obj)) {
        return false;;
    }

    if (!GET_JSON_INT_VALUE(LogType, _logType, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_time, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_log, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_stackTrace, obj)) {
        return false;
    }

    return true;
}

bool SocketLogMessage::ToJson(QJsonObject& obj) const {
    if (!SocketMessageBase::ToJson(obj)) {
        return false;;
    }

    SET_JSON_INT_VALUE(_logType, obj);
    SET_JSON_VALUE(_log, obj);
    SET_JSON_VALUE(_stackTrace, obj);
    return true;
}

//---------------------------------

bool SocketConnectionInformationMessage::FromJson(QJsonObject& obj) {
    if (!SocketMessageBase::FromJson(obj)) {
        return false;;
    }

    if (!GET_JSON_VALUE(_requestId, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_applicationName, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_uuid, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_deviceName, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_deviceModel, obj)) {
        return false;
    }

    return true;
}

bool SocketConnectionInformationMessage::ToJson(QJsonObject& obj) const {
    if (!SocketMessageBase::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_applicationName, obj);
    SET_JSON_VALUE(_uuid, obj);
    SET_JSON_VALUE(_deviceName, obj);
    SET_JSON_VALUE(_deviceModel, obj);
    return true;
}

//---------------------------------

bool SocketFileListMessage::FromJson(QJsonObject& obj) {
    if (!SocketMessageBase::FromJson(obj)) {
        return false;;
    }

    if (!GET_JSON_VALUE(_requestId, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_directory, obj)) {
        return false;
    }
    if (!ImportFiles(obj)) {
        return false;;
    }
    if (!ImportDirectories(obj)) {
        return false;;
    }

    return true;
}

bool SocketFileListMessage::ImportFiles(QJsonObject& obj) {
    _files.clear();

    QJsonValueRef value = obj["_files"];
    if (!value.isArray()) {
        return false;
    }

    auto files = value.toArray();
    for (int i = 0, count = files.count(); i < count; ++i) {
        _files.push_back(files[i].toString().toUtf8().data());
    }

    return true;
}

bool SocketFileListMessage::ImportDirectories(QJsonObject& obj) {
    _directories.clear();

    QJsonValueRef value = obj["_directories"];
    if (!value.isArray()) {
        return false;
    }

    auto directories = value.toArray();
    for (int i = 0, count = directories.count(); i < count; ++i) {
        _directories.push_back(directories[i].toString().toUtf8().data());
    }

    return true;
}

//---------------------------------

bool SocketImageDataMessage::SetImage(const std::string& imagePath) {
    std::vector<char> buffer;
    if (!WebSocketApp::ReadFile(imagePath, buffer)) {
        return false;
    }

    QByteArray array(&(buffer[0]), buffer.size());
    _imageData = array.toBase64().data();

    return true;
}

bool SocketImageDataMessage::FromJson(QJsonObject& obj) {
    if (!SocketMessageBase::FromJson(obj)) {
        return false;;
    }

    if (!GET_JSON_VALUE(_imageData, obj)) {
        return false;
    }
    _bytes = QByteArray::fromBase64(QByteArray(_imageData.c_str(), _imageData.size()));

    return true;
}

bool SocketImageDataMessage::ToJson(QJsonObject& obj) const {
    if (!SocketMessageBase::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_imageData, obj);
    return true;
}

//---------------------------------

bool SocketFileMessage::SetFile(const std::string& dataPath, UnityDirectoryType directoryType /*= UnityDirectoryType::Invalid*/) {
    std::vector<char> buffer;
    if (!WebSocketApp::ReadFile(dataPath, buffer)) {
        return false;
    }

    QByteArray array(&(buffer[0]), buffer.size());
    _data = array.toBase64().data();

    _directoryType = directoryType;
    _targetPath = dataPath;

    return true;
}

bool SocketFileMessage::FromJson(QJsonObject& obj) {
    if (!SocketMessageBase::FromJson(obj)) {
        return false;;
    }

    if (!GET_JSON_VALUE(_requestId, obj)) {
        return false;
    }
    if (!GET_JSON_INT_VALUE(UnityDirectoryType, _directoryType, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_targetPath, obj)) {
        return false;
    }

    if (!GET_JSON_VALUE(_data, obj)) {
        return false;
    }
    _bytes = QByteArray::fromBase64(QByteArray(_data.c_str(), _data.size()));

    return true;
}

bool SocketFileMessage::ToJson(QJsonObject& obj) const {
    if (!SocketMessageBase::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_requestId, obj);
    SET_JSON_INT_VALUE(_directoryType, obj);
    SET_JSON_VALUE(_targetPath, obj);
    SET_JSON_VALUE(_data, obj);
    return true;
}

//---------------------------------

bool SocketScreenShotMessage::FromJson(QJsonObject& obj) {
    if (!SocketImageDataMessage::FromJson(obj)) {
        return false;;
    }

    if (!GET_JSON_VALUE(_requestId, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_dateTime, obj)) {
        return false;
    }

    return true;
}

//---------------------------------

bool SocketMoveGameObjectMessage::FromJson(QJsonObject& obj) {
    if (!SocketMessageBase::FromJson(obj)) {
        return false;;
    }

    if (!GET_JSON_VALUE(_manipulateTarget, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_x, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_y, obj)) {
        return false;
    }
    if (!GET_JSON_VALUE(_z, obj)) {
        return false;
    }

    return true;
}

bool SocketMoveGameObjectMessage::ToJson(QJsonObject& obj) const {
    if (!SocketMessageBase::ToJson(obj)) {
        return false;;
    }

    SET_JSON_VALUE(_manipulateTarget, obj);
    SET_JSON_VALUE(_x, obj);
    SET_JSON_VALUE(_y, obj);
    SET_JSON_VALUE(_z, obj);
    return true;
}
