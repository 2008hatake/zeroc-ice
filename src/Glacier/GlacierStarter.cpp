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

#ifdef _WIN32
#   error Sorry, the Glacier Starter is not yet supported on WIN32.
#endif

#include <Ice/Application.h>
#include <Glacier/StarterI.h>
#include <signal.h>
#include <sys/wait.h>
#include <fstream>

using namespace std;
using namespace Ice;

namespace Glacier
{

class RouterApp : public Application
{
public:

    void usage();
    virtual int run(int, char*[]);
};

};

void
Glacier::RouterApp::usage()
{
    cerr << "Usage: " << appName() << " [options]\n";
    cerr <<     
        "Options:\n"
        "-h, --help           Show this message.\n"
        "-v, --version        Display the Ice version.\n"
        ;
}

int
Glacier::RouterApp::run(int argc, char* argv[])
{
    for(int i = 1; i < argc; ++i)
    {
	if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
	{
	    usage();
	    return EXIT_SUCCESS;
	}
	else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
	{
	    cout << ICE_STRING_VERSION << endl;
	    return EXIT_SUCCESS;
	}
	else
	{
	    cerr << appName() << ": unknown option `" << argv[i] << "'" << endl;
	    usage();
	    return EXIT_FAILURE;
	}
    }

    PropertiesPtr properties = communicator()->getProperties();

    //
    // Initialize the object adapter (and make sure this object
    // adapter doesn't register itself with the locator).
    // 
    const char* endpointsProperty = "Glacier.Starter.Endpoints";
    if(properties->getProperty(endpointsProperty).empty())
    {
	cerr << appName() << ": property `" << endpointsProperty << "' is not set" << endl;
	return EXIT_FAILURE;
    }
    ObjectAdapterPtr adapter = communicator()->createObjectAdapter("Glacier.Starter");

    //
    // Get the password verifier, or create one if no verifier is
    // specified.
    //
    string verifierProperty = properties->getProperty("Glacier.Starter.PasswordVerifier");
    PasswordVerifierPrx verifier;
    if(!verifierProperty.empty())
    {
	verifier = PasswordVerifierPrx::checkedCast(communicator()->stringToProxy(verifierProperty));
	if(!verifier)
	{
	    cerr << appName() << ": password verifier `" << verifierProperty << "' is invalid" << endl;
	    return EXIT_FAILURE;
	}
    }
    else
    {
	string passwordsProperty = properties->getPropertyWithDefault("Glacier.Starter.CryptPasswords", "passwords");

	ifstream passwordFile(passwordsProperty.c_str());
	if(!passwordFile)
	{
	    cerr << appName() << ": cannot open `" << passwordsProperty << "' for reading: " << strerror(errno)
		 << endl;
	    return EXIT_FAILURE;
	}

	map<string, string> passwords;

	while(true)
	{
	    string userId;
	    passwordFile >> userId;
	    if(!passwordFile)
	    {
		break;
	    }

	    string password;
	    passwordFile >> password;
	    if(!passwordFile)
	    {
		break;
	    }

	    assert(!userId.empty());
	    assert(!password.empty());
	    passwords.insert(make_pair(userId, password));
	}

	PasswordVerifierPtr verifierImpl = new CryptPasswordVerifierI(passwords);
	verifier = PasswordVerifierPrx::uncheckedCast(adapter->addWithUUID(verifierImpl));
    }

    //
    // Create and initialize the starter object.
    //
    StarterPtr starter = new StarterI(communicator(), verifier);
    adapter->add(starter, stringToIdentity("Glacier/starter"));

    //
    // Everything ok, let's go.
    //
    shutdownOnInterrupt();
    adapter->activate();
    communicator()->waitForShutdown();
    ignoreInterrupt();

    //
    // Destroy the starter.
    //
    StarterI* st = dynamic_cast<StarterI*>(starter.get());
    assert(st);
    st->destroy();

    return EXIT_SUCCESS;
}

static void
childHandler(int)
{
    wait(0);
}

int
main(int argc, char* argv[])
{
    //
    // This application forks, so we need a signal handler for child
    // termination.
    //
    struct sigaction action;
    action.sa_handler = childHandler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGCHLD);
    action.sa_flags = 0;
    sigaction(SIGCHLD, &action, 0);

    //
    // Make sure that this process doesn't use a router.
    //
    PropertiesPtr defaultProperties;
    try
    {
	defaultProperties = getDefaultProperties(argc, argv);
        StringSeq args = argsToStringSeq(argc, argv);
        args = defaultProperties->parseCommandLineOptions("Ice", args);
        args = defaultProperties->parseCommandLineOptions("Glacier", args);
        stringSeqToArgs(args, argc, argv);
    }
    catch(const Exception& ex)
    {
	cerr << argv[0] << ": " << ex << endl;
	return EXIT_FAILURE;
    }
    defaultProperties->setProperty("Ice.Default.Router", "");

    Glacier::RouterApp app;
    return app.main(argc, argv);
}
