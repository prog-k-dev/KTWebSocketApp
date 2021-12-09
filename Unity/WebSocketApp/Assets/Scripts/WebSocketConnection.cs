using System;
using System.Collections.Generic;
using UnityEngine;

using WebSocketSharp;
using WebSocketSharp.Server;

namespace WebSocketApp
{
    public class WebSocketConnection : MonoBehaviour
    {
        private const string SOCKET_PATH = "/WebSocketApp/takahashi_kenji";
        public const ushort SOCKET_PORT_DEFAULT = 5637;

        [SerializeField]
        private string _path = SOCKET_PATH;

        [SerializeField]
        private int _port = SOCKET_PORT_DEFAULT;
        public int Port
        {
            get => _port;
            set => _port = value;
        }

        public bool IsConnect
        {
            get => _isConnect;
            private set
            {
                bool changed = (_isConnect != value);
                _isConnect = value;
                if (changed)
                {
                    OnConnectionChanged?.Invoke();
                }
            }
        }
        private bool _isConnect = false;
        public Action OnConnectionChanged = null;

        public bool IsOpen
        {
            get
            {
                return IsConnect && _webSocketBehavior != null && _webSocketBehavior.IsOpen;
            }
        }

        private WebSocketServer _webSocket = null;
        private MyWebSocketBehavior _webSocketBehavior = null;
        private List<ISocketMessageAccepter> _accepters = new List<ISocketMessageAccepter>();

        private void OnEnable()
        {
            Connect();
        }

        private void OnDisable()
        {
            Disconnect();
        }

        public void AddAccepter(ISocketMessageAccepter accepter)
        {
            if (_accepters.IndexOf(accepter) == -1)
            {
                _accepters.Add(accepter);
            }
        }
        public void RemoveAccepter(ISocketMessageAccepter accepter)
        {
            if (_accepters.IndexOf(accepter) != -1)
            {
                _accepters.Remove(accepter);
            }
        }

        public bool SendMessage(SocketMessageBase message, bool asyncFlag = false)
        {
            if (_webSocket == null || _webSocketBehavior == null)
            {
                return false;
            }

            var payload = SocketMessageBase.ExportMessage(message);
            if (string.IsNullOrEmpty(payload))
            {
                Debug.LogErrorFormat("メッセージから送信データが生成できなかった：message={0}", (message == null ? "(null)" : message.MessageType));
                return false;
            }

            if (asyncFlag)
            {
                _webSocketBehavior.SendStringAsync(payload);
            }
            else
            {
                _webSocketBehavior.SendString(payload);
            }

            return true;
        }

        public void Connect()
        {
            if (_webSocket != null)
            {
                Disconnect();
            }

            _webSocket = new WebSocketServer(Port);
            _webSocket.AddWebSocketService<MyWebSocketBehavior>(_path, InitWebSocketBehavior);
            _webSocket.Start();
        }

        private void InitWebSocketBehavior(WebSocketBehavior webSocketBehavior)
        {
            _webSocketBehavior = webSocketBehavior as MyWebSocketBehavior;
            if (_webSocketBehavior == null)
            {
                Debug.LogErrorFormat("WebSocket生成に失敗：WebSocketBehaviorがnull");
            }
            else
            {
                _webSocketBehavior.Connection = this;
            }
        }

        public void Disconnect()
        {
            _webSocket?.Stop();
            _webSocket = null;
        }

        private void OnOpen()
        {
            IsConnect = true;
        }

        private void OnClose()
        {
            IsConnect = false;
        }

        private bool OnReceiveMessage(string val)
        {
            var message = SocketMessageBase.ImportMessage(val);
            if (message == null)
            {
                return false;
            }

            return Accept( SocketMessageBase.ImportMessage(val));
        }

        private bool OnReceiveMessage(byte[] val)
        {
            Debug.LogErrorFormat("バイナリデータの受信には対応していない：length={0}", val.Length);
            return false;
        }

        private bool Accept(SocketMessageBase message)
        {
            bool accepted = false;
            foreach (var accepter in _accepters)
            {
                if (accepter.Accept(message))
                {
                    accepted = true;
                }
            }
            return accepted;
        }

        //---------------------------------

        private class MyWebSocketBehavior : WebSocketBehavior
        {
            public WebSocketConnection Connection
            {
                get;
                set;
            } = null;

            public bool IsOpen
            {
                get
                {
                    return State == WebSocketState.Open;
                }
            }

            public void SendString(string val)
            {
                if (!IsOpen)
                {
                    Debug.LogWarningFormat("ソケットが開いてないので送信がキャンセルされた：{0} バイト", val.Length);
                    return;
                }

                Send(val);
            }

            public void SendStringAsync(string val)
            {
                if (!IsOpen)
                {
                    Debug.LogWarningFormat("ソケットが開いてないので送信がキャンセルされた：{0} バイト", val.Length);
                    return;
                }

                SendAsync(val, comp =>
                {
                });
            }

            protected override void OnOpen()
            {
                Connection?.OnOpen();
            }

            protected override void OnClose(CloseEventArgs e)
            {
                Connection?.OnClose();
            }

            protected override void OnMessage(MessageEventArgs args)
            {
                if (args.Data != null)
                {
                    Connection?.OnReceiveMessage(args.Data);
                }
                else if (args.RawData != null)
                {
                    Connection?.OnReceiveMessage(args.RawData);
                }
            }
        } // class MyWebSocketBehavior
    } // class WebSocketConnection
} // namespace WebSocketApp
