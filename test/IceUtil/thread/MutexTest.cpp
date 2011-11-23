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

#include <IceUtil/IceUtil.h>

#include <MutexTest.h>
#include <TestCommon.h>

using namespace std;
using namespace IceUtil;

static const string mutexTestName("mutex");

class MutexTestThread : public Thread
{
public:
    
    MutexTestThread(Mutex& m) :
	_mutex(m),
	_tryLock(false)
    {
    }

    virtual void run()
    {	
	Mutex::TryLock tlock(_mutex);
	test(!tlock.acquired());

	{
	    Mutex::Lock lock(_tryLockMutex);
	    _tryLock = true;
	}
	_tryLockCond.signal();

	Mutex::Lock lock(_mutex);
    }

    void
    waitTryLock()
    {
	Mutex::Lock lock(_tryLockMutex);
	while(!_tryLock)
	{
	    _tryLockCond.wait(lock);
	}
    }

private:

    Mutex& _mutex;
    bool _tryLock;
    //
    // Use native Condition variable here, not Monitor.
    //
    Cond _tryLockCond;
    Mutex _tryLockMutex;
};

typedef Handle<MutexTestThread> MutexTestThreadPtr;

MutexTest::MutexTest() :
    TestBase(mutexTestName)
{
}

void
MutexTest::run()
{
    Mutex mutex;
    MutexTestThreadPtr t;
    ThreadControl control;

    {
	Mutex::Lock lock(mutex);

	// LockT testing: 
	//

	test(lock.acquired());

	try
	{
	    lock.acquire();
	    test(false);
	}
	catch(const ThreadLockedException&)
	{
	    // Expected
	}

	try
	{
	    lock.tryAcquire();
	    test(false);
	}
	catch(const ThreadLockedException&)
	{
	    // Expected
	}

	test(lock.acquired());
	lock.release();
	test(!lock.acquired());

	try
	{
	    lock.release();
	    test(false);
	}
	catch(const ThreadLockedException&)
	{
	    // Expected
	}
	
	Mutex::TryLock lock2(mutex);
	test(lock.tryAcquire() == false);
	lock2.release();
	test(lock.tryAcquire() == true);
	test(lock.acquired());	

	// Deadlock testing
	//

#if !defined(NDEBUG) && !defined(_WIN32)
	try
	{
	    Mutex::Lock lock3(mutex);
	    test(false);
	}
	catch(const ThreadSyscallException& e)
	{    
	    // Expected
	    test(e.error() == EDEADLK);
	}
#endif

	// TEST: Start thread, try to acquire the mutex.
	t = new MutexTestThread(mutex);
	control = t->start();
	
	// TEST: Wait until the tryLock has been tested.
	t->waitTryLock();
    }

    //
    // TEST: Once the mutex has been released, the thread should
    // acquire the mutex and then terminate.
    //
    control.join();
}
