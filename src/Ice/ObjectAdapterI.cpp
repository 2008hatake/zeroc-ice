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

#include <IceUtil/UUID.h>
#include <Ice/ObjectAdapterI.h>
#include <Ice/Instance.h>
#include <Ice/Proxy.h>
#include <Ice/ProxyFactory.h>
#include <Ice/ReferenceFactory.h>
#include <Ice/Endpoint.h>
#include <Ice/EndpointFactoryManager.h>
#include <Ice/ConnectionFactory.h>
#include <Ice/ServantManager.h>
#include <Ice/RouterInfo.h>
#include <Ice/LocalException.h>
#include <Ice/Properties.h>
#include <Ice/Functional.h>
#include <Ice/LocatorInfo.h>
#include <Ice/Locator.h>
#include <Ice/LoggerUtil.h>
#include <Ice/ThreadPool.h>

#ifdef _WIN32
#   include <sys/timeb.h>
#else
#   include <sys/time.h>
#endif

using namespace std;
using namespace Ice;
using namespace IceInternal;

string
Ice::ObjectAdapterI::getName()
{
    //
    // No mutex lock necessary, _name is immutable.
    //
    return _name;
}

CommunicatorPtr
Ice::ObjectAdapterI::getCommunicator()
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();

    return _communicator;
}

void
Ice::ObjectAdapterI::activate()
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();
    
    if(!_printAdapterReadyDone)
    {
	if(_locatorInfo && !_id.empty())
	{
	    Identity ident;
	    ident.name = "dummy";
	    
	    //
	    // TODO: This might throw if we can't connect to the
	    // locator. Shall we raise a special exception for the
	    // activate operation instead of a non obvious network
	    // exception?
	    //
	    try
	    {
		_locatorInfo->getLocatorRegistry()->setAdapterDirectProxy(_id, newDirectProxy(ident));
	    }
	    catch(const Ice::AdapterNotFoundException&)
	    {
		NotRegisteredException ex(__FILE__, __LINE__);
		ex.kindOfObject = "object adapter";
		ex.id = _id;
		throw ex;
	    }
	    catch(const Ice::AdapterAlreadyActiveException&)
	    {
		ObjectAdapterIdInUseException ex(__FILE__, __LINE__);
		ex.id = _id;
		throw ex;
	    }
	}
    }

    for_each(_incomingConnectionFactories.begin(), _incomingConnectionFactories.end(),
	     Ice::voidMemFun(&IncomingConnectionFactory::activate));

    if(!_printAdapterReadyDone)
    {
	if(_instance->properties()->getPropertyAsInt("Ice.PrintAdapterReady") > 0)
	{
	    cout << _name << " ready" << endl;
	}
	
	_printAdapterReadyDone = true;
    }
}

void
Ice::ObjectAdapterI::hold()
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();
	
    for_each(_incomingConnectionFactories.begin(), _incomingConnectionFactories.end(),
	     Ice::voidMemFun(&IncomingConnectionFactory::hold));
}
    
void
Ice::ObjectAdapterI::waitForHold()
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();

    for_each(_incomingConnectionFactories.begin(), _incomingConnectionFactories.end(),
	     Ice::constVoidMemFun(&IncomingConnectionFactory::waitUntilHolding));
}

void
Ice::ObjectAdapterI::deactivate()
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);
    
    //
    // Ignore deactivation requests if the object adapter has already
    // been deactivated.
    //
    if(_deactivated)
    {
	return;
    }
    
    for_each(_incomingConnectionFactories.begin(), _incomingConnectionFactories.end(),
	     Ice::voidMemFun(&IncomingConnectionFactory::destroy));
    
    _instance->outgoingConnectionFactory()->removeAdapter(this);

    _deactivated = true;
    
    notifyAll();
}

void
Ice::ObjectAdapterI::waitForDeactivate()
{
    {
	IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

	//
	// First we wait for deactivation of the adapter itself, and for
	// the return of all direct method calls using this adapter.
	//
	while(!_deactivated || _directCount > 0)
	{
	    wait();
	}

	//
	// If some other thread is currently deactivating, we wait
	// until this thread is finished.
	//
	while(_waitForDeactivate)
	{
	    wait();
	}
	_waitForDeactivate = true;
    }

    //
    // Now we wait until all incoming connection factories are
    // finished.
    //
    for_each(_incomingConnectionFactories.begin(), _incomingConnectionFactories.end(),
	     Ice::voidMemFun(&IncomingConnectionFactory::waitUntilFinished));

    //
    // Now it's also time to clean up our servants and servant
    // locators.
    //
    if(_servantManager)
    {
	_servantManager->destroy();
    }

    //
    // Destroy the thread pool.
    //
    if(_threadPool)
    {
	_threadPool->destroy();
	_threadPool->joinWithAllThreads();
    }

    {
	IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

	//
	// Signal that waiting is complete.
	//
	_waitForDeactivate = false;
	notifyAll();

	//
	// We're done, now we can throw away all incoming connection
	// factories.
	//
	_incomingConnectionFactories.clear();

	//
	// Remove object references (some of them cyclic).
	//
	_instance = 0;
	_threadPool = 0;
	_servantManager = 0;
	_communicator = 0;
    }
}

ObjectPrx
Ice::ObjectAdapterI::add(const ObjectPtr& object, const Identity& ident)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();
    checkIdentity(ident);

    _servantManager->addServant(object, ident);

    return newProxy(ident);
}

ObjectPrx
Ice::ObjectAdapterI::addWithUUID(const ObjectPtr& object)
{
    Identity ident;
    ident.name = IceUtil::generateUUID();
    return add(object, ident);
}

void
Ice::ObjectAdapterI::remove(const Identity& ident)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();
    checkIdentity(ident);

    _servantManager->removeServant(ident);
}

void
Ice::ObjectAdapterI::addServantLocator(const ServantLocatorPtr& locator, const string& prefix)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();

    _servantManager->addServantLocator(locator, prefix);
}

ServantLocatorPtr
Ice::ObjectAdapterI::findServantLocator(const string& prefix)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();

    return _servantManager->findServantLocator(prefix);
}

ObjectPtr
Ice::ObjectAdapterI::identityToServant(const Identity& ident)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();
    checkIdentity(ident);

    return _servantManager->findServant(ident);
}

ObjectPtr
Ice::ObjectAdapterI::proxyToServant(const ObjectPrx& proxy)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();

    ReferencePtr ref = proxy->__reference();
    return identityToServant(ref->identity);
}

ObjectPrx
Ice::ObjectAdapterI::createProxy(const Identity& ident)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();
    checkIdentity(ident);

    return newProxy(ident);
}

ObjectPrx
Ice::ObjectAdapterI::createDirectProxy(const Identity& ident)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);
    
    checkForDeactivation();
    checkIdentity(ident);

    return newDirectProxy(ident);
}

ObjectPrx
Ice::ObjectAdapterI::createReverseProxy(const Identity& ident)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);
    
    checkForDeactivation();
    checkIdentity(ident);

    //
    // Create a reference and return a reverse proxy for this
    // reference.
    //
    vector<EndpointPtr> endpoints;
    ReferencePtr ref = _instance->referenceFactory()->create(ident, Context(), vector<string>(), Reference::ModeTwoway,
							     false, "", endpoints, 0, 0, this, true);
    return _instance->proxyFactory()->referenceToProxy(ref);
}

void
Ice::ObjectAdapterI::addRouter(const RouterPrx& router)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);
    
    checkForDeactivation();

    RouterInfoPtr routerInfo = _instance->routerManager()->get(router);
    if(routerInfo)
    {
	//
	// Add the router's server proxy endpoints to this object
	// adapter.
	//
	ObjectPrx proxy = routerInfo->getServerProxy();
	copy(proxy->__reference()->endpoints.begin(), proxy->__reference()->endpoints.end(),
	     back_inserter(_routerEndpoints));
	sort(_routerEndpoints.begin(), _routerEndpoints.end()); // Must be sorted.
	_routerEndpoints.erase(unique(_routerEndpoints.begin(), _routerEndpoints.end()), _routerEndpoints.end());

	//
	// Associate this object adapter with the router. This way,
	// new outgoing connections to the router's client proxy will
	// use this object adapter for callbacks.
	//
	routerInfo->setAdapter(this);

	//
	// Also modify all existing outgoing connections to the
	// router's client proxy to use this object adapter for
	// callbacks.
	//	
	_instance->outgoingConnectionFactory()->setRouter(routerInfo->getRouter());
    }
}

void
Ice::ObjectAdapterI::setLocator(const LocatorPrx& locator)
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);
    
    checkForDeactivation();

    _locatorInfo = _instance->locatorManager()->get(locator);
}

bool
Ice::ObjectAdapterI::isLocal(const ObjectPrx& proxy) const
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();

    ReferencePtr ref = proxy->__reference();
    vector<EndpointPtr>::const_iterator p;

    if(!ref->adapterId.empty())
    {
	//
	// Proxy is local if the reference adapter id matches this
	// adapter id.
	//
	return ref->adapterId == _id;
    }

    //
    // Proxies which have at least one endpoint in common with the
    // endpoints used by this object adapter's incoming connection
    // factories are considered local.
    //
    for(p = ref->endpoints.begin(); p != ref->endpoints.end(); ++p)
    {
	vector<IncomingConnectionFactoryPtr>::const_iterator q;
	for(q = _incomingConnectionFactories.begin(); q != _incomingConnectionFactories.end(); ++q)
	{
	    if((*q)->equivalent(*p))
	    {
		return true;
	    }
	}
    }

    //
    // Proxies which have at least one endpoint in common with the
    // router's server proxy endpoints (if any), are also considered
    // local.
    //
    for(p = ref->endpoints.begin(); p != ref->endpoints.end(); ++p)
    {
	if(binary_search(_routerEndpoints.begin(), _routerEndpoints.end(), *p)) // _routerEndpoints is sorted.
	{
	    return true;
	}
    }
	
    return false;
}

list<ConnectionPtr>
Ice::ObjectAdapterI::getIncomingConnections() const
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    checkForDeactivation();

    list<ConnectionPtr> connections;
    vector<IncomingConnectionFactoryPtr>::const_iterator p;
    for(p = _incomingConnectionFactories.begin(); p != _incomingConnectionFactories.end(); ++p)
    {
	list<ConnectionPtr> cons = (*p)->connections();
	connections.splice(connections.end(), cons);
    }
    return connections;
}

void
Ice::ObjectAdapterI::flushBatchRequests()
{
    std::vector<IceInternal::IncomingConnectionFactoryPtr> f;
    {
	IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);
	f = _incomingConnectionFactories;
    }
    for_each(f.begin(), f.end(), Ice::voidMemFun(&IncomingConnectionFactory::flushBatchRequests));
}

void
Ice::ObjectAdapterI::incDirectCount()
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);
 
    checkForDeactivation();

    assert(_directCount >= 0);
    ++_directCount;
}

void
Ice::ObjectAdapterI::decDirectCount()
{
    IceUtil::Monitor<IceUtil::RecMutex>::Lock sync(*this);

    // Not check for deactivation here!

    assert(_instance); // Must not be called after waitForDeactivate().

    assert(_directCount > 0);
    if(--_directCount == 0)
    {
	notifyAll();
    }    
}

ThreadPoolPtr
Ice::ObjectAdapterI::getThreadPool() const
{
    // No mutex lock necessary, _threadPool and _instance are
    // immutable after creation until they are removed in
    // waitForDeactivate().

    // Not check for deactivation here!

    assert(_instance); // Must not be called after waitForDeactivate().

    if(_threadPool)
    {
	return _threadPool;
    }
    else
    {
	return _instance->serverThreadPool();
    }
}

ServantManagerPtr
Ice::ObjectAdapterI::getServantManager() const
{
    // No mutex lock necessary, _threadPool and _instance are
    // immutable after creation until they are removed in
    // waitForDeactivate().

    // Not check for deactivation here!

    assert(_instance); // Must not be called after waitForDeactivate().

    return _servantManager;
}

Ice::ObjectAdapterI::ObjectAdapterI(const InstancePtr& instance, const CommunicatorPtr& communicator,
				    const string& name) :
    _deactivated(false),
    _instance(instance),
    _communicator(communicator),
    _servantManager(new ServantManager(instance, name)),
    _printAdapterReadyDone(false),
    _name(name),
    _id(instance->properties()->getProperty(name + ".AdapterId")),
    _directCount(0),
    _waitForDeactivate(false)
{
    __setNoDelete(true);
    try
    {
	string endpts = _instance->properties()->getProperty(name + ".Endpoints");
	transform(endpts.begin(), endpts.end(), endpts.begin(), ::tolower);

	string::size_type beg;
	string::size_type end = 0;

	while(end < endpts.length())
	{
	    const string delim = " \t\n\r";
	    
	    beg = endpts.find_first_not_of(delim, end);
	    if(beg == string::npos)
	    {
		break;
	    }

	    end = endpts.find(':', beg);
	    if(end == string::npos)
	    {
		end = endpts.length();
	    }
	    
	    if(end == beg)
	    {
		++end;
                continue;
	    }
	    
	    string s = endpts.substr(beg, end - beg);
	   
	    //
	    // Don't store the endpoint in the adapter. The Collector
	    // might change it, for example, to fill in the real port
	    // number if a zero port number is given.
	    //
	    EndpointPtr endp = _instance->endpointFactoryManager()->create(s);
	    _incomingConnectionFactories.push_back(new IncomingConnectionFactory(instance, endp, this));

	    ++end;
	}

	string router = _instance->properties()->getProperty(_name + ".Router");
	if(!router.empty())
	{
	    addRouter(RouterPrx::uncheckedCast(_instance->proxyFactory()->stringToProxy(router)));
	}
	
	string locator = _instance->properties()->getProperty(_name + ".Locator");
	if(!locator.empty())
	{
	    setLocator(LocatorPrx::uncheckedCast(_instance->proxyFactory()->stringToProxy(locator)));
	}
	else
	{
	    setLocator(_instance->referenceFactory()->getDefaultLocator());
	}

	int size = _instance->properties()->getPropertyAsInt(_name + ".ThreadPool.Size");
	int sizeMax = _instance->properties()->getPropertyAsInt(_name + ".ThreadPool.SizeMax");
	if(size > 0 || sizeMax > 0)
	{
	    _threadPool = new ThreadPool(_instance, _name + ".ThreadPool", 0);
	}
    }
    catch(...)
    {
	deactivate();
	waitForDeactivate();
	__setNoDelete(false);
	throw;
    }
    __setNoDelete(false);  
}

Ice::ObjectAdapterI::~ObjectAdapterI()
{
    if(!_deactivated)
    {
	Warning out(_instance->logger());
	out << "object adapter `" << _name << "' has not been deactivated";
    }
    else if(_instance)
    {
	Warning out(_instance->logger());
	out << "object adapter `" << _name << "' deactivation had not been waited for";
    }
    else
    {
	assert(!_threadPool);
	assert(!_servantManager);
	assert(!_communicator);
	assert(_incomingConnectionFactories.empty());
	assert(_directCount == 0);
	assert(!_waitForDeactivate);
    }
}

ObjectPrx
Ice::ObjectAdapterI::newProxy(const Identity& ident) const
{
    if(_id.empty())
    {
	return newDirectProxy(ident);
    }
    else
    {
	//
	// Create a reference with the adapter id.
	//
	vector<EndpointPtr> endpoints;
	ReferencePtr ref = _instance->referenceFactory()->create(ident, Context(), vector<string>(),
								 Reference::ModeTwoway, false, _id,
								 endpoints, 0, _locatorInfo, 0, true);

	//
	// Return a proxy for the reference. 
	//
	return _instance->proxyFactory()->referenceToProxy(ref);
    }
}

ObjectPrx
Ice::ObjectAdapterI::newDirectProxy(const Identity& ident) const
{
    vector<EndpointPtr> endpoints;

    // 
    // First we add all endpoints from all incoming connection
    // factories.
    //
    transform(_incomingConnectionFactories.begin(), _incomingConnectionFactories.end(), back_inserter(endpoints),
	      Ice::constMemFun(&IncomingConnectionFactory::endpoint));
    
    //
    // Now we also add the endpoints of the router's server proxy, if
    // any. This way, object references created by this object adapter
    // will also point to the router's server proxy endpoints.
    //
    copy(_routerEndpoints.begin(), _routerEndpoints.end(), back_inserter(endpoints));
    
    //
    // Create a reference and return a proxy for this reference.
    //
    ReferencePtr ref = _instance->referenceFactory()->create(ident, Context(), vector<string>(), Reference::ModeTwoway,
							     false, "", endpoints, 0, _locatorInfo, 0, true);
    return _instance->proxyFactory()->referenceToProxy(ref);

}

void
Ice::ObjectAdapterI::checkForDeactivation() const
{
    if(_deactivated)
    {
	ObjectAdapterDeactivatedException ex(__FILE__, __LINE__);
	ex.name = _name;
	throw ex;
    }
}

void
Ice::ObjectAdapterI::checkIdentity(const Identity& ident)
{
    if(ident.name.size() == 0)
    {
        IllegalIdentityException e(__FILE__, __LINE__);
        e.id = ident;
        throw e;
    }
}
