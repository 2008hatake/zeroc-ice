// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <IcePack/ObjectRegistryI.h>
#include <IcePack/TraceLevels.h>
#include <Freeze/Initialize.h>

using namespace std;
using namespace IcePack;

IcePack::ObjectRegistryI::ObjectRegistryI(const Ice::CommunicatorPtr& communicator,
					  const string& envName,
					  const string& objectsDbName,
					  const string& typesDbName,
					  const TraceLevelsPtr& traceLevels) :
    _connectionCache(Freeze::createConnection(communicator, envName)),
    _objectsCache(_connectionCache, objectsDbName, true),
    _typesCache(_connectionCache, typesDbName, true),
    _traceLevels(traceLevels),
    _envName(envName),
    _communicator(communicator),
    _objectsDbName(objectsDbName),
    _typesDbName(typesDbName)
{
}

void
IcePack::ObjectRegistryI::add(const ObjectDescription& obj, const Ice::Current&)
{
    IceUtil::Mutex::Lock sync(*this);

    Freeze::ConnectionPtr connection = Freeze::createConnection(_communicator, _envName);
    IdentityObjectDescDict objects(connection, _objectsDbName);
    StringObjectProxySeqDict types(connection, _typesDbName);

    Ice::Identity id = obj.proxy->ice_getIdentity();

    IdentityObjectDescDict::iterator p = objects.find(id);
    if(p != objects.end())
    {
	throw ObjectExistsException();
    }

    //
    // Add the object to the object dictionary.
    //
    objects.put(pair<const Ice::Identity, const ObjectDescription>(id, obj));

    //
    // Add the object to the interface dictionary.
    //
    if(!obj.type.empty())
    {
	Ice::ObjectProxySeq seq;
	
	StringObjectProxySeqDict::iterator q = types.find(obj.type);
	if(q != types.end())
	{
	    seq = q->second;
	}
	
	seq.push_back(obj.proxy);
	
	if(q == types.end())
	{
	    types.put(pair<const string, const Ice::ObjectProxySeq>(obj.type, seq));
	}
	else
	{
	    q.set(seq);
	}
    }

    if(_traceLevels->objectRegistry > 0)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->objectRegistryCat);
	out << "added object `" << Ice::identityToString(id) << "'";
    }
}

void
IcePack::ObjectRegistryI::remove(const Ice::ObjectPrx& object, const Ice::Current&)
{
    IceUtil::Mutex::Lock sync(*this);
    
    Freeze::ConnectionPtr connection = Freeze::createConnection(_communicator, _envName);
    IdentityObjectDescDict objects(connection, _objectsDbName);
    StringObjectProxySeqDict types(connection, _typesDbName);

    Ice::Identity id = object->ice_getIdentity();

    IdentityObjectDescDict::iterator p = objects.find(id);
    if(p == objects.end())
    {
	throw ObjectNotExistException();
    }

    ObjectDescription obj = p->second;
    
    if(!obj.type.empty())
    {
	//
	// Remove the object from the interface dictionary.
	//
	StringObjectProxySeqDict::iterator q = types.find(obj.type);
	assert(q != types.end());
	
	Ice::ObjectProxySeq seq = q->second;
	
	Ice::ObjectProxySeq::iterator r;
	for(r = seq.begin(); r != seq.end(); ++r)
	{
	    if((*r)->ice_getIdentity() == id)
	    {
		break;
	    }
	}
	
	assert(r != seq.end());
	seq.erase(r);
	
	if(seq.size() == 0)
	{
	    types.erase(q);
	}
	else
	{
	    q.set(seq);
	}
    }
    
    //
    // Remove the object from the object dictionary.
    //
    objects.erase(p);    

    if(_traceLevels->objectRegistry > 0)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->objectRegistryCat);
	out << "removed object `" << id << "'";
    }
}

ObjectDescription
IcePack::ObjectRegistryI::getObjectDescription(const Ice::Identity& id, const Ice::Current&) const
{
    Freeze::ConnectionPtr connection = Freeze::createConnection(_communicator, _envName);
    IdentityObjectDescDict objects(connection, _objectsDbName);

    IdentityObjectDescDict::iterator p = objects.find(id);
    if(p == objects.end())
    {
	throw ObjectNotExistException();
    }

    return p->second;
}

Ice::ObjectPrx
IcePack::ObjectRegistryI::findById(const Ice::Identity& id, const Ice::Current&) const
{
    Freeze::ConnectionPtr connection = Freeze::createConnection(_communicator, _envName);
    IdentityObjectDescDict objects(connection, _objectsDbName);

    IdentityObjectDescDict::iterator p = objects.find(id);
    if(p == objects.end())
    {
	throw ObjectNotExistException();
    }

    return p->second.proxy;
}

Ice::ObjectPrx
IcePack::ObjectRegistryI::findByType(const string& type, const Ice::Current&) const
{
    Freeze::ConnectionPtr connection = Freeze::createConnection(_communicator, _envName);
    StringObjectProxySeqDict types(connection, _typesDbName);

    StringObjectProxySeqDict::iterator p = types.find(type);
    if(p == types.end())
    {
	throw ObjectNotExistException();
    }

    int r = rand() % int(p->second.size());
    return p->second[r];
}

Ice::ObjectProxySeq
IcePack::ObjectRegistryI::findAllWithType(const string& type, const Ice::Current&) const
{
    Freeze::ConnectionPtr connection = Freeze::createConnection(_communicator, _envName);
    StringObjectProxySeqDict types(connection, _typesDbName);

    StringObjectProxySeqDict::iterator p = types.find(type);
    if(p == types.end())
    {
	throw ObjectNotExistException();
    }

    return p->second;
}
