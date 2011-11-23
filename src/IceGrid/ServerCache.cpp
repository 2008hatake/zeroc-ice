// **********************************************************************
//
// Copyright (c) 2003-2005 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/LoggerUtil.h>

#include <IceGrid/ServerCache.h>
#include <IceGrid/NodeCache.h>
#include <IceGrid/AdapterCache.h>
#include <IceGrid/ObjectCache.h>

using namespace std;
using namespace IceGrid;

namespace IceGrid
{

    struct AddCommunicator : std::unary_function<CommunicatorDescriptorPtr&, void>
    {
	AddCommunicator(ServerCache& serverCache, const ServerEntryPtr& entry) :
	    _serverCache(serverCache), _entry(entry)
	{
	}
	
	void
	operator()(const CommunicatorDescriptorPtr& desc)
	{
	    _serverCache.addCommunicator(desc, _entry);
	}
	
	ServerCache& _serverCache;
	const ServerEntryPtr _entry;
    };

    struct RemoveCommunicator : std::unary_function<CommunicatorDescriptorPtr&, void>
    {
	RemoveCommunicator(ServerCache& serverCache, const ServerEntryPtr& entry) : 
	    _serverCache(serverCache), _entry(entry)
	{
	}

	void
	operator()(const CommunicatorDescriptorPtr& desc)
	{
	    _serverCache.removeCommunicator(desc, _entry);
	}

	ServerCache& _serverCache;
	const ServerEntryPtr _entry;
    };
}

ServerCache::ServerCache(NodeCache& nodeCache, 
			 AdapterCache& adapterCache, 
			 ObjectCache& objectCache) :
    _nodeCache(nodeCache), 
    _adapterCache(adapterCache), 
    _objectCache(objectCache)
{
}

ServerEntryPtr
ServerCache::add(const ServerInfo& info)
{
    Lock sync(*this);

    ServerEntryPtr entry = getImpl(info.descriptor->id, true);
    entry->update(info);
    _nodeCache.get(info.node, true)->addServer(entry);

    forEachCommunicator(AddCommunicator(*this, entry))(info.descriptor);

    if(_traceLevels && _traceLevels->server > 0)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	out << "added server `" << info.descriptor->id << "'";	
    }

    return entry;
}

ServerEntryPtr
ServerCache::get(const string& id)
{
    Lock sync(*this);
    ServerEntryPtr entry = getImpl(id);
    if(!entry)
    {
	ServerNotExistException ex;
	ex.id = id;
	throw ex;
    }
    return entry;
}

bool
ServerCache::has(const string& id)
{
    Lock sync(*this);
    ServerEntryPtr entry = getImpl(id);
    return entry && !entry->isDestroyed();
}

ServerEntryPtr
ServerCache::remove(const string& id, bool destroy)
{
    Lock sync(*this);

    ServerEntryPtr entry = getImpl(id);
    ServerInfo info = entry->getServerInfo();
    if(destroy)
    {
	entry->destroy();
    }

    _nodeCache.get(info.node)->removeServer(entry);

    forEachCommunicator(RemoveCommunicator(*this, entry))(info.descriptor);

    if(_traceLevels && _traceLevels->server > 0)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->serverCat);
	out << "removed server `" << id << "'";	
    }

    return entry;
}

void
ServerCache::clear(const string& id)
{
    Lock sync(*this);
    CacheByString<ServerEntry>::removeImpl(id);
}

NodeCache&
ServerCache::getNodeCache() const
{
    return _nodeCache;
}

void
ServerCache::addCommunicator(const CommunicatorDescriptorPtr& comm, const ServerEntryPtr& entry)
{
    const string application = entry->getApplication();
    for(AdapterDescriptorSeq::const_iterator q = comm->adapters.begin() ; q != comm->adapters.end(); ++q)
    {
	if(!q->id.empty())
	{
	    _adapterCache.getServerAdapter(q->id, true)->set(entry, q->replicaGroupId);
	}

	for(ObjectDescriptorSeq::const_iterator r = q->objects.begin(); r != q->objects.end(); ++r)
	{
	    const string edpts = IceGrid::getProperty(comm->properties, q->name + ".Endpoints");
	    _objectCache.add(application, q->id, edpts, *r);
	}
    }
}

void
ServerCache::removeCommunicator(const CommunicatorDescriptorPtr& comm, const ServerEntryPtr& entry)
{
    for(AdapterDescriptorSeq::const_iterator q = comm->adapters.begin() ; q != comm->adapters.end(); ++q)
    {
	if(!q->id.empty())
	{
	    _adapterCache.getServerAdapter(q->id)->destroy();
	}
	for(ObjectDescriptorSeq::const_iterator r = q->objects.begin(); r != q->objects.end(); ++r)
	{
	    _objectCache.remove(r->id);
	}
    }
}

ServerEntry::ServerEntry(Cache<string, ServerEntry>& cache, const string& id) :
    _cache(*dynamic_cast<ServerCache*>(&cache)), 
    _id(id),
    _synchronizing(false),
    _updated(false)
{
}

void
ServerEntry::sync()
{
    try
    {
	syncImpl(true);
    }
    catch(const Ice::Exception&)
    {
    }
}

void
ServerEntry::update(const ServerInfo& info)
{
    Lock sync(*this);

    auto_ptr<ServerInfo> descriptor(new ServerInfo());
    *descriptor = info;

    _updated = true;

    if(!_destroy.get())
    {
	if(_loaded.get() && descriptor->node != _loaded->node)
	{
	    _destroy = _loaded;
	}
	else if(_load.get() && descriptor->node != _load->node)
	{
	    _destroy = _load;
	}
    }

    _load = descriptor;
    _loaded.reset(0);
    _proxy = 0;
    _adapters.clear();
}

void
ServerEntry::destroy()
{
    Lock sync(*this);

    _updated = true;

    assert(_loaded.get() || _load.get());
    if(!_destroy.get())
    {
	if(_loaded.get())
	{
	    assert(!_destroy.get());
	    _destroy = _loaded;
	}
	else if(_load.get())
	{
	    assert(!_destroy.get());
	    _destroy = _load;
	}
    }
    
    _load.reset(0);
    _loaded.reset(0);
    _proxy = 0;
    _adapters.clear();
}

ServerInfo
ServerEntry::getServerInfo(bool resolve) const
{
    ServerInfo info;
    {
	Lock sync(*this);
	if(!_loaded.get() && !_load.get())
	{
	    throw ServerNotExistException();
	}
	info = _proxy ? *_loaded : *_load;
    }
    assert(info.descriptor);
    if(resolve)
    {
	try
	{
	    return _cache.getNodeCache().get(info.node)->getServerInfo(info);
	}
	catch(const NodeNotExistException&)
	{
	}
	catch(const NodeUnreachableException&)
	{
	}
    }
    return info;
}

string
ServerEntry::getId() const
{
    return _id;
}

ServerPrx
ServerEntry::getProxy(int& activationTimeout, int& deactivationTimeout, string& node)
{
    ServerPrx proxy;
    {
	Lock sync(*this);
	if(_proxy) // Synced
	{
	    proxy = _proxy;
	    activationTimeout = _activationTimeout;
	    deactivationTimeout = _deactivationTimeout;
	    node = _loaded->node;
	}
    }

    if(proxy)
    {
	try
	{
	    proxy->ice_ping();
	    return proxy;
	}
	catch(const Ice::LocalException&)
	{
	}
    }

    syncImpl(true);

    {
	Lock sync(*this);
	if(!_proxy)
	{
	    throw ServerNotExistException(_id);
	}
	activationTimeout = _activationTimeout;
	deactivationTimeout = _deactivationTimeout;
	node = _loaded->node;
	return _proxy;
    }
}

AdapterPrx
ServerEntry::getAdapter(const string& id)
{
    AdapterPrx proxy;

    {
	Lock sync(*this);
	if(_proxy) // Synced
	{
	    proxy = _adapters[id];
	}
    }

    if(proxy)
    {
	try
	{
	    proxy->ice_ping();
	    return proxy;
	}
	catch(const Ice::LocalException&)
	{
	}
    }

    syncImpl(true);

    {    
	Lock sync(*this);
	proxy = _adapters[id];
	if(!proxy)
	{
	    throw AdapterNotExistException(id);
	}
	return proxy;
    }
}

NodeEntryPtr
ServerEntry::getNode() const
{
    Lock sync(*this);
    if(!_loaded.get() && !_load.get())
    {
	throw ServerNotExistException();
    }
    return _proxy ? _cache.getNodeCache().get(_loaded->node) : _cache.getNodeCache().get(_load->node);
}

string
ServerEntry::getApplication() const
{
    Lock sync(*this);
    if(!_loaded.get() && !_load.get())
    {
	throw ServerNotExistException();
    }
    return _proxy ? _loaded->application : _load->application;
}

float
ServerEntry::getLoad(LoadSample sample) const
{
    Lock sync(*this);
    if(!_loaded.get() && !_load.get())
    {
	throw ServerNotExistException();
    }

    ServerInfo& info = _proxy ? *_loaded.get() : *_load.get();
    float factor;
    LoadInfo load = _cache.getNodeCache().get(info.node)->getLoadInfoAndLoadFactor(info.application, factor);
    switch(sample)
    {
    case LoadSample1:
	return load.avg1 < 0.f ? 1.0f : load.avg1 * factor;
    case LoadSample5:
	return load.avg5 < 0.f ? 1.0f : load.avg5 * factor;
    case LoadSample15:
	return load.avg15 < 0.f ? 1.0f : load.avg15 * factor;
    default:
	assert(false);
	return 1.0f;
    }
}

void
ServerEntry::syncImpl(bool waitForUpdate)
{
    ServerInfo load;
    ServerInfo destroy;

    {
	Lock sync(*this);
	if(_synchronizing)
	{
	    if(waitForUpdate)
	    {
		while(_synchronizing)
		{
		    wait();
		}
	    }
	    else
	    {
		return;
	    }
	}

	if(!_load.get() && !_destroy.get())
	{
	    _load = _loaded; // Re-load the current server.
	    _proxy = 0;
	    _adapters.clear();
	}

	_updated = false;
	_exception.reset(0);

	if(_destroy.get())
	{
	    destroy = *_destroy;
	}
	else if(_load.get())
	{
	    load = *_load;
	}
	else
	{
	    return;
	}

	_synchronizing = true;
    }
    
    if(destroy.descriptor)
    {
	try
	{
	    _cache.getNodeCache().get(destroy.node)->destroyServer(this, destroy.descriptor->id);
	}
	catch(NodeNotExistException&)
	{
	    exception(NodeUnreachableException(destroy.node, "node is not active"));
	}    
    }
    else if(load.descriptor)
    {
	try
	{
	    _cache.getNodeCache().get(load.node)->loadServer(this, load);
	}
	catch(NodeNotExistException&)
	{
	    exception(NodeUnreachableException(load.node, "node is not active"));
	}
    }
    
    if(waitForUpdate)
    {
	Lock sync(*this);
	while(_synchronizing)
	{
	    wait();
	}
	if(_exception.get())
	{
	    _exception->ice_throw();
	}
    }
}

void
ServerEntry::loadCallback(const ServerPrx& proxy, const AdapterPrxDict& adpts, int at, int dt)
{
    ServerInfo load;
    ServerInfo destroy;
    {
	Lock sync(*this);
	if(!_updated)
	{
	    //
	    // Set timeout on server and adapter proxies. Most of the
	    // calls on the proxies shouldn't block for longer than the
	    // node session timeout. Calls that might block for a longer
	    // time should set the correct timeout before invoking on the
	    // proxy (e.g.: server start/stop, adapter activate).
	    //
	    _loaded = _load;
	    assert(_loaded.get());
	    int timeout = _cache.getNodeCache().getSessionTimeout() * 1000; // sec to ms
	    _proxy = ServerPrx::uncheckedCast(proxy->ice_timeout(timeout)->ice_collocationOptimization(false));
	    _adapters.clear();
	    for(AdapterPrxDict::const_iterator p = adpts.begin(); p != adpts.end(); ++p)
	    {
		Ice::ObjectPrx adapter = p->second->ice_timeout(timeout)->ice_collocationOptimization(false);
		_adapters.insert(make_pair(p->first, AdapterPrx::uncheckedCast(adapter)));
	    }
	    _activationTimeout = at + timeout;
	    _deactivationTimeout = dt + timeout;

	    assert(!_destroy.get() && !_load.get());
	    _synchronizing = false;
	    notifyAll();
	    return;
	}
	else
	{
	    _updated = false;
	    if(_destroy.get())
	    {
		destroy = *_destroy;
	    }
	    else if(_load.get())
	    {
		load = *_load;
	    }
	}
    }

    assert(destroy.descriptor || load.descriptor);
    if(destroy.descriptor)
    {
	try
	{
	    _cache.getNodeCache().get(destroy.node)->destroyServer(this, destroy.descriptor->id);
	}
	catch(NodeNotExistException&)
	{
	    exception(NodeUnreachableException(destroy.node, "node is not active"));
	}    
    }
    else if(load.descriptor)
    {
	try
	{
	    _cache.getNodeCache().get(load.node)->loadServer(this, load);
	}
	catch(NodeNotExistException&)
	{
	    exception(NodeUnreachableException(load.node, "node is not active"));
	}
    }
}

void
ServerEntry::destroyCallback()
{
    ServerInfo load;
    {
	Lock sync(*this);
	_destroy.reset(0);

	if(!_load.get())
	{
	    assert(!_load.get() && !_loaded.get());
	    _synchronizing = false;
	    notifyAll();
	}
	else
	{
	    _updated = false;
	    load = *_load;
	}
    }

    if(load.descriptor)
    {
	try
	{
	    _cache.getNodeCache().get(load.node)->loadServer(this, load);
	}
	catch(NodeNotExistException&)
	{
	    exception(NodeUnreachableException(load.node, "node is not active"));
	}
    }
    else
    {
	_cache.clear(_id);
    }
}

void
ServerEntry::exception(const Ice::Exception& ex)
{
    ServerInfo load;
    bool remove = false;
    {
	Lock sync(*this);
	if((_destroy.get() && !_load.get()) || (!_destroy.get() && !_updated))
	{
	    remove = _destroy.get();
	    _destroy.reset(0);
	    _exception.reset(ex.ice_clone());
	    _synchronizing = false;
	    notifyAll();
	}
	else
	{
	    _destroy.reset(0);
	    _updated = false;
	    load = *_load.get();
	}
    }

    if(load.descriptor)
    {
	try
	{
	    _cache.getNodeCache().get(load.node)->loadServer(this, load);
	}
	catch(NodeNotExistException&)
	{
	    exception(NodeUnreachableException(load.node, "node is not active"));
	}
    }
    else if(remove)
    {
	_cache.clear(_id);
    }
}

bool
ServerEntry::isDestroyed()
{
     Lock sync(*this);
     return !_loaded.get() && !_load.get();
}

bool
ServerEntry::canRemove()
{
     Lock sync(*this);
     return !_loaded.get() && !_load.get() && !_destroy.get();
}
