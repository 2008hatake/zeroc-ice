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

#include <Ice/Ice.h>
#include <IceStorm/OnewaySubscriber.h>
#include <IceStorm/TraceLevels.h>

using namespace IceStorm;
using namespace std;

OnewaySubscriber::OnewaySubscriber(const TraceLevelsPtr& traceLevels, const Ice::ObjectPrx& obj) :
    Subscriber(traceLevels, obj->ice_getIdentity()),
    _obj(obj)
{
}

OnewaySubscriber::~OnewaySubscriber()
{
}

bool
OnewaySubscriber::persistent() const
{
    return false;
}

void
OnewaySubscriber::unsubscribe()
{
    IceUtil::Mutex::Lock sync(_stateMutex);
    _state = StateUnsubscribed;

    if(_traceLevels->subscriber > 0)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->subscriberCat);
	out << "Unsubscribe " << _obj->ice_getIdentity();
    }
}

void
OnewaySubscriber::replace()
{
    IceUtil::Mutex::Lock sync(_stateMutex);
    _state = StateReplaced;

    if(_traceLevels->subscriber > 0)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->subscriberCat);
	out << "Replace " << _obj->ice_getIdentity();
    }
}

void
OnewaySubscriber::publish(const Event& event)
{
    try
    {
	std::vector< ::Ice::Byte> dummy;
	_obj->ice_invoke(event.op, ::Ice::Idempotent, event.data, dummy, event.context);
    }
    catch(const Ice::LocalException& e)
    {
	IceUtil::Mutex::Lock sync(_stateMutex);
	//
	// It's possible that the subscriber was unsubscribed, or
	// marked invalid by another thread. Don't display a
	// diagnostic in this case.
	//
	if(_state == StateActive)
	{
	    if(_traceLevels->subscriber > 0)
	    {
		Ice::Trace out(_traceLevels->logger, _traceLevels->subscriberCat);
		out << _obj->ice_getIdentity() << ": publish failed: " << e;
	    }
	    _state = StateError;
	}
    }

}
