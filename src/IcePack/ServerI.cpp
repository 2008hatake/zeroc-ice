// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <IcePack/ServerI.h>
#include <IcePack/ServerFactory.h>
#include <IcePack/TraceLevels.h>
#include <IcePack/Activator.h>

#include <IceBox/IceBox.h>

using namespace std;
using namespace IcePack;

IcePack::ServerI::ServerI(const ServerFactoryPtr& factory, const TraceLevelsPtr& traceLevels, 
			  const ActivatorPtr& activator, Ice::Int waitTime) :
    _factory(factory),
    _traceLevels(traceLevels),
    _activator(activator),
    _waitTime(waitTime),
    _state(Inactive)
{
    assert(_activator);

}

IcePack::ServerI::~ServerI()
{
}

ServerDescription
IcePack::ServerI::getServerDescription(const Ice::Current&)
{
    return description; // Description is immutable.
}

bool
IcePack::ServerI::start(ServerActivation act, const Ice::Current& current)
{
    while(true)
    {
	IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);
	switch(_state)
	{
	case Inactive:
	{
	    if(act < activation)
	    {
	        return false;
	    }

	    _state = Activating;
	    break;
	}
	case Activating:
	case Deactivating:
	{
	    wait(); // TODO: Timeout?
	    continue;
	}
 	case Active:
	{
	    return true; // Raise an exception instead?
	}
	case Destroying:
	case Destroyed:
	{
	    Ice::ObjectNotExistException ex(__FILE__,__LINE__);
	    ex.id = current.id;
	    throw ex;
	}
	}

	if(_traceLevels->server > 2)
	{
	    Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	    out << "changed server `" << description.name << "' state to `Activating'";
	}
	assert(_state == Activating);
	break;
    }

    try
    {
        bool active = _activator->activate(this);
	setState(active ? Active : Inactive);
	return active;
    }
    catch(const Ice::SyscallException& ex)
    {
	Ice::Warning out(_traceLevels->logger);
	out << "activation failed for server `" << description.name << "':\n";
	out << ex;

	setState(Inactive);
	return false;
    }
}

void
IcePack::ServerI::stop(const Ice::Current& current)
{
    while(true)
    {
	IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);
	switch(_state)
	{
	case Inactive:
	{
	    return;
	}
	case Activating:
	{
	    wait(); // TODO: Timeout?
	    continue;
	}
 	case Active:
	{	    
	    _state = Deactivating;
	    break;
	}
	case Deactivating:
	{
	    wait();
	    continue;
	}
	case Destroying:
	case Destroyed:
	{
	    Ice::ObjectNotExistException ex(__FILE__,__LINE__);
	    ex.id = current.id;
	    throw ex;
	}
	}

	if(_traceLevels->server > 2)
	{
	    Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	    out << "changed server `" << description.name << "' state to `Deactivating'";
	}
	assert(_state == Deactivating);
	break;
    }

    stopInternal();
}

void
IcePack::ServerI::sendSignal(const string& signal, const Ice::Current& current)
{
    _activator->sendSignal(this, signal);
}

void
IcePack::ServerI::writeMessage(const string& message, Ice::Int fd, const Ice::Current& current)
{
    _activator->writeMessage(this, message, fd);
}


void
IcePack::ServerI::destroy(const Ice::Current& current)
{
    bool stop = false;

    while(true)
    {
	IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);
	switch(_state)
	{
	case Inactive:
	{
	    _state = Destroyed;
	    break;
	}
 	case Active:
	{
	    stop = true;
	    _state = Destroying;
	    break;
	}
	case Activating:
	case Deactivating:
	{
	    wait(); // TODO: Timeout?
	    continue;
	}
	case Destroying:
	case Destroyed:
	{
	    Ice::ObjectNotExistException ex(__FILE__,__LINE__);
	    ex.id = current.id;
	    throw ex;
	}
	}

	assert(_state == Destroyed || _state == Destroying);

	if(_traceLevels->server > 2)
	{
	    Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	    out << "changed server `" << description.name << "' state to `";
	    out << (_state == Destroyed ? "Destroyed" : "Destroying") << "'";
	}
	break;
    }

    if(stop)
    {
	stopInternal();
    }

    //
    // Destroy the server adapters.
    //
    for(ServerAdapters::const_iterator p = adapters.begin(); p != adapters.end(); ++p)
    {
	try
	{
	    (*p)->destroy();
	}
	catch(const Ice::ObjectNotExistException&)
	{
	}
    }

    _factory->destroy(this, current.id);
}

void
IcePack::ServerI::terminated(const Ice::Current&)
{
    ServerState newState = Inactive; // Initialize to keep the compiler happy.
    while(true)
    {
	IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);
	switch(_state)
	{
	case Inactive:
	{
	    assert(false);
	}
	case Activating:
	{
	    wait(); // TODO: Timeout?
	    continue;
	}
	case Active:
	{
	    _state = Deactivating;
	    if(_traceLevels->server > 2)
	    {
		Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
		out << "changed server `" << description.name << "' state to `Deactivating'";
	    }
	    newState = Inactive;
	    break;
	}
	case Deactivating:
	{
	    //
	    // Deactivation was initiated by the stop method.
	    //
	    newState = Inactive;
	    break;
	}
	case Destroying:
	{
	    //
	    // Deactivation was initiated by the destroy method.
	    //
	    newState = Destroyed;
	    break;
	}
	case Destroyed:
	{
	    assert(false);	    
	}
	}

	assert(_state == Deactivating || _state == Destroying);
	break;
    }

    if(newState != Destroyed)
    {
	//
	// The server has terminated, set its adapter direct proxies to
	// null to cause the server re-activation if one of its adapter
	// direct proxy is requested.
	//
	for(ServerAdapters::iterator p = adapters.begin(); p != adapters.end(); ++p)
	{
	    try
	    {
		(*p)->setDirectProxy(0);
	    }
	    catch(const Ice::ObjectNotExistException&)
	    {
		//
		// TODO: Inconsistent database.
		//
	    }
	    catch(const Ice::LocalException&)
	    {
		//cerr << (*p)->__reference()->toString() << endl;
		//cerr << ex << endl;
		throw;
	    }
	}
    }

    setState(newState);
}

ServerState
IcePack::ServerI::getState(const Ice::Current&)
{
    IceUtil::Monitor< ::IceUtil::Mutex>::Lock sync(*this);
    return _state;
}

Ice::Int
IcePack::ServerI::getPid(const Ice::Current& current)
{
    return _activator->getServerPid(this);
}

void 
IcePack::ServerI::setActivationMode(ServerActivation mode, const ::Ice::Current&)
{
    IceUtil::Monitor< ::IceUtil::Mutex>::Lock sync(*this);
    activation = mode;
}

ServerActivation 
IcePack::ServerI::getActivationMode(const ::Ice::Current&)
{
    IceUtil::Monitor< ::IceUtil::Mutex>::Lock sync(*this);
    return activation;
}

void
IcePack::ServerI::setProcess(const ::Ice::ProcessPrx& proc, const ::Ice::Current&)
{
    IceUtil::Monitor< ::IceUtil::Mutex>::Lock sync(*this);
    _process = proc;
}

Ice::ProcessPrx
IcePack::ServerI::getProcess(const ::Ice::Current&)
{
    IceUtil::Monitor< ::IceUtil::Mutex>::Lock sync(*this);
    return _process;
}

void
IcePack::ServerI::stopInternal()
{
    //
    // If the server is an icebox, first try to use the IceBox service
    // manager to shutdown the server.
    //
    bool deactivate = true;
    
    //
    // TODO: use the service manager to shutdown icebox servers. The
    // following code should work once it's possible to set an
    // activation mode on a per adapter basis -- we really don't want
    // to activate the icebox in this method if it's already
    // deactivated.
    //

//     if(description.serviceManager)
//     {
// 	if(_state == Deactivating)
// 	{
// 	    try
// 	    {
// 		//
// 		// Set a timeout on the service manager proxy and try to
// 		// shutdown the icebox.
// 		//
// 		IceBox::ServiceManagerPrx serviceManager = 
// 		    IceBox::ServiceManagerPrx::uncheckedCast(description.serviceManager->ice_timeout(_waitTime));
		
// 		serviceManager->shutdown();
		
// 		deactivate = false;
// 	    }
// 	    catch(const Ice::LocalException& ex)
// 	    {
// 		if(_traceLevels->server > 1)
// 		{
// 		    Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
// 		    out << "couldn't contact the IceBox `" << description.name << "' service manager:\n";
// 		    out << ex;
// 		}
// 	    }
// 	}
//     }

    if(deactivate)
    {
        //
        // Deactivate the server.
        //
        try
        {
            _activator->deactivate(this);
        }
        catch(const Ice::Exception& ex)
        {
            Ice::Warning out(_traceLevels->logger);
            out << "deactivation failed for server `" << description.name << "':\n";
            out << ex;
            
            setState(Active);
            return;
        }
    }

    //
    // Wait for the server to be inactive (the activator monitors the
    // process and should notify us when it detects the process
    // termination by calling the terminated() method).
    //
    {
	IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);

#ifndef NDEBUG
	ServerState oldState = _state;
#endif

	while(true)
	{
	    if(_state == Inactive || _state == Destroyed)
	    {
		//
		// State changed to inactive or destroyed, the server
		// has been correctly deactivated, we can return.
		//
		return;
	    }
	    
	    //
	    // Wait for a notification.
	    //
	    bool notify = timedWait(IceUtil::Time::seconds(_waitTime));
	    if(!notify)
	    {
		assert(oldState == _state);
		break;
	    }
	}
    }
    
    if(_traceLevels->server > 1)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	out << "graceful server shutdown failed, killing server `" << description.name << "'";
    }
    
    //
    // The server is still not inactive, kill it.
    //
    try
    {
	_activator->kill(this);
    }
    catch(const Ice::SyscallException& ex)
    {
	Ice::Warning out(_traceLevels->logger);
	out << "deactivation failed for server `" << description.name << "':\n";
	out << ex;
	
	setState(Active);
    }
}

void
IcePack::ServerI::setState(ServerState st)
{
    IceUtil::Monitor< ::IceUtil::Mutex>::Lock sync(*this);

    _state = st;

    if(_traceLevels->server > 1)
    {
	if(_state == Active)
	{
	    Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	    out << "changed server `" << description.name << "' state to `Active'";
	}
	else if(_state == Inactive)
	{
	    Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	    out << "changed server `" << description.name << "' state to `Inactive'";
	}
	else if(_state == Destroyed)
	{
	    Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	    out << "changed server `" << description.name << "' state to `Destroyed'";
	}
	else if(_traceLevels->server > 2)
	{
	    if(_state == Activating)
	    {
		Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
		out << "changed server `" << description.name << "' state to `Activating'";
	    }
	    else if(_state == Deactivating)
	    {
		Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
		out << "changed server `" << description.name << "' state to `Deactivating'";
	    }
	}
    }

    notifyAll();
}
