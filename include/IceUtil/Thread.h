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

#ifndef ICE_UTIL_THREAD_H
#define ICE_UTIL_THREAD_H

#include <IceUtil/Shared.h>
#include <IceUtil/Handle.h>

namespace IceUtil
{

class Time;

#ifdef _WIN32
struct HandleWrapper : public Shared
{
    // Inline for performance reasons.
    HandleWrapper(HANDLE h) :
	handle(h)
    {
    }

    // Inline for performance reasons.
    virtual ~HandleWrapper()
    {
	if(handle)
	{
	    CloseHandle(handle);
	}
    }

    HANDLE handle;
};

typedef Handle<HandleWrapper> HandleWrapperPtr;
#endif

class ICE_UTIL_API ThreadControl
{
public:

    ThreadControl();

#ifdef _WIN32
    ThreadControl(const HandleWrapperPtr&, unsigned);
#else
    ThreadControl(pthread_t);
#endif

    bool operator==(const ThreadControl&) const;
    bool operator!=(const ThreadControl&) const;
    bool operator<(const ThreadControl&) const;

    //
    // Wait until the controlled thread terminates. The call has POSIX
    // semantics.
    //
    // At most one thread can wait for the termination of a given
    // thread.C alling join on a thread on which another thread is
    // already waiting for termination results in undefined behaviour.
    // Joining with a thread after having joined with it previously,
    // or joining with a detached thread raises ThreadSyscallException.
    //
    void join();

    //
    // Detach a thread. Once a thread is detached, it cannot be detached
    // again, nor can it be joined with. Every thread must either be
    // joined with or detached exactly once. Detaching a thread a second
    // time, or detaching a thread that was previously joined with raises
    // ThreadSyscallException.
    //
    void detach();

    static void sleep(const Time&);
    static void yield();

private:

#ifdef _WIN32
    HandleWrapperPtr _handle;
    unsigned _id;
#else
    pthread_t _id;
#endif
    bool _detached;
};

class ICE_UTIL_API Thread : virtual public IceUtil::Shared
{
public:
#ifdef _WIN32
    typedef unsigned ThreadId;
#else
    typedef pthread_t ThreadId;
#endif

    Thread();
    virtual ~Thread();

    ThreadId id() const;

    virtual void run() = 0;

    ThreadControl start();

    ThreadControl getThreadControl() const;

    bool operator==(const Thread&) const;
    bool operator!=(const Thread&) const;
    bool operator<(const Thread&) const;

private:

    bool _started;
#ifdef _WIN32
    unsigned _id;
    HandleWrapperPtr _handle;
#else
    pthread_t _id;
#endif
};

typedef Handle<Thread> ThreadPtr;
    
}

#endif

