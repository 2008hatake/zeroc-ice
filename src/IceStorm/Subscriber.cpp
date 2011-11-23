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

#include <IceStorm/Subscriber.h>
#include <IceStorm/TraceLevels.h>

using namespace IceStorm;
using namespace std;

Subscriber::Subscriber(const TraceLevelsPtr& traceLevels, const Ice::Identity& id) :
    _traceLevels(traceLevels),
    _state(StateActive),
    _id(id)
{
}

Subscriber::~Subscriber()
{
}

bool
Subscriber::inactive() const
{
    IceUtil::Mutex::Lock sync(_stateMutex);
    return _state != StateActive;;
}

bool
Subscriber::error() const
{
    IceUtil::Mutex::Lock sync(_stateMutex);
    return _state == StateError;
}


Ice::Identity
Subscriber::id() const
{
    return _id;
}
