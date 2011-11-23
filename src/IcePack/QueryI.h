// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_PACK_QUERY_I_H
#define ICE_PACK_QUERY_I_H

#include <IcePack/Query.h>

namespace IcePack
{

class QueryI : public Query, public IceUtil::Mutex
{
public:

    QueryI(const Ice::CommunicatorPtr&, const ObjectRegistryPtr&);
    virtual ~QueryI();

    virtual ::Ice::ObjectPrx findObjectById(const ::Ice::Identity&, const ::Ice::Current&) const;
    virtual ::Ice::ObjectPrx findObjectByType(const ::std::string&, const ::Ice::Current&) const;
    virtual ::Ice::ObjectProxySeq findAllObjectsWithType(const ::std::string&, const ::Ice::Current&) const;

private:

    Ice::CommunicatorPtr _communicator;
    ObjectRegistryPtr _objectRegistry;
};

}

#endif
