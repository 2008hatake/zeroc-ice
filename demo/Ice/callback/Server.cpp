// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Application.h>
#include <CallbackI.h>

using namespace std;
using namespace Ice;

class CallbackServer : public Application
{
public:

    virtual int run(int, char*[]);
};

int
main(int argc, char* argv[])
{
    CallbackServer app;
    return app.main(argc, argv, "config.server");
}

int
CallbackServer::run(int argc, char* argv[])
{
    ObjectAdapterPtr adapter = communicator()->createObjectAdapter("Callback.Server");
    CallbackPrx self = CallbackPrx::uncheckedCast(adapter->createProxy(Ice::stringToIdentity("callback")));
    adapter->add(new CallbackI, Ice::stringToIdentity("callback"));
    adapter->activate();
    communicator()->waitForShutdown();
    return EXIT_SUCCESS;
}
