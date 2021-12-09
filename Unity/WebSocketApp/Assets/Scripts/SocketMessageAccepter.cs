using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace WebSocketApp
{
    public interface ISocketMessageAccepter
    {
        bool Accept(SocketMessageBase message);
    } // interface ISocketMessageAccepter
} // namespace WebSocketApp
