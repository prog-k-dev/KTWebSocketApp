using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.UI;

namespace WebSocketApp
{
    public class WebSocketApp : MonoBehaviour, ISocketMessageAccepter
    {
        private const string SCREENSHOT_FILENAME = "screenshot.png";

        private static readonly Dictionary<string, Type> ACCEPTABLE_MESSAGE_TYPES = new Dictionary<string, Type>()
        {
            {typeof(SockeTextMessage).Name,  typeof(SockeTextMessage)},
            {typeof(SocketRequestMessage).Name,  typeof(SocketRequestMessage)},
            {typeof(SocketConnectionInformationMessage).Name,  typeof(SocketConnectionInformationMessage)},
            {typeof(SocketScreenShotRequestMessage).Name,  typeof(SocketScreenShotRequestMessage)},
            {typeof(SocketFileListRequestMessage).Name,  typeof(SocketFileListRequestMessage)},
            {typeof(SocketFileUploadRequestMessage).Name,  typeof(SocketFileUploadRequestMessage)},
            {typeof(SocketConnectGameObjectRequestMessage).Name,  typeof(SocketConnectGameObjectRequestMessage)},
            {typeof(SocketFileMessage).Name,  typeof(SocketFileMessage)},
            {typeof(SocketImageDataMessage).Name,  typeof(SocketImageDataMessage)},
            {typeof(SocketMoveGameObjectMessage).Name,  typeof(SocketMoveGameObjectMessage)},
        };

        [SerializeField]
        private WebSocketConnection _connection = null;

        [SerializeField]
        private int _maxLogLength = 1024;

        [SerializeField]
        private Text _connectionInfoText = null;

        [SerializeField]
        private Text _receivedFileText = null;

        [SerializeField]
        private Text _receivedText = null;

        [SerializeField]
        private RawImage _rawImage = null;

        private bool IsOpen
        {
            get
            {
                return _connection != null && _connection.IsOpen;
            }
        }

        private bool _isConnectionChanged = false;
        private Queue<SocketMessageBase> _messageStore = new Queue<SocketMessageBase>();
        private Transform _manipulateTarget = null;
        private Coroutine _screenShotCoroutine = null;

        private void Start()
        {
            if (!Setup())
            {
                Debug.LogErrorFormat("セットアップ失敗");
            }
        }

        private void OnEnable()
        {
            _connection.OnConnectionChanged = OnConnectionChanged;
        }

        private void OnDisable()
        {
            _connection.OnConnectionChanged = null;

            StopUpdateScreenShot();
        }


        private bool Setup()
        {
            Application.logMessageReceivedThreaded += ReceiveLogMessage;

            if (_connection == null)
            {
                Debug.LogErrorFormat("コネクションが指定されていない");
                return false;
            }
            _connection.AddAccepter(this);

            return true;
        }

        private void OnConnectionChanged()
        {
            _isConnectionChanged = true;
        }

        public bool Accept(SocketMessageBase message)
        {
            if (!ACCEPTABLE_MESSAGE_TYPES.ContainsKey(message._messageType))
            {
                return false;
            }

            StoreMessage(message);
            return true;
        }

        private void Update()
        {
            if (_isConnectionChanged)
            {
                _isConnectionChanged = false;
                if (_connection != null && !_connection.IsConnect)
                {
                    _connectionInfoText.text = "切断しました";
                }
            }

            ApplyMessage();
        }

        private void ApplyMessage()
        {
            SocketMessageBase message = GetNextMessage();
            while (message != null)
            {
                ApplyMessage(message);

                message = GetNextMessage();
            }
        }
        private void ApplyMessage(SocketMessageBase message)
        {
            if (message.MessageType == SocketRequestMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketRequestMessage);
            }
            else if (message.MessageType == SockeTextMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SockeTextMessage);
            }
            else if (message.MessageType == SocketConnectionInformationMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketConnectionInformationMessage);
            }
            else if (message.MessageType == SocketScreenShotRequestMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketScreenShotRequestMessage);
            }
            else if (message.MessageType == SocketFileListRequestMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketFileListRequestMessage);
            }
            else if (message.MessageType == SocketFileUploadRequestMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketFileUploadRequestMessage);
            }
            else if (message.MessageType == SocketConnectGameObjectRequestMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketConnectGameObjectRequestMessage);
            }
            else if (message.MessageType == SocketFileMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketFileMessage);
            }
            else if (message.MessageType == SocketImageDataMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketImageDataMessage);
            }
            else if (message.MessageType == SocketMoveGameObjectMessage.MESSAGE_TYPE)
            {
                ApplyMessage(message as SocketMoveGameObjectMessage);
            }
        }

        public bool ApplyMessage(SockeTextMessage message)
        {
            if (_connection == null)
            {
                return false;
            }

            _receivedText.text = message._text;
            return true;
        }

        public bool ApplyMessage(SocketRequestMessage message)
        {
            if (_connection == null)
            {
                return false;
            }

            if (message._request == SocketConnectionInformationMessage.MESSAGE_TYPE)
            {
                if (!SendConnectionInformation(message._requestId))
                {
                    return false;
                }
            }

            return true;
        }

        public bool ApplyMessage(SocketConnectionInformationMessage message)
        {
            if (_connection == null)
            {
                return false;
            }

            _connectionInfoText.text = string.Format("[{0}({1})] で動作中のアプリケーション[{2}]に接続中",
                message._deviceName, message._deviceModel, message._applicationName);
            return true;
        }

        public bool ApplyMessage(SocketScreenShotRequestMessage message)
        {
            if (_connection == null)
            {
                return false;
            }

            UpdateScreenShot(message);
            return true;
        }

        public bool ApplyMessage(SocketFileListRequestMessage message)
        {
            if (_connection == null)
            {
                return false;
            }

            SendFileList(message._requestId, message._directoryType, message._targetPath);
            return true;
        }

        public bool ApplyMessage(SocketFileUploadRequestMessage message)
        {
            if (_connection == null)
            {
                return false;
            }

            SendFile(message._requestId, message._directoryType, message._targetPath);
            return true;
        }

        public bool ApplyMessage(SocketConnectGameObjectRequestMessage message)
        {
            if (_connection == null)
            {
                return false;
            }

            var obj = GameObject.Find(message._gameObjectName);
            if (obj == null)
            {
                Debug.LogErrorFormat("指定された名前({0})のGameObjectは存在しません", message._gameObjectName);
                return false;
            }
            Debug.LogFormat("GameObject({0})に接続しました", message._gameObjectName);

            SetManipulateTarget(obj.transform);

            return true;
        }

        private void SetManipulateTarget(Transform target)
        {
            if (_manipulateTarget != null)
            {
                var renderer = _manipulateTarget.GetComponent<MeshRenderer>();
                if (renderer)
                {
                    renderer.material.color = Color.white;
                }
            }

            _manipulateTarget = target;
            {
                var renderer = _manipulateTarget.GetComponent<MeshRenderer>();
                if (renderer)
                {
                    renderer.material.color = Color.red;
                }
            }
        }

        public bool ApplyMessage(SocketFileMessage message)
        {
            if (_connection == null)
            {
                return false;
            }

            if (!message.WriteFile())
            {
                return false;
            }
            Debug.LogFormat("受信したファイル({0})を書き込みました：directory={1}", message.GetFileName(), SocketMessageBase.GetDirectory(message._directoryType));

            var path = Path.Combine(SocketMessageBase.GetDirectory(message._directoryType), message.GetFileName());
            FileInfo fileInfo = new FileInfo(path);
            _receivedFileText.text = string.Format("{0}：size={1:#,0}", message.GetFileName(), fileInfo.Length);

            if (fileInfo.Extension.ToLower() == ".png")
            {
                Texture2D texture = new Texture2D(4, 4);
                {
                    byte[] image = message.GetFileData();
                    if (image == null)
                    {
                        return false;
                    }
                    if (!texture.LoadImage(image))
                    {
                        return false;
                    }
                }
                _rawImage.texture = texture;
            }

            return true;
        }

        public bool ApplyMessage(SocketImageDataMessage message)
        {
            if (_connection == null || _rawImage == null)
            {
                return false;
            }

            Texture2D texture = new Texture2D(4, 4);
            {
                byte[] image = message.GetImage();
                if (image == null)
                {
                    return false;
                }
                if (!texture.LoadImage(image))
                {
                    return false;
                }
            }
            _rawImage.texture = texture;

            return true;
        }

        public bool ApplyMessage(SocketMoveGameObjectMessage message)
        {
            if (_connection == null || _rawImage == null)
            {
                return false;
            }

            if (_manipulateTarget)
            {
                _manipulateTarget.position += new Vector3(message._x, message._y, message._z);
            }

            return true;
        }

        private void StoreMessage(SocketMessageBase message)
        {
            lock (_messageStore)
            {
                _messageStore.Enqueue(message);
            }
        }

        private SocketMessageBase GetNextMessage()
        {
            SocketMessageBase message = null;
            lock (_messageStore)
            {
                if (_messageStore.Count > 0)
                {
                    message = _messageStore.Dequeue();
                }
            }
            return message;
        }

        private bool SendConnectionInformation(int requestId)
        {
            if (_connection == null)
            {
                return false;
            }

            _connection.SendMessage(new SocketConnectionInformationMessage(requestId));
            return true;
        }

        private void SendFileList(int requestId, UnityDirectoryType directoryType, string targetPath)
        {
            SocketFileListMessage message = new SocketFileListMessage(requestId);
            message.UpdateList(directoryType, targetPath);
            _connection.SendMessage(message);
        }

        private void SendFile(int requestId, UnityDirectoryType directoryType, string targetPath)
        {
            SocketFileMessage message = new SocketFileMessage(requestId);
            message.SetFile(directoryType, targetPath);
            _connection.SendMessage(message);
        }

        private void UpdateScreenShot(SocketScreenShotRequestMessage message)
        {
            StopUpdateScreenShot();
            if (message._stop)
            {
                return;
            }

            _screenShotCoroutine = StartCoroutine(UpdateScreenShotAsync(message._requestId, message._interval));

        }
        private IEnumerator UpdateScreenShotAsync(int requestId, float interval)
        {
            while (IsOpen)
            {
                yield return new WaitForEndOfFrame();

                if (!IsOpen)
                {
                    break;
                }
                SendScreenShot(requestId);

                if (interval <= 0)
                {
                    break;
                }
                yield return new WaitForSeconds(interval); 
            }

            _screenShotCoroutine = null;
        }

        private void StopUpdateScreenShot()
        {
            if (_screenShotCoroutine != null)
            {
                StopCoroutine(_screenShotCoroutine);
                _screenShotCoroutine = null;
            }
        }

        private bool SendScreenShot(int requestId)
        {
            if (!IsOpen)
            {
                return false;
            }

            var now = DateTime.Now;
            Texture2D texture = ScreenCapture.CaptureScreenshotAsTexture();
            if (texture == null)
            {
                Debug.LogError("スクリーンショット撮影失敗");
                return false;
            }

            SocketScreenShotMessage message = new SocketScreenShotMessage(requestId);
            if (!message.SetImage(texture.EncodeToPNG()))
            {
                return false;
            }

            message._dateTime = string.Format("{0:0000}/{1:00}/{2:00} {3:00}:{4:00}:{5:00}",
                now.Year, now.Month, now.Day, now.Hour, now.Minute, now.Second);
            Debug.LogFormat("スクリーンショット（{0}）を送信", message._dateTime);

            _connection.SendMessage(message, true);
            return true;
        }

        private void ReceiveLogMessage(string logString, string stackTrace, LogType type)
        {
            if (_connection == null)
            {
                return;
            }

            _connection.SendMessage(new SocketLogMessage(type, logString.Length <= _maxLogLength ? logString : logString.Substring(0, _maxLogLength), stackTrace));
        }

        private void OnDestroy()
        {
            Application.logMessageReceivedThreaded -= ReceiveLogMessage;
        }
    } // class WebSocketApp
} // namespace WebSocketApp
