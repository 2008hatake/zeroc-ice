// **********************************************************************
//
// Copyright (c) 2003
// ZeroC, Inc.
// Billerica, MA, USA
//
// All Rights Reserved.
//
// Ice is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation.
//
// **********************************************************************

#include <Ice/UdpTransceiver.h>
#include <Ice/Instance.h>
#include <Ice/TraceLevels.h>
#include <Ice/LoggerUtil.h>
#include <Ice/Stats.h>
#include <Ice/Buffer.h>
#include <Ice/Network.h>
#include <Ice/LocalException.h>

using namespace std;
using namespace Ice;
using namespace IceInternal;

SOCKET
IceInternal::UdpTransceiver::fd()
{
    assert(_fd != INVALID_SOCKET);
    return _fd;
}

void
IceInternal::UdpTransceiver::close()
{
    if(_traceLevels->network >= 1)
    {
	Trace out(_logger, _traceLevels->networkCat);
	out << "closing udp connection\n" << toString();
    }

    assert(_fd != INVALID_SOCKET);
    closeSocket(_fd);
    _fd = INVALID_SOCKET;
}

void
IceInternal::UdpTransceiver::shutdown()
{
}

void
IceInternal::UdpTransceiver::write(Buffer& buf, int)
{
    assert(buf.i == buf.b.begin());
#ifndef NDEBUG
    const int packetSize = 64 * 1024; // TODO: configurable
    assert(packetSize >= static_cast<int>(buf.b.size())); // TODO: exception
#endif

repeat:

    assert(_fd != INVALID_SOCKET);
    int ret = ::send(_fd, &buf.b[0], buf.b.size(), 0);
    
    if(ret == SOCKET_ERROR)
    {
	if(interrupted())
	{
	    goto repeat;
	}

	if(wouldBlock())
	{
	repeatSelect:

	    assert(_fd != INVALID_SOCKET);
	    FD_SET(_fd, &_wFdSet);
	    int ret = ::select(_fd + 1, 0, &_wFdSet, 0, 0);
	    
	    if(ret == SOCKET_ERROR)
	    {
		if(interrupted())
		{
		    goto repeatSelect;
		}
		
		SocketException ex(__FILE__, __LINE__);
		ex.error = getSocketErrno();
		throw ex;
	    }
	    
	    goto repeat;
	}
	
	SocketException ex(__FILE__, __LINE__);
	ex.error = getSocketErrno();
	throw ex;
    }

    if(_traceLevels->network >= 3)
    {
	Trace out(_logger, _traceLevels->networkCat);
	out << "sent " << ret << " bytes via udp\n" << toString();
    }
    
    if(_stats)
    {
	_stats->bytesSent(_name, ret);
    }

    assert(ret == static_cast<int>(buf.b.size()));
    buf.i = buf.b.end();
}

void
IceInternal::UdpTransceiver::read(Buffer& buf, int)
{
    assert(buf.i == buf.b.begin());
    const int packetSize = 64 * 1024; // TODO: configurable
    assert(packetSize >= static_cast<int>(buf.b.size())); // TODO: exception
    buf.b.resize(packetSize);
    buf.i = buf.b.begin();

repeat:

    int ret;
    if(_connect)
    {
	//
	// If we must connect, then we connect to the first peer that
	// sends us a packet.
	//
	struct sockaddr_in peerAddr;
	memset(&peerAddr, 0, sizeof(struct sockaddr_in));
	socklen_t len = sizeof(peerAddr);
	assert(_fd != INVALID_SOCKET);
	ret = recvfrom(_fd, &buf.b[0], packetSize, 0, reinterpret_cast<struct sockaddr*>(&peerAddr), &len);
	if(ret != SOCKET_ERROR)
	{
	    doConnect(_fd, peerAddr, -1);
	    _connect = false; // We are connected now.

	    if(_traceLevels->network >= 1)
	    {
		Trace out(_logger, _traceLevels->networkCat);
		out << "connected udp socket\n" << toString();
	    }
	}
    }
    else
    {
	assert(_fd != INVALID_SOCKET);
	ret = ::recv(_fd, &buf.b[0], packetSize, 0);
    }
    
    if(ret == SOCKET_ERROR)
    {
	if(interrupted())
	{
	    goto repeat;
	}
	
	if(wouldBlock())
	{
	repeatSelect:
	    
	    assert(_fd != INVALID_SOCKET);
	    FD_SET(_fd, &_rFdSet);
	    int ret = ::select(_fd + 1, &_rFdSet, 0, 0, 0);
	    
	    if(ret == SOCKET_ERROR)
	    {
		if(interrupted())
		{
		    goto repeatSelect;
		}
		
		SocketException ex(__FILE__, __LINE__);
		ex.error = getSocketErrno();
		throw ex;
	    }
	    
	    goto repeat;
	}

	SocketException ex(__FILE__, __LINE__);
	ex.error = getSocketErrno();
	throw ex;
    }
    
    if(_traceLevels->network >= 3)
    {
	Trace out(_logger, _traceLevels->networkCat);
	out << "received " << ret << " bytes via udp\n" << toString();
    }

    if(_stats)
    {
	_stats->bytesReceived(_name, ret);
    }

    buf.b.resize(ret);
    buf.i = buf.b.end();
}

string
IceInternal::UdpTransceiver::toString() const
{
    return fdToString(_fd);
}

bool
IceInternal::UdpTransceiver::equivalent(const string& host, int port) const
{
    struct sockaddr_in addr;
    getAddress(host, port, addr);
    return compareAddress(addr, _addr);
}

int
IceInternal::UdpTransceiver::effectivePort()
{
    return ntohs(_addr.sin_port);
}

IceInternal::UdpTransceiver::UdpTransceiver(const InstancePtr& instance, const string& host, int port) :
    _traceLevels(instance->traceLevels()),
    _logger(instance->logger()),
    _incoming(false),
    _connect(true)
{
    try
    {
	_fd = createSocket(true);
	setBlock(_fd, false);
	getAddress(host, port, _addr);
	doConnect(_fd, _addr, -1);
	_connect = false; // We're connected now
	
	if(_traceLevels->network >= 1)
	{
	    Trace out(_logger, _traceLevels->networkCat);
	    out << "starting to send udp packets\n" << toString();
	}
    }
    catch(...)
    {
	_fd = INVALID_SOCKET;
	throw;
    }

    FD_ZERO(&_rFdSet);
    FD_ZERO(&_wFdSet);
}

IceInternal::UdpTransceiver::UdpTransceiver(const InstancePtr& instance, const string& host, int port, bool connect) :
    _traceLevels(instance->traceLevels()),
    _logger(instance->logger()),
    _stats(instance->stats()),
    _name("udp"),
    _incoming(true),
    _connect(connect)
{
    try
    {
	_fd = createSocket(true);
	setBlock(_fd, false);
	getAddress(host, port, _addr);
	if(_traceLevels->network >= 2)
	{
	    Trace out(_logger, _traceLevels->networkCat);
	    out << "attempting to bind to udp socket\n" << toString();
	}
	doBind(_fd, _addr);
	    
	if(_traceLevels->network >= 1)
	{
	    Trace out(_logger, _traceLevels->networkCat);
	    out << "starting to receive udp packets\n" << toString();
	}
    }
    catch(...)
    {
	_fd = INVALID_SOCKET;
	throw;
    }

    FD_ZERO(&_rFdSet);
    FD_ZERO(&_wFdSet);
}

IceInternal::UdpTransceiver::~UdpTransceiver()
{
    assert(_fd == INVALID_SOCKET);
}
