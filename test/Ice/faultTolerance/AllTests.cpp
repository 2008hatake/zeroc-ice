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
#include <TestCommon.h>
#include <Test.h>

using namespace std;

void
allTests(const Ice::CommunicatorPtr& communicator, const vector<int>& ports)
{
    cout << "testing stringToProxy... " << flush;
    ostringstream ref;
    ref << "test";
    for(vector<int>::const_iterator p = ports.begin(); p != ports.end(); ++p)
    {
	ref << ":default -t 4000 -p " << *p;
    }
    Ice::ObjectPrx base = communicator->stringToProxy(ref.str());
    test(base);
    cout << "ok" << endl;

    cout << "testing checked cast... " << flush;
    TestPrx obj = TestPrx::checkedCast(base);
    test(obj);
    test(obj == base);
    cout << "ok" << endl;

    int oldPid = 0;
    for(unsigned int i = 1, j = 0; i <= ports.size(); ++i, j = j >= 3 ? 0 : j + 1)
    {
	cout << "testing server #" << i << "... " << flush;
	int pid = obj->pid();
	test(pid != oldPid);
	cout << "ok" << endl;
	oldPid = pid;

	if(j == 0)
	{
	    cout << "shutting down server #" << i << "... " << flush;
	    obj->shutdown();
	    cout << "ok" << endl;
	}
	else if(j == 1 || i + 1 > ports.size())
	{
	    cout << "aborting server #" << i << "... " << flush;
	    try
	    {
		obj->abort();
		test(false);
	    }
	    catch(const Ice::ConnectionLostException&)
	    {
		cout << "ok" << endl;
	    }
	}
	else if(j == 2)
	{
	    cout << "aborting server #" << i << " and #" << i + 1 << " with idempotent call... " << flush;
	    try
	    {
		obj->idempotentAbort();
		test(false);
	    }
	    catch(const Ice::ConnectionLostException&)
	    {
		cout << "ok" << endl;
	    }
	    ++i;
	}
	else if(j == 3)
	{
	    cout << "aborting server #" << i << " and #" << i + 1 << " with nonmutating call... " << flush;
	    try
	    {
		obj->nonmutatingAbort();
		test(false);
	    }
	    catch(const Ice::ConnectionLostException&)
	    {
		cout << "ok" << endl;
	    }
	    ++i;
	}
	else
	{
	    assert(false);
	}
    }

    cout << "testing whether all servers are gone... " << flush;
    try
    {
	obj->ice_ping();
	test(false);
    }
    catch(const Ice::LocalException&)
    {
	cout << "ok" << endl;
    }
}
