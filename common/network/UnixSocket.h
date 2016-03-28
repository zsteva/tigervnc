/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * ma
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __NETWORK_UNIX_SOCKET_H__
#define __NETWORK_UNIX_SOCKET_H__

#ifndef WIN32

#include <network/Socket.h>

#include <sys/socket.h> /* for socklen_t */
#include <netinet/in.h> /* for struct sockaddr_in */
#include <sys/un.h> /* for struct sockaddr_in */

#include <list>

namespace network {

  class UnixSocket : public Socket {
  public:
    UnixSocket(int sock, bool close=true);
    UnixSocket(const char *path);
    virtual ~UnixSocket();

    virtual char* getPeerAddress();
    virtual char* getPeerEndpoint();
    virtual bool sameMachine();
	virtual int getMyPort();
	virtual int getPeerPort();


    virtual void shutdown();

    static bool enableNagles(int sock, bool enable);
    static bool cork(int sock, bool enable);
    static bool isListening(int sock);

  private:
    bool closeFd;
  };

  class UnixListener : public SocketListener {
  public:
    UnixListener(const char *path);
    UnixListener(int sock);
    virtual ~UnixListener();

    virtual void shutdown();
    virtual Socket* accept();

    static void getMyAddresses(std::list<char*>* result);
    int getMyPort();
  };

}

#endif

#endif // __NETWORK_UNIX_SOCKET_H__
