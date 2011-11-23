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

#ifndef TOPIC_I_H
#define TOPIC_I_H

#include <IceUtil/RecMutex.h>
#include <IceStorm/IceStormInternal.h>
#include <IceStorm/IdentityLinkDict.h>
#include <IceStorm/SubscriberFactory.h>

namespace IceStorm
{

//
// Forward declarations.
//
class TopicSubscribers;
typedef IceUtil::Handle<TopicSubscribers> TopicSubscribersPtr;

class TraceLevels;
typedef IceUtil::Handle<TraceLevels> TraceLevelsPtr;

class SubscriberFactory;
typedef IceUtil::Handle<SubscriberFactory> SubscriberFactoryPtr;

//
// TopicInternal implementation.
//
class TopicI : public TopicInternal, public IceUtil::RecMutex
{
public:

    TopicI(const Ice::ObjectAdapterPtr&, const TraceLevelsPtr&, const std::string&, const SubscriberFactoryPtr&,
	   const Freeze::DBPtr&);
    ~TopicI();

    virtual std::string getName(const Ice::Current&) const;
    virtual Ice::ObjectPrx getPublisher(const Ice::Current&) const;
    virtual void destroy(const Ice::Current&);
    virtual void link(const TopicPrx&, Ice::Int, const Ice::Current&);
    virtual void unlink(const TopicPrx&, const Ice::Current&);
    virtual LinkInfoSeq getLinkInfoSeq(const Ice::Current&) const;

    virtual TopicLinkPrx getLinkProxy(const Ice::Current&);

    // Internal methods
    bool destroyed() const;
    void subscribe(const Ice::ObjectPrx&, const QoS&);
    void unsubscribe(const Ice::ObjectPrx&);

    void reap();

private:

    //
    // Immutable members.
    //
    Ice::ObjectAdapterPtr _adapter;
    TraceLevelsPtr _traceLevels;
    std::string _name; // The topic name
    SubscriberFactoryPtr _factory;

    Ice::ObjectPtr _publisher; // Publisher & associated proxy
    Ice::ObjectPrx _publisherPrx;

    Ice::ObjectPtr _link; // TopicLink & associated proxy
    TopicLinkPrx _linkPrx;

    //
    // Mutable members. Protected by *this
    //
    bool _destroyed; // Has this Topic been destroyed?

    TopicSubscribersPtr _subscribers; // Set of Subscribers

    IdentityLinkDict _links; // The database of Topic links
    Freeze::DBPtr _linksDb;
};

typedef IceUtil::Handle<TopicI> TopicIPtr;

} // End namespace IceStorm

#endif
