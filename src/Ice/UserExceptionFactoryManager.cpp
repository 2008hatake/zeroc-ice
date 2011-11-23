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

#include <Ice/UserExceptionFactoryManager.h>
#include <Ice/UserExceptionFactory.h>
#include <Ice/Functional.h>
#include <Ice/LocalException.h>

using namespace std;
using namespace Ice;
using namespace IceInternal;

void IceInternal::incRef(UserExceptionFactoryManager* p) { p->__incRef(); }
void IceInternal::decRef(UserExceptionFactoryManager* p) { p->__decRef(); }

void
IceInternal::UserExceptionFactoryManager::add(const UserExceptionFactoryPtr& factory, const string& id)
{
    IceUtil::Mutex::Lock sync(*this);

    if(_factoryMap.find(id) != _factoryMap.end())
    {
	AlreadyRegisteredException ex(__FILE__, __LINE__);
	ex.kindOfObject = "user exception factory";
	ex.id = id;
	throw ex;
    }

    _factoryMapHint = _factoryMap.insert(_factoryMapHint, make_pair(id, factory));
}

void
IceInternal::UserExceptionFactoryManager::remove(const string& id)
{
    UserExceptionFactoryPtr factory;

    {
	IceUtil::Mutex::Lock sync(*this);
	
	map<string, UserExceptionFactoryPtr>::iterator p = _factoryMap.find(id);
	if(p == _factoryMap.end())
	{
	    NotRegisteredException ex(__FILE__, __LINE__);
	    ex.kindOfObject = "user exception factory";
	    ex.id = id;
	    throw ex;
	}
	
	factory = p->second;

	_factoryMap.erase(p);
	_factoryMapHint = _factoryMap.end();
    }
    
    factory->destroy();
}

UserExceptionFactoryPtr
IceInternal::UserExceptionFactoryManager::find(const string& id)
{
    IceUtil::Mutex::Lock sync(*this);
    
    if(_factoryMapHint != _factoryMap.end())
    {
	if(_factoryMapHint->first == id)
	{
	    return _factoryMapHint->second;
	}
    }
    
    map<string, UserExceptionFactoryPtr>::iterator p = _factoryMap.find(id);
    if(p != _factoryMap.end())
    {
	_factoryMapHint = p;
	return p->second;
    }
    else
    {
	return 0;
    }
}

IceInternal::UserExceptionFactoryManager::UserExceptionFactoryManager() :
    _factoryMapHint(_factoryMap.end())
{
}

void
IceInternal::UserExceptionFactoryManager::destroy()
{
    IceUtil::Mutex::Lock sync(*this);
    for_each(_factoryMap.begin(), _factoryMap.end(),
	     Ice::secondVoidMemFun<string, UserExceptionFactory>(&UserExceptionFactory::destroy));
    _factoryMap.clear();
    _factoryMapHint = _factoryMap.end();
}
