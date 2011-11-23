// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <TestCommon.h>
#include <Test.h>

using namespace std;

int
run(int argc, char* argv[], const Ice::CommunicatorPtr& communicator)
{
    bool withDeploy = false;
    
    for(int i = 1; i < argc; ++i)
    {
	if(strcmp(argv[i], "--with-deploy") == 0)
	{
	    withDeploy = true;
	    break;
	}
    }

    TestPrx obj;

    if(!withDeploy)
    {
	TestPrx allTests(const Ice::CommunicatorPtr&);
	obj = allTests(communicator);

	cout << "shutting down server... " << flush;
	obj->shutdown();
	cout << "ok" << endl;
    }
    else
    {
	TestPrx allTestsWithDeploy(const Ice::CommunicatorPtr&);
	obj = allTestsWithDeploy(communicator);
    }

    return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
    int status;
    Ice::CommunicatorPtr communicator;

    try
    {
	communicator = Ice::initialize(argc, argv);
	status = run(argc, argv, communicator);
    }
    catch(const Ice::Exception& ex)
    {
	cerr << ex << endl;
	status = EXIT_FAILURE;
    }

    if(communicator)
    {
	try
	{
	    communicator->destroy();
	}
	catch(const Ice::Exception& ex)
	{
	    cerr << ex << endl;
	    status = EXIT_FAILURE;
	}
    }

    return status;
}
