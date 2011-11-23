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

#ifndef FREEZE_EVICTOR_I_H
#define FREEZE_EVICTOR_I_H

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <Freeze/Evictor.h>
#include <Freeze/IdentityObjectDict.h>

#include <list>

namespace Freeze
{

class EvictorI : public Evictor, public IceUtil::Mutex
{
public:

    EvictorI(const Freeze::DBPtr&, EvictorPersistenceMode);
    virtual ~EvictorI();

    virtual DBPtr getDB();

    virtual void setSize(Ice::Int);
    virtual Ice::Int getSize();
    
    virtual void createObject(const Ice::Identity&, const Ice::ObjectPtr&);
    virtual void destroyObject(const Ice::Identity&);

    virtual void installServantInitializer(const ServantInitializerPtr&);
    virtual EvictorIteratorPtr getIterator();
    virtual bool hasObject(const Ice::Identity&);

    virtual Ice::ObjectPtr locate(const Ice::Current&, Ice::LocalObjectPtr&);
    virtual void finished(const Ice::Current&, const Ice::ObjectPtr&, const Ice::LocalObjectPtr&);
    virtual void deactivate();

private:

    struct EvictorElement : public Ice::LocalObject
    {
	Ice::ObjectPtr servant;
	std::list<Ice::Identity>::iterator position;
	int usageCount;
        bool destroyed;
    };
    typedef IceUtil::Handle<EvictorElement> EvictorElementPtr;

    void evict();
    EvictorElementPtr add(const Ice::Identity&, const Ice::ObjectPtr&);
    EvictorElementPtr remove(const Ice::Identity&);
    
    std::map<Ice::Identity, EvictorElementPtr> _evictorMap;
    std::list<Ice::Identity> _evictorList;
    std::map<Ice::Identity, EvictorElementPtr>::size_type _evictorSize;

    bool _deactivated;
    IdentityObjectDict _dict;
    Freeze::DBPtr _db;
    EvictorPersistenceMode _persistenceMode;
    ServantInitializerPtr _initializer;
    int _trace;
};

}

#endif
