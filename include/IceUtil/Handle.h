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

#ifndef ICE_UTIL_HANDLE_H
#define ICE_UTIL_HANDLE_H

#include <IceUtil/Exception.h>
#include <algorithm>

//
// "Handle" or "smart pointer" class for classes derived from
// IceUtil::GCShared, IceUtil::Shared or IceUtil::SimpleShared.
//
namespace IceUtil
{

template<typename T>
class HandleBase
{
public:

    typedef T element_type;
    
    T* get() const
    {
	return _ptr;
    }

    T* operator->() const
    {
	if(!_ptr)
	{
	    throw NullHandleException(__FILE__, __LINE__);	    
	}

	return _ptr;
    }

    T& operator*() const
    {
	if(!_ptr)
	{
	    throw NullHandleException(__FILE__, __LINE__);	    
	}

	return *_ptr;
    }

    operator bool() const
    {
	return _ptr ? true : false;
    }

    void swap(HandleBase& other)
    {
	std::swap(_ptr, other._ptr);
    }

    T* _ptr;
};

template<typename T, typename U>
inline bool operator==(const HandleBase<T>& lhs, const HandleBase<U>& rhs)
{
    T* l = lhs.get();
    U* r = rhs.get();
    if(l && r)
    {
	return *l == *r;
    }
    else
    {
	return !l && !r;
    }	
}

template<typename T, typename U>
inline bool operator!=(const HandleBase<T>& lhs, const HandleBase<U>& rhs)
{
    T* l = lhs.get();
    U* r = rhs.get();
    if(l && r)
    {
	return *l != *r;
    }
    else
    {
	return l || r;
    }	
}

template<typename T, typename U>
inline bool operator<(const HandleBase<T>& lhs, const HandleBase<U>& rhs)
{
    T* l = lhs.get();
    U* r = rhs.get();
    if(l && r)
    {
	return *l < *r;
    }
    else
    {
	return !l && r;
    }
}

template<typename T>
class Handle : public HandleBase<T>
{
public:
    
    Handle(T* p = 0)
    {
	_ptr = p;

	if(_ptr)
	{
	    _ptr->__incRef();
	}
    }
    
    template<typename Y>
    Handle(const Handle<Y>& r)
    {
	_ptr = r._ptr;

	if(_ptr)
	{
	    _ptr->__incRef();
	}
    }

#ifdef _WIN32 // COMPILERBUG: Is VC++ or GNU C++ right here???
    template<>
    Handle(const Handle<T>& r)
#else
    Handle(const Handle& r)
#endif
    {
	_ptr = r._ptr;

	if(_ptr)
	{
	    _ptr->__incRef();
	}
    }
    
    ~Handle()
    {
	if(_ptr)
	{
	    _ptr->__decRef();
	}
    }
    
    Handle& operator=(T* p)
    {
	if(_ptr != p)
	{
	    if(p)
	    {
		p->__incRef();
	    }

	    T* ptr = _ptr;
	    _ptr = p;

	    if(ptr)
	    {
		ptr->__decRef();
	    }
	}
	return *this;
    }
        
    template<typename Y>
    Handle& operator=(const Handle<Y>& r)
    {
	if(_ptr != r._ptr)
	{
	    if(r._ptr)
	    {
		r._ptr->__incRef();
	    }

	    T* ptr = _ptr;
	    _ptr = r._ptr;

	    if(ptr)
	    {
		ptr->__decRef();
	    }
	}
	return *this;
    }

#ifdef _WIN32 // COMPILERBUG: Is VC++ or GNU C++ right here???
    template<>
    Handle& operator=(const Handle<T>& r)
#else
    Handle& operator=(const Handle& r)
#endif
    {
	if(_ptr != r._ptr)
	{
	    if(r._ptr)
	    {
		r._ptr->__incRef();
	    }

	    T* ptr = _ptr;
	    _ptr = r._ptr;

	    if(ptr)
	    {
		ptr->__decRef();
	    }
	}
	return *this;
    }
        
    template<class Y>
    static Handle dynamicCast(const HandleBase<Y>& r)
    {
	return Handle(dynamic_cast<T*>(r._ptr));
    }

    template<class Y>
    static Handle dynamicCast(Y* p)
    {
	return Handle(dynamic_cast<T*>(p));
    }
};

}

#endif
