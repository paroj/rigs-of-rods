/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file
/// @author Petr Ohlidal
/// @brief  Game state manager and message-queue provider

#pragma once

#include <list>
#include <mutex>
#include <queue>
#include <string>

namespace RoR {

/// All gameplay events are specified here (work in progress)
enum MsgType
{
    MSG_INVALID,
    // Networking
    MSG_NET_CONNECT_REQUESTED,
    MSG_NET_CONNECT_STARTED,
    MSG_NET_CONNECT_PROGRESS,
    MSG_NET_CONNECT_SUCCESS,
    MSG_NET_CONNECT_FAILURE,
    MSG_NET_SERVER_KICK,
    MSG_NET_DISCONNECT_REQUESTED,
    MSG_NET_USER_DISCONNECT,
    MSG_NET_RECV_ERROR,
};

/// Unified game event system - all requests and state changes are reported using a message (work in progress)
struct Message
{
    Message(MsgType _type): type(_type) {}

    MsgType     type;
    std::string description;
};

typedef std::queue < Message, std::list<Message>> GameMsgQueue;

class GameContext
{
public:
    GameContext();

    // Message queue
    void                PushMessage(Message m);
    bool                HasMessages();
    Message             PopMessage();

private:
    // Message queue
    GameMsgQueue        m_msg_queue;
    std::mutex          m_msg_mutex;
};

} // namespace RoR
