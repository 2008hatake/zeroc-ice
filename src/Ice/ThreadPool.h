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

#ifndef ICE_THREAD_POOL_H
#define ICE_THREAD_POOL_H

#include <IceUtil/Shared.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/Monitor.h>
#include <IceUtil/Thread.h>

#include <Ice/ThreadPoolF.h>
#include <Ice/InstanceF.h>
#include <Ice/LoggerF.h>
#include <Ice/PropertiesF.h>
#include <Ice/EventHandlerF.h>
#include <list>

#ifndef _WIN32
#   define SOCKET int
#endif

namespace IceInternal
{

class BasicStream;

class ThreadPool : public IceUtil::Shared, public IceUtil::Mutex
{
public:

    ThreadPool(const InstancePtr&, const std::string&, int);
    virtual ~ThreadPool();

    void destroy();

    void _register(SOCKET, const EventHandlerPtr&);
    void unregister(SOCKET);
    void promoteFollower();
    void joinWithAllThreads();
    
private:

    void initiateShutdown(); // Signal-safe shutdown initiation.
    bool clearInterrupt();
    void setInterrupt(char);

    bool run(); // Returns true if a follower should be promoted.
    void read(const EventHandlerPtr&);

    InstancePtr _instance;
    bool _destroyed;
    const std::string _prefix;

    SOCKET _maxFd;
    SOCKET _minFd;
    SOCKET _lastFd;
    SOCKET _fdIntrRead;
    SOCKET _fdIntrWrite;
    fd_set _fdSet;

    std::list<std::pair<SOCKET, EventHandlerPtr> > _changes; // Event handler set for addition; null for removal.

    std::map<SOCKET, EventHandlerPtr> _handlerMap;

    int _timeout;

    class EventHandlerThread : public IceUtil::Thread
    {
    public:
	
	EventHandlerThread(const ThreadPoolPtr&);
	virtual void run();

    private:

	ThreadPoolPtr _pool;
    };
    friend class EventHandlerThread;

    const int _size; // Number of threads that are pre-created.
    const int _sizeMax; // Maximum number of threads.
    const int _sizeWarn; // If _inUse reaches _sizeWarn, a "low on threads" warning will be printed.

    const int _messageSizeMax;

    std::vector<IceUtil::ThreadControl> _threads; // Control for all threads, running or not.
    int _running; // Number of running threads.
    int _inUse; // Number of threads that are currently in use.
    double _load; // Current load in number of threads.

    const bool _warnUdp;

    IceUtil::Mutex _promoteMutex;
};

}

#endif
