// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/ConnectionMonitor.h>
#include <Ice/Connection.h>
#include <Ice/LocalException.h>
#include <Ice/LoggerUtil.h>
#include <Ice/Instance.h>

using namespace std;
using namespace Ice;
using namespace IceInternal;

void IceInternal::incRef(ConnectionMonitor* p) { p->__incRef(); }
void IceInternal::decRef(ConnectionMonitor* p) { p->__decRef(); }

void
IceInternal::ConnectionMonitor::destroy()
{
    {
	IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
	
	assert(_instance);
	
	_instance = 0;
	_connections.clear();
	
	notify();
    }

    getThreadControl().join();
}

void
IceInternal::ConnectionMonitor::add(const ConnectionPtr& connection)
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
    assert(_instance);
    _connections.insert(connection);
}

void
IceInternal::ConnectionMonitor::remove(const ConnectionPtr& connection)
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
    assert(_instance);
    _connections.erase(connection);
}

IceInternal::ConnectionMonitor::ConnectionMonitor(const InstancePtr& instance, int interval) :
    _instance(instance),
    _interval(interval)
{
    assert(_interval > 0);
    start();
}

IceInternal::ConnectionMonitor::~ConnectionMonitor()
{
    assert(!_instance);
    assert(_connections.empty());
}

void
IceInternal::ConnectionMonitor::run()
{
    IceUtil::Time waitTime = IceUtil::Time::seconds(_interval);

    while(true)
    {
	set<ConnectionPtr> connections;
	
	{
	    IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
	    
	    if(!_instance)
	    {
		return;
	    }
	    
	    if(!timedWait(waitTime))
	    {
		connections = _connections;
	    }
	    
	    if(!_instance)
	    {
		return;
	    }
	}
	
	//
	// Monitor connections outside the thread synchronization, so
	// that connections can be added or removed during monitoring.
	//
	for(set<ConnectionPtr>::const_iterator p = connections.begin(); p != connections.end(); ++p)
	{
	    try
	    {	       
		(*p)->monitor();
	    }
	    catch(const Exception& ex)
	    {	
		IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
		
		if(!_instance)
		{
		    return;
		}

		Error out(_instance->logger());
		out << "exception in connection monitor:\n" << ex;
	    }
	    catch(...)
	    {
		IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
		
		if(!_instance)
		{
		    return;
		}

		Error out(_instance->logger());
		out << "unknown exception in connection monitor";
	    }
	}
    }
}
