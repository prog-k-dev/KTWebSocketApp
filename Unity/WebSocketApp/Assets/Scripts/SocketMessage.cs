using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using UnityEngine;

namespace WebSocketApp
{
    public enum UnityDirectoryType
    {
        Invalid = -1,

        Data,
        StreamingAssets,
        PersistentData,
        TemporaryCache,
    } // enum UnityDirectoryType

    //---------------------------------

    [Serializable]
    public class SocketMessageBase
    {
        private static readonly Dictionary<string, Type> MESSAGE_TYPES = new Dictionary<string, Type>()
        {
            {typeof(SockeTextMessage).Name,  typeof(SockeTextMessage)},
            {typeof(SocketRequestMessage).Name,  typeof(SocketRequestMessage)},
            {typeof(SocketConnectionInformationMessage).Name,  typeof(SocketConnectionInformationMessage)},
            {typeof(SocketScreenShotRequestMessage).Name,  typeof(SocketScreenShotRequestMessage)},
            {typeof(SocketFileListRequestMessage).Name,  typeof(SocketFileListRequestMessage)},
            {typeof(SocketFileUploadRequestMessage).Name,  typeof(SocketFileUploadRequestMessage)},
            {typeof(SocketConnectGameObjectRequestMessage).Name,  typeof(SocketConnectGameObjectRequestMessage)},
            {typeof(SocketFileMessage).Name,  typeof(SocketFileMessage)},
            {typeof(SocketScreenShotMessage).Name,  typeof(SocketScreenShotMessage)},
            {typeof(SocketMoveGameObjectMessage).Name,  typeof(SocketMoveGameObjectMessage)},
            {typeof(SocketImageDataMessage).Name,  typeof(SocketImageDataMessage)},
        };

        public string MessageType
        {
            get => _messageType;
        }
        public string _messageType;

        private static byte[] _base64Buffer = new byte[1024];


        public SocketMessageBase()
        {
            _messageType = GetType().Name;
        }

        public static SocketMessageBase ImportMessage(string val)
        {
            var index = val.IndexOf(',');
            if (index < 0)
            {
                Debug.LogErrorFormat("メッセージの書式が不正：{0}", val);
                return null;
            }

            var typeKey = val.Substring(0, index);
            if (!MESSAGE_TYPES.TryGetValue(typeKey, out var type))
            {
                Debug.LogErrorFormat("未対応のメッセージタイプ：{0}", typeKey);
                return null;
            }
            ++index;

            bool compressed = (val[index] == 'c');
            index += 2;

            SocketMessageBase message = null;
            try
            {
                var json = DecodeBase64(val.Substring(index), compressed);
                message = JsonUtility.FromJson(json, type) as SocketMessageBase;
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
                return null;
            }

            return message;
        }

        public static string ExportMessage(SocketMessageBase message)
        {
            string result = message.MessageType + ",";

            try
            {
                var json = JsonUtility.ToJson(message);

                if (string.IsNullOrEmpty(json))
                {
                    result += "-,";
                } else
                {
                    var length = System.Text.Encoding.UTF8.GetByteCount(json);
                    var jsonBuffer = GetEncodeBuffer(length);
                    var jsonBufferLength = System.Text.Encoding.UTF8.GetBytes(json, 0, json.Length, jsonBuffer, 0);
                    var compressedArray = Compress(jsonBuffer, 0, jsonBufferLength);
                    if (compressedArray == null || compressedArray.Length >= jsonBufferLength)
                    {
                        result += "-,";
                        result += Convert.ToBase64String(jsonBuffer, 0, jsonBufferLength);
                    }
                    else
                    {
                        result += "c,";
                        result += Convert.ToBase64String(compressedArray, 0, compressedArray.Length);
                    }
                }
            }
            catch(Exception ex)
            {
                Debug.LogException(ex);
                return "";
            }

            return result;
        }

        public static string GetDirectory(UnityDirectoryType directoryType)
        {
            switch (directoryType)
            {
                case UnityDirectoryType.Data:
                    return Application.dataPath;
                case UnityDirectoryType.StreamingAssets:
                    return Application.streamingAssetsPath;
                case UnityDirectoryType.PersistentData:
                    return Application.persistentDataPath;
                case UnityDirectoryType.TemporaryCache:
                    return Application.temporaryCachePath;

                default:
                    break;
            }
            return "";
        }

        public static byte[] Compress(byte[] src, int srcOffset, int srcLength)
        {
            using (MemoryStream outMemoryStream = new MemoryStream())
            {
                using (GZipStream gzipStream = new GZipStream(outMemoryStream, System.IO.Compression.CompressionLevel.Fastest))
                {
                    gzipStream.Write(src, srcOffset, srcLength);
                }
                return outMemoryStream.ToArray();
            }
        }

        public static byte[] Decompress(byte[] bytes)
        {
            using (MemoryStream inStream = new MemoryStream(bytes))
            {
                using (MemoryStream outStream = Decompress(inStream))
                {
                    if (outStream != null)
                    {
                        return outStream.ToArray();
                    }
                }
            }
            return null;
        }

        public static MemoryStream Decompress(Stream inStream)
        {
            MemoryStream outStream = new MemoryStream(capacity: (int)inStream.Length);
            using (GZipStream gzipStream = new GZipStream(inStream, CompressionMode.Decompress))
            {
                int length = 1024;
                byte[] buffer = new byte[length];
                while ((length = gzipStream.Read(buffer, 0, buffer.Length)) > 0)
                {
                    outStream.Write(buffer, 0, length);
                }
                outStream.Position = 0;
            }
            return outStream;
        }
        public static string EncodeBase64(string src, out bool compressed)
        {
            compressed = false;
            if (string.IsNullOrEmpty(src))
            {
                return "";
            }

            var length = System.Text.Encoding.UTF8.GetByteCount(src);
            var buffer = GetEncodeBuffer(length);
            var bufferLength = System.Text.Encoding.UTF8.GetBytes(src, 0, src.Length, buffer, 0);
            return Convert.ToBase64String(buffer, 0, bufferLength);
        }

        public static string DecodeBase64(string base64Encoded, bool compressed = false)
        {
            char[] encodedArray = base64Encoded.ToCharArray();
            var decodedArray = Convert.FromBase64CharArray(encodedArray, 0, encodedArray.Length);
            if (!compressed)
            {
                return System.Text.Encoding.UTF8.GetString(decodedArray);
            }
            else
            {
                var compressedArray = Decompress(decodedArray);
                if (compressedArray == null)
                {
                    return "";
                }
                return System.Text.Encoding.UTF8.GetString(compressedArray);
            }
        }

        private static byte[] GetEncodeBuffer(int length)
        {
            return (length <= _base64Buffer.Length ? _base64Buffer : (_base64Buffer = new byte[length]));
        }
    } // class SocketMessageBase

    //---------------------------------

    [Serializable]
    public class SockeTextMessage : SocketMessageBase
    {
        public static readonly string MESSAGE_TYPE = typeof(SockeTextMessage).Name;

        public string _text;
    } // class SockeTextMessage

    //---------------------------------

    [Serializable]
    public class SocketRequestMessage : SocketMessageBase
    {
        public static readonly string MESSAGE_TYPE = typeof(SocketRequestMessage).Name;

        public string _request;
        public int _requestId;
    } // class SocketRequestMessage

    //---------------------------------

    [Serializable]
    public class SocketScreenShotRequestMessage : SocketRequestMessage
    {
        public static readonly new string MESSAGE_TYPE = typeof(SocketScreenShotRequestMessage).Name;

        public bool _stop = false;
        public float _interval = -1f;
    } // class SocketScreenShotRequestMessage

    //---------------------------------

    [Serializable]
    public class SocketFileListRequestMessage : SocketRequestMessage
    {
        public static readonly new string MESSAGE_TYPE = typeof(SocketFileListRequestMessage).Name;

        public UnityDirectoryType _directoryType = UnityDirectoryType.Invalid;
        public string _targetPath;
    } // class SocketFileListRequestMessage

    //---------------------------------

    [Serializable]
    public class SocketFileUploadRequestMessage : SocketRequestMessage
    {
        public static readonly new string MESSAGE_TYPE = typeof(SocketFileUploadRequestMessage).Name;

        public UnityDirectoryType _directoryType = UnityDirectoryType.Invalid;
        public string _targetPath;
    } // class SocketFileUploadRequestMessage

    //---------------------------------

    [Serializable]
    public class SocketConnectGameObjectRequestMessage : SocketRequestMessage
    {
        public static readonly new string MESSAGE_TYPE = typeof(SocketConnectGameObjectRequestMessage).Name;

        public string _gameObjectName;
    } // class SocketConnectGameObjectRequestMessage

    //---------------------------------

    [Serializable]
    public class SocketLogMessage : SocketMessageBase
    {
        public static readonly string MESSAGE_TYPE = typeof(SocketLogMessage).Name;

        public SocketLogMessage(LogType logType, string log, string stackTrace)
        {
            var now = DateTime.Now;
            _logType = logType;
            _time = string.Format("{0:00}:{1:00}:{2:00}", now.Hour, now.Minute, now.Second);
            _log = log;
            _stackTrace = stackTrace;
        }

        public LogType _logType;
        public string _time;
        public string _log;
        public string _stackTrace;
    }; // class SocketLogMessage

    //---------------------------------

    [Serializable]
    public class SocketConnectionInformationMessage : SocketMessageBase
    {
        public static readonly string MESSAGE_TYPE = typeof(SocketConnectionInformationMessage).Name;

        public SocketConnectionInformationMessage(int requestId) : base()
        {
            _requestId = requestId;

            try
            {
                _applicationName = Application.productName;
                _uuid = SystemInfo.deviceUniqueIdentifier;
                _deviceName = UnityEngine.SystemInfo.deviceName;
#if UNITY_EDITOR
                _deviceModel = UnityEngine.SystemInfo.deviceModel;
#elif UNITY_IPHONE
                _deviceModel = UnityEngine.iOS.Device.generation.ToString();
#elif UNITY_ANDROID
                _deviceModel = UnityEngine.SystemInfo.deviceModel;
#endif
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
        }

        public int _requestId;
        public string _applicationName;
        public string _uuid;
        public string _deviceName;
        public string _deviceModel;
    } // class SocketConnectionInformationMessage

    //---------------------------------

    [Serializable]
    public class SocketFileListMessage : SocketMessageBase
    {
        public static readonly string MESSAGE_TYPE = typeof(SocketFileListMessage).Name;

        public SocketFileListMessage(int requestId) : base()
        {
            _requestId = requestId;
        }

        public void UpdateList(UnityDirectoryType directoryType, string targetPath)
        {
            _files = null;
            _directories = null;

            _directory = GetDirectory(directoryType);
            var path = (string.IsNullOrEmpty(targetPath) ? _directory : Path.Combine(_directory, targetPath));
            if (!Directory.Exists(path))
            {
                return;
            }

            _files = Directory.GetFiles(path);
            _directories = Directory.GetDirectories(path);
        }

        public int _requestId;
        public string _directory;
        public string[] _files = null;
        public string[] _directories = null;
    } // class SocketFileListMessage

    //---------------------------------

    [Serializable]
    public class SocketFileMessage : SocketMessageBase
    {
        public static readonly string MESSAGE_TYPE = typeof(SocketFileMessage).Name;

        public SocketFileMessage(int requestId) : base()
        {
            _requestId = requestId;
        }

        public bool SetFile(UnityDirectoryType directoryType, string targetPath)
        {
            _directoryType = directoryType;
            _targetPath = targetPath;

            var directory = GetDirectory(directoryType);
            var path = (string.IsNullOrEmpty(targetPath) ? directory : Path.Combine(directory, targetPath));
            if (!File.Exists(path))
            {
                Debug.LogErrorFormat("指定されたファイルが存在しない：{0}", path);
                return false;
            }

            var bytes = System.IO.File.ReadAllBytes(path);
            if (bytes == null)
            {
                Debug.LogErrorFormat("ファイルの読み込み失敗：{0}", path);
                return false;
            }
            _data = Convert.ToBase64String(bytes);

            return true;
        }

        public byte[] GetFileData()
        {
            if (string.IsNullOrEmpty(_data))
            {
                return null;
            }

            char[] encodedArray = _data.ToCharArray();
            var file = Convert.FromBase64CharArray(encodedArray, 0, encodedArray.Length);
            return file;
        }

        public bool WriteFile()
        {
            var directory = GetDirectory(_directoryType);
            if (!Directory.Exists(directory))
            {
                Debug.LogErrorFormat("ディレクトリが存在しない： {0}", directory);
                return false;
            }

            string fileName = GetFileName();
            string path = Path.Combine(directory, fileName);

            var fileData = GetFileData();
            if (fileData == null)
            {
                return false;
            }

            using (var stream = new FileStream(path, FileMode.Create))
            {
                using (var writer = new BinaryWriter(stream))
                {
                    writer.Write(fileData);
                }
                stream.Close();

                if (!File.Exists(path))
                {
                    Debug.LogErrorFormat("ファイルの書き込みに失敗： {0}", path);
                    return false;
                }
            }

            return true;
        }

        public string GetFileName()
        {
            var index = _targetPath.LastIndexOf('/');
            if (index == -1)
            {
                return _targetPath;
            }
            else
            {
                return _targetPath.Substring(index + 1);
            }
        }

        public UnityDirectoryType _directoryType = UnityDirectoryType.Invalid;
        public string _targetPath;
        public int _requestId;
        public string _data;
    } // class SocketFileMessage

    //---------------------------------

    [Serializable]
    public class SocketImageDataMessage : SocketMessageBase
    {
        public static readonly string MESSAGE_TYPE = typeof(SocketImageDataMessage).Name;

        public byte[] GetImage()
        {
            if (string.IsNullOrEmpty(_imageData))
            {
                return null;
            }

            char[] encodedArray = _imageData.ToCharArray();
            var image = Convert.FromBase64CharArray(encodedArray, 0, encodedArray.Length);
            return image;
        }

        public bool SetImage(byte[] bytes)
        {
            if (bytes == null)
            {
                return false;
            }
            _imageData = Convert.ToBase64String(bytes);

            return true;
        }

        public string _imageData;
    } // class SocketImageDataMessage

    //---------------------------------

    [Serializable]
    public class SocketScreenShotMessage : SocketImageDataMessage
    {
        public new static readonly string MESSAGE_TYPE = typeof(SocketScreenShotMessage).Name;

        public SocketScreenShotMessage(int requestId) : base()
        {
            _requestId = requestId;
        }

        public int _requestId;
        public string _dateTime;
    } // class SocketScreenShotMessage

    //---------------------------------

    [Serializable]
    public class SocketMoveGameObjectMessage : SocketImageDataMessage
    {
        public new static readonly string MESSAGE_TYPE = typeof(SocketMoveGameObjectMessage).Name;

        public SocketMoveGameObjectMessage() : base()
        {
        }

        public int _manipulateTarget;
        public float _x;
        public float _y;
        public float _z;
    } // class SocketMoveGameObjectMessage
} // namespace WebSocketApp
