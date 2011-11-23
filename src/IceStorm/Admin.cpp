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

#include <Ice/Application.h>
#include <IceStorm/Parser.h>

#include <fstream>

using namespace std;
using namespace Ice;
using namespace IceStorm;

class Client : public Application
{
public:

    void usage();
    virtual int run(int, char*[]);
};

int
main(int argc, char* argv[])
{
    Client app;
    int rc = app.main(argc, argv);
    return rc;
}

void
Client::usage()
{
    cerr << "Usage: " << appName() << " [options] [file...]\n";
    cerr <<	
	"Options:\n"
	"-h, --help           Show this message.\n"
	"-v, --version        Display the Ice version.\n"
	"-DNAME               Define NAME as 1.\n"
	"-DNAME=DEF           Define NAME as DEF.\n"
	"-UNAME               Remove any definition for NAME.\n"
	"-IDIR                Put DIR in the include file search path.\n"
	"-e COMMANDS          Execute COMMANDS.\n"
	"-d, --debug          Print debug messages.\n"
	;
}

int
Client::run(int argc, char* argv[])
{
    string cpp("cpp");
    string commands;
    bool debug = false;

    int idx = 1;
    while(idx < argc)
    {
	if(strncmp(argv[idx], "-I", 2) == 0)
	{
	    cpp += ' ';
	    cpp += argv[idx];

	    for(int i = idx ; i + 1 < argc ; ++i)
	    {
		argv[i] = argv[i + 1];
	    }
	    --argc;
	}
	else if(strncmp(argv[idx], "-D", 2) == 0 || strncmp(argv[idx], "-U", 2) == 0)
	{
	    cpp += ' ';
	    cpp += argv[idx];

	    for(int i = idx ; i + 1 < argc ; ++i)
	    {
		argv[i] = argv[i + 1];
	    }
	    --argc;
	}
	else if(strcmp(argv[idx], "-h") == 0 || strcmp(argv[idx], "--help") == 0)
	{
	    usage();
	    return EXIT_SUCCESS;
	}
	else if(strcmp(argv[idx], "-v") == 0 || strcmp(argv[idx], "--version") == 0)
	{
	    cout << ICE_STRING_VERSION << endl;
	    return EXIT_SUCCESS;
	}
	else if(strcmp(argv[idx], "-e") == 0)
	{
	    if(idx + 1 >= argc)
            {
		cerr << appName() << ": argument expected for`" << argv[idx] << "'" << endl;
		usage();
		return EXIT_FAILURE;
            }
	    
	    commands += argv[idx + 1];
	    commands += ';';

	    for(int i = idx ; i + 2 < argc ; ++i)
	    {
		argv[i] = argv[i + 2];
	    }
	    argc -= 2;
	}
	else if(strcmp(argv[idx], "-d") == 0 || strcmp(argv[idx], "--debug") == 0)
	{
	    debug = true;
	    for(int i = idx ; i + 1 < argc ; ++i)
	    {
		argv[i] = argv[i + 1];
	    }
	    --argc;
	}
	else if(argv[idx][0] == '-')
	{
	    cerr << appName() << ": unknown option `" << argv[idx] << "'" << endl;
	    usage();
	    return EXIT_FAILURE;
	}
	else
	{
	    ++idx;
	}
    }

    if(argc >= 2 && !commands.empty())
    {
	cerr << appName() << ": `-e' option cannot be used if input files are given" << endl;
	usage();
	return EXIT_FAILURE;
    }

    PropertiesPtr properties = communicator()->getProperties();
    const char* managerProxyProperty = "IceStorm.TopicManager.Proxy";
    string managerProxy = properties->getProperty(managerProxyProperty);
    if(managerProxy.empty())
    {
	cerr << appName() << ": property `" << managerProxyProperty << "' is not set" << endl;
	return EXIT_FAILURE;
    }

    ObjectPrx base = communicator()->stringToProxy(managerProxy);
    IceStorm::TopicManagerPrx manager = IceStorm::TopicManagerPrx::checkedCast(base);
    if(!manager)
    {
	cerr << appName() << ": `" << managerProxy << "' is not running" << endl;
	return EXIT_FAILURE;
    }

    ParserPtr p = Parser::createParser(communicator(), manager);
    int status = EXIT_SUCCESS;

    if(argc < 2) // No files given
    {
	if(!commands.empty()) // Commands were given
	{
	    int parseStatus = p->parse(commands, debug);
	    if(parseStatus == EXIT_FAILURE)
	    {
		status = EXIT_FAILURE;
	    }
	}
	else // No commands, let's use standard input
	{
	    int parseStatus = p->parse(stdin, debug);
	    if(parseStatus == EXIT_FAILURE)
	    {
		status = EXIT_FAILURE;
	    }
	}
    }
    else // Process files given on the command line
    {
	for(idx = 1 ; idx < argc ; ++idx)
	{
	    ifstream test(argv[idx]);
	    if(!test)
	    {
		cerr << appName() << ": can't open `" << argv[idx] << "' for reading: " << strerror(errno) << endl;
		return EXIT_FAILURE;
	    }
	    test.close();
	    
	    string cmd = cpp + " " + argv[idx];
#ifdef _WIN32
	    FILE* cppHandle = _popen(cmd.c_str(), "r");
#else
	    FILE* cppHandle = popen(cmd.c_str(), "r");
#endif
	    if(cppHandle == NULL)
	    {
		cerr << appName() << ": can't run C++ preprocessor: " << strerror(errno) << endl;
		return EXIT_FAILURE;
	    }
	    
	    int parseStatus = p->parse(cppHandle, debug);
	    
#ifdef _WIN32
	    _pclose(cppHandle);
#else
	    pclose(cppHandle);
#endif

	    if(parseStatus == EXIT_FAILURE)
	    {
		status = EXIT_FAILURE;
	    }
	}
    }

    return status;
}
