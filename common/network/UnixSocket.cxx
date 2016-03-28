/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// http://www.tutorialspoint.com/unix_sockets/unix_sockets_tutorial.pdf

#ifndef WIN32

#define errorNumber errno
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include <stdlib.h>
#include <unistd.h>
#include <network/UnixSocket.h>
#include <rfb/util.h>
#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>

using namespace network;
using namespace rdr;

static rfb::LogWriter vlog("UnixSocket");

// -=- Socket initialisation
static bool socketsInitialised = false;
static void initSockets() {
  if (socketsInitialised)
    return;

  signal(SIGPIPE, SIG_IGN);

  socketsInitialised = true;
}


// -=- UnixSocket

UnixSocket::UnixSocket(int sock, bool close)
  : Socket(new FdInStream(sock), new FdOutStream(sock), true), closeFd(close)
{
}

UnixSocket::UnixSocket(const char *path)
  : closeFd(true)
{
  int sock, err, result;
  struct sockaddr_un sa;

  // - Create a socket
  initSockets();

  memset(&sa, 0, sizeof(struct sockaddr_un));

  sa.sun_family = AF_UNIX;
  strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);


  sock = -1;
  err = 0;
    vlog.debug("Connecting to unix:%s", sa.sun_path);

    sock = socket (AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
      err = errorNumber;
      throw SocketException("unable to create socket", err);
    }

  /* Attempt to connect to the remote host */
    while ((result = connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr_un))) == -1) {
      err = errorNumber;

      if (err == EINTR)
        continue;

      vlog.debug("Failed to connect to unix socket %s", sa.sun_path);
      closesocket(sock);
      sock = -1;
      break;
    }

  if (sock == -1) {
    if (err == 0)
      throw Exception("No useful address for host");
    else
      throw SocketException("unable connect to socket", err);
  }

  // - By default, close the socket on exec()
  fcntl(sock, F_SETFD, FD_CLOEXEC);

  // Disable Nagle's algorithm, to reduce latency
  enableNagles(sock, false);

  // Create the input and output streams
  instream = new FdInStream(sock);
  outstream = new FdOutStream(sock);
  ownStreams = true;
}

UnixSocket::~UnixSocket() {
  if (closeFd)
    closesocket(getFd());
}

/*
int UnixSocket::getMyPort() {
  return getSockPort(getFd());
}
*/

char* UnixSocket::getPeerAddress() {
  struct sockaddr_un sa;
  socklen_t sa_size = sizeof(sa);

  if (getpeername(getFd(), (struct sockaddr *)&sa, &sa_size) != 0) {
    vlog.error("unable to get peer name for socket");
    return rfb::strDup("");
  }

  return rfb::strDup(sa.sun_path);
}

char* UnixSocket::getPeerEndpoint() {
	return getPeerAddress();
}

bool UnixSocket::sameMachine() {
  return true;
}

int UnixSocket::getMyPort() {
	return 0;
}

int UnixSocket::getPeerPort() {
	return 0;
}

void UnixSocket::shutdown()
{
  Socket::shutdown();
  ::shutdown(getFd(), 2);
}

bool UnixSocket::enableNagles(int sock, bool enable) {
  return true;
}

bool UnixSocket::cork(int sock, bool enable) {
  return false;
}

bool UnixSocket::isListening(int sock)
{
    return false;
}

UnixListener::UnixListener(int sock)
{
  fd = sock;
}

UnixListener::UnixListener(const char *path)
{
  int one = 1;
  struct sockaddr_un sa;
  int sock;

  initSockets();

  if ((sock = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
    throw SocketException("unable to create listening socket", errorNumber);

  memset (&sa, 0, sizeof(struct sockaddr_un));
  sa.sun_family = AF_UNIX;
  strncpy(sa.sun_path, path, sizeof(sa.sun_path)-1);

#ifdef FD_CLOEXEC
  // - By default, close the socket on exec()
  fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif

  // SO_REUSEADDR is broken on Windows. It allows binding to a port
  // that already has a listening socket on it. SO_EXCLUSIVEADDRUSE
  // might do what we want, but requires investigation.
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&one, sizeof(one)) < 0) {
    int e = errorNumber;
    closesocket(sock);
    throw SocketException("unable to create listening socket", e);
  }

  if (bind(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr_un)) == -1) {
    int e = errorNumber;
    closesocket(sock);
    throw SocketException("failed to bind socket", e);
  }

  // - Set it to be a listening socket
  if (listen(sock, 5) < 0) {
    int e = errorNumber;
    closesocket(sock);
    throw SocketException("unable to set socket to listening mode", e);
  }

  fd = sock;
}

UnixListener::~UnixListener() {
  closesocket(fd);
}

void UnixListener::shutdown()
{
  ::shutdown(getFd(), 2);
}


Socket*
UnixListener::accept() {
  int new_sock = -1;

  // Accept an incoming connection
  if ((new_sock = ::accept(fd, 0, 0)) < 0)
    throw SocketException("unable to accept new connection", errorNumber);

  // - By default, close the socket on exec()
  fcntl(new_sock, F_SETFD, FD_CLOEXEC);

  // Disable Nagle's algorithm, to reduce latency
  UnixSocket::enableNagles(new_sock, false);

  // Create the socket object & check connection is allowed
  UnixSocket* s = new UnixSocket(new_sock);
  return s;
}

#endif
