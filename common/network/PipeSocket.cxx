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
#include <network/PipeSocket.h>
#include <rfb/util.h>
#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>

using namespace network;
using namespace rdr;

static rfb::LogWriter vlog("PipeSocket");

// -=- Socket initialisation
static bool socketsInitialised = false;
static void initSockets() {
  if (socketsInitialised)
    return;

  signal(SIGPIPE, SIG_IGN);

  socketsInitialised = true;
}


// -=- PipeSocket

PipeSocket::PipeSocket(int sock_in, int sock_out, bool close)
  : Socket(new FdInStream(sock_in), new FdOutStream(sock_out), true), closeFd(close)
{
}

PipeSocket::PipeSocket(const char *cmd)
  : closeFd(true)
{
  int err;

  // - Create a socket
  initSockets();

  int pipeOut[2]; // to child
  int pipeIn[2]; // from child

  if (pipe(pipeOut) < 0) {
		err = errorNumber;
		throw SocketException("unable to create pipe", err);
  }
  
  if (pipe(pipeIn) < 0) {
	  err = errorNumber;
	  throw SocketException("unable to create pipe", err);
  }

  vlog.debug("Running pipe command %s", cmd);

  int childPid = fork();

  if (childPid == 0) { // child

	  if (dup2(pipeOut[0], 0) < 0) {
		  _exit(-1);
	  }

	  if (dup2(pipeIn[1], 1) < 0) {
		  _exit(-1);
	  }

	  close(pipeOut[0]);
	  close(pipeOut[1]);
	  close(pipeIn[0]);
	  close(pipeIn[1]);

	  char *shell = getenv("SHELL");
	  if (shell == NULL) {
			shell = new char[10];
			strcpy(shell, "/bin/sh");
	  }

	  if (execl(shell, shell, "-c", cmd, (char *)0) < 0) {
		_exit(-1);
	  }

	  _exit(0);
  }
  if (childPid < 0) {
		err = errorNumber;
		throw SocketException("fork error", err);
  }

  close(pipeOut[0]);
  close(pipeIn[1]);

  // Create the input and output streams
  instream = new FdInStream(pipeIn[0]);
  outstream = new FdOutStream(pipeOut[1]);
  ownStreams = true;
}

PipeSocket::~PipeSocket() {
  if (closeFd) {
    close(outstream->getFd());
    close(instream->getFd());
  }
}

int PipeSocket::getMyPort() {
  return 0;
}

char* PipeSocket::getPeerAddress() {
  return rfb::strDup("");
}

char* PipeSocket::getPeerEndpoint() {
	return getPeerAddress();
}

bool PipeSocket::sameMachine() {
  return true;
}

int PipeSocket::getPeerPort() {
	return 0;
}

void PipeSocket::shutdown()
{
  Socket::shutdown();
  close(outstream->getFd());
  close(instream->getFd());
}

bool PipeSocket::enableNagles(int sock, bool enable) {
  return true;
}

bool PipeSocket::cork(int sock, bool enable) {
  return false;
}

bool PipeSocket::isListening(int sock)
{
    return false;
}

#endif
