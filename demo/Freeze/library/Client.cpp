// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Application.h>
#include <Parser.h>

using namespace std;

class LibraryClient : public Ice::Application
{
    virtual int run(int argc, char* argv[]);
};

int
main(int argc, char* argv[])
{
    LibraryClient app;
    return app.main(argc, argv, "config");
}

int
LibraryClient::run(int argc, char* argv[])
{
    int runParser(int, char*[], const Ice::CommunicatorPtr&);
    return runParser(argc, argv, communicator());
}
