// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/TcpTransceiver.h>
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
IceInternal::TcpTransceiver::fd()
{
    assert(_fd != INVALID_SOCKET);
    return _fd;
}

void
IceInternal::TcpTransceiver::close()
{
    if(_traceLevels->network >= 1)
    {
	Trace out(_logger, _traceLevels->networkCat);
	out << "closing tcp connection\n" << toString();
    }

    assert(_fd != INVALID_SOCKET);
    try
    {
	closeSocket(_fd);
	_fd = INVALID_SOCKET;
    }
    catch(const SocketException&)
    {
	_fd = INVALID_SOCKET;
	throw;
    }
}

void
IceInternal::TcpTransceiver::shutdown()
{
    if(_traceLevels->network >= 2)
    {
	Trace out(_logger, _traceLevels->networkCat);
	out << "shutting down tcp connection\n" << toString();
    }

    assert(_fd != INVALID_SOCKET);
    shutdownSocket(_fd);
}

void
IceInternal::TcpTransceiver::write(Buffer& buf, int timeout)
{
     Buffer::Container::difference_type packetSize = buf.b.end() - buf.i;
    
#ifdef _WIN32
    //
    // Limit packet size to avoid performance problems on WIN32
    //
    if(packetSize > 64 * 1024)
    {
	packetSize = 64 * 1024;
    }
#endif

    while(buf.i != buf.b.end())
    {
	assert(_fd != INVALID_SOCKET);
	ssize_t ret = ::send(_fd, reinterpret_cast<const char*>(&*buf.i), packetSize, 0);

	if(ret == 0)
	{
	    ConnectionLostException ex(__FILE__, __LINE__);
	    ex.error = 0;
	    throw ex;
	}

	if(ret == SOCKET_ERROR)
	{
	    if(interrupted())
	    {
		continue;
	    }

	    if(noBuffers() && packetSize > 1024)
	    {
		packetSize /= 2;
		continue;
	    }

	    if(wouldBlock())
	    {
	    repeatSelect:

		int rs;
		assert(_fd != INVALID_SOCKET);
		FD_SET(_fd, &_wFdSet);

		if(timeout >= 0)
		{
		    struct timeval tv;
		    tv.tv_sec = timeout / 1000;
		    tv.tv_usec = (timeout - tv.tv_sec * 1000) * 1000;
		    rs = ::select(_fd + 1, 0, &_wFdSet, 0, &tv);
		}
		else
		{
		    rs = ::select(_fd + 1, 0, &_wFdSet, 0, 0);
		}
		
		if(rs == SOCKET_ERROR)
		{
		    if(interrupted())
		    {
			goto repeatSelect;
		    }
		    
		    SocketException ex(__FILE__, __LINE__);
		    ex.error = getSocketErrno();
		    throw ex;
		}
		
		if(rs == 0)
		{
		    throw TimeoutException(__FILE__, __LINE__);
		}
		
		continue;
	    }
	    
	    if(connectionLost())
	    {
		ConnectionLostException ex(__FILE__, __LINE__);
		ex.error = getSocketErrno();
		throw ex;
	    }
	    else
	    {
		SocketException ex(__FILE__, __LINE__);
		ex.error = getSocketErrno();
		throw ex;
	    }
	}

	if(_traceLevels->network >= 3)
	{
	    Trace out(_logger, _traceLevels->networkCat);
	    out << "sent " << ret << " of " << packetSize << " bytes via tcp\n" << toString();
	}

	if(_stats)
	{
	    _stats->bytesSent(_name, static_cast<Int>(ret));
	}

	buf.i += ret;

	if(packetSize > buf.b.end() - buf.i)
	{
	    packetSize = buf.b.end() - buf.i;
	}
    }
}

void
IceInternal::TcpTransceiver::read(Buffer& buf, int timeout)
{
    Buffer::Container::difference_type packetSize = buf.b.end() - buf.i;
    
    while(buf.i != buf.b.end())
    {
	assert(_fd != INVALID_SOCKET);
	ssize_t ret = ::recv(_fd, reinterpret_cast<char*>(&*buf.i), packetSize, 0);

	if(ret == 0)
	{
	    //
	    // If the connection is lost when reading data, we shut
	    // down the write end of the socket. This helps to unblock
	    // threads that are stuck in send() or select() while
	    // sending data. Note: I don't really understand why
	    // send() or select() sometimes don't detect a connection
	    // loss. Therefore this helper to make them detect it.
	    //
	    //assert(_fd != INVALID_SOCKET);
	    //shutdownSocket(_fd);
	    
	    ConnectionLostException ex(__FILE__, __LINE__);
	    ex.error = 0;
	    throw ex;
	}

	if(ret == SOCKET_ERROR)
	{
	    if(interrupted())
	    {
		continue;
	    }
	    
	    if(noBuffers() && packetSize > 1024)
	    {
		packetSize /= 2;
		continue;
	    }

	    if(wouldBlock())
	    {
	    repeatSelect:

		int rs;
		assert(_fd != INVALID_SOCKET);
		FD_SET(_fd, &_rFdSet);

		if(timeout >= 0)
		{
		    struct timeval tv;
		    tv.tv_sec = timeout / 1000;
		    tv.tv_usec = (timeout - tv.tv_sec * 1000) * 1000;
		    rs = ::select(_fd + 1, &_rFdSet, 0, 0, &tv);
		}
		else
		{
		    rs = ::select(_fd + 1, &_rFdSet, 0, 0, 0);
		}
		
		if(rs == SOCKET_ERROR)
		{
		    if(interrupted())
		    {
			goto repeatSelect;
		    }
		    
		    SocketException ex(__FILE__, __LINE__);
		    ex.error = getSocketErrno();
		    throw ex;
		}
		
		if(rs == 0)
		{
		    throw TimeoutException(__FILE__, __LINE__);
		}
		
		continue;
	    }
	    
	    if(connectionLost())
	    {
		//
		// See the commment above about shutting down the
		// socket if the connection is lost while reading
		// data.
		//
		//assert(_fd != INVALID_SOCKET);
		//shutdownSocket(_fd);
	    
		ConnectionLostException ex(__FILE__, __LINE__);
		ex.error = getSocketErrno();
		throw ex;
	    }
	    else
	    {
		SocketException ex(__FILE__, __LINE__);
		ex.error = getSocketErrno();
		throw ex;
	    }
	}

	if(_traceLevels->network >= 3)
	{
	    Trace out(_logger, _traceLevels->networkCat);
	    out << "received " << ret << " of " << packetSize << " bytes via tcp\n" << toString();
	}

	if(_stats)
	{
	    _stats->bytesReceived(_name, static_cast<Int>(ret));
	}

	buf.i += ret;

	if(packetSize > buf.b.end() - buf.i)
	{
	    packetSize = buf.b.end() - buf.i;
	}
    }
}

string
IceInternal::TcpTransceiver::toString() const
{
    return _desc;
}

IceInternal::TcpTransceiver::TcpTransceiver(const InstancePtr& instance, SOCKET fd) :
    _traceLevels(instance->traceLevels()),
    _logger(instance->logger()),
    _stats(instance->stats()),
    _name("tcp"),
    _fd(fd)
{
    FD_ZERO(&_rFdSet);
    FD_ZERO(&_wFdSet);

    //
    // fdToString may raise a socket exception.
    //
    const_cast<string&>(_desc) = fdToString(_fd);
}

IceInternal::TcpTransceiver::~TcpTransceiver()
{
    assert(_fd == INVALID_SOCKET);
}
