// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <IcePack/Admin.h>
#include <TestCommon.h>
#include <Test.h>

using namespace std;

TestPrx
allTests(const Ice::CommunicatorPtr& communicator)
{
    cout << "testing stringToProxy... " << flush;
    Ice::ObjectPrx base = communicator->stringToProxy("test @ TestAdapter");
    test(base);
    cout << "ok" << endl;

    cout << "testing checked cast... " << flush;
    TestPrx obj = TestPrx::checkedCast(base);
    test(obj);
    test(obj == base);
    cout << "ok" << endl;

    cout << "pinging server... " << flush;
    obj->ice_ping();
    cout << "ok" << endl;

    return obj;
}

TestPrx
allTestsWithDeploy(const Ice::CommunicatorPtr& communicator)
{
    cout << "testing stringToProxy... " << flush;
    Ice::ObjectPrx base = communicator->stringToProxy("test @ TestAdapter");
    test(base);
    Ice::ObjectPrx base2 = communicator->stringToProxy("test");
    test(base2);
    cout << "ok" << endl;

    cout << "testing checked cast... " << flush;
    TestPrx obj = TestPrx::checkedCast(base);
    test(obj);
    test(obj == base);
    TestPrx obj2 = TestPrx::checkedCast(base2);
    test(obj2);
    test(obj2 == base2);
    cout << "ok" << endl;

    cout << "pinging server... " << flush;
    obj->ice_ping();
    obj2->ice_ping();
    cout << "ok" << endl;

    cout << "testing reference with unknown identity... " << flush;
    try
    {
	communicator->stringToProxy("unknown/unknown")->ice_ping();
	test(false);
    }
    catch (const Ice::NotRegisteredException& ex)
    {
	test(ex.kindOfObject == "object");
	test(ex.id == "unknown/unknown");
    }
    cout << "ok" << endl;

    cout << "testing reference with unknown adapter... " << flush;
    try
    {
	communicator->stringToProxy("test @ TestAdapterUnknown")->ice_ping();
	test(false);
    }
    catch (const Ice::NotRegisteredException& ex)
    {
	test(ex.kindOfObject == "object adapter");
	test(ex.id == "TestAdapterUnknown");
    }
    cout << "ok" << endl;

    IcePack::AdminPrx admin = IcePack::AdminPrx::checkedCast(communicator->stringToProxy("IcePack/Admin"));
    test(admin);

    admin->setServerActivation("server", IcePack::Manual);
    admin->stopServer("server");

    cout << "testing whether server is still reachable... " << flush;
    try
    {
	obj = TestPrx::checkedCast(base);
	test(false);
    }
    catch(const Ice::NoEndpointException&)
    {
    }
    try
    {
	obj2 = TestPrx::checkedCast(base2);
	test(false);
    }
    catch(const Ice::NoEndpointException&)
    {
    }
    
    admin->setServerActivation("server", IcePack::OnDemand);

    try
    {
	obj = TestPrx::checkedCast(base);
    }
    catch(const Ice::NoEndpointException&)
    {
	test(false);
    }
    try
    {
	obj2 = TestPrx::checkedCast(base2);
    }
    catch(const Ice::NoEndpointException&)
    {
	test(false);
    }
    cout << "ok" << endl;    

    return obj;
}
