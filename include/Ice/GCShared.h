// **********************************************************************
//
// Copyright (c) 2003-2005 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_GC_SHARED_H
#define ICE_GC_SHARED_H

#include <IceUtil/Config.h>
#include <Ice/GCRecMutex.h>
#include <set>

namespace IceInternal
{

class GC;
class GCShared;

typedef std::set<GCShared*> GCObjectSet;
extern ICE_API GCObjectSet gcObjects; // Set of pointers to all existing classes with class data members.

typedef std::multiset<GCShared*> GCObjectMultiSet;

class ICE_API GCShared
{
public:

    GCShared();
    GCShared(const GCShared&);
    virtual ~GCShared() {}

    GCShared& operator=(const GCShared&)
    {
        return *this;
    }

    virtual void __incRef(); // First derived class with class data members overrides this.
    virtual void __decRef(); // Ditto.
    virtual int __getRef() const;
    virtual void __setNoDelete(bool);

    int __getRefUnsafe() const
    {
	return _ref;
    }

    void __decRefUnsafe()
    {
	--_ref;
    }

    virtual void __gcReachable(GCObjectMultiSet&) const = 0;
    virtual void __gcClear() = 0;

protected:

    static void __addObject(GCObjectMultiSet&, GCShared*);

    int _ref;
    bool _noDelete;

    friend class IceInternal::GC; // Allows IceInternal::GC to read value of _ref.
};

}

#endif
