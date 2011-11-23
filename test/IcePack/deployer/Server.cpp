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
#include <Ice/Application.h>
#include <TestI.h>
#include <TestCommon.h>

using namespace std;

class Server : public Ice::Application
{
public:

    virtual int run(int argc, char* argv[]);

};

int
Server::run(int argc, char* argv[])
{
    Ice::PropertiesPtr properties = communicator()->getProperties();

    Ice::StringSeq args = Ice::argsToStringSeq(argc, argv);
    args = properties->parseCommandLineOptions("Test", args);
    Ice::stringSeqToArgs(args, argc, argv);

    string name = properties->getProperty("Ice.ProgramName");

    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("Server");
    Ice::ObjectPtr object = new TestI(adapter, properties);
    adapter->add(object, Ice::stringToIdentity(name));
    shutdownOnInterrupt();
    try
    {
        adapter->activate();
    }
    catch(const Ice::ObjectAdapterDeactivatedException&)
    {
    }
    communicator()->waitForShutdown();
    ignoreInterrupt();
    return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
    //
    // Test if MY_ENV_VARIABLE is set.
    //
    char* value = getenv("MY_ENV_VARIABLE");
    test(value != 0 && string(value) == "12");

    Server app;
    int rc = app.main(argc, argv);
    return rc;
}
