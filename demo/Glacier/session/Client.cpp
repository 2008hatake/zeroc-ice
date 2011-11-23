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
#include <Glacier/Glacier.h>
#include <Glacier/Router.h>
#include <IceSSL/Plugin.h>

#include <HelloSession.h>

using namespace std;

void
menu()
{
    cout <<
	"usage:\n"
	"t: send greeting as twoway\n"
	"o: send greeting as oneway\n"
	"O: send greeting as batch oneway\n"
	"d: send greeting as datagram\n"
	"D: send greeting as batch datagram\n"
	"f: flush all batch requests\n"
	"T: set a timeout\n"
	"S: switch secure mode on/off\n"
	"x: exit\n"
	"?: help\n";
}

int
run(int argc, char* argv[], const Ice::CommunicatorPtr& communicator)
{
    Ice::PropertiesPtr properties = communicator->getProperties();
    //
    // Do Glacier setup.
    //
    const char* glacierStarterEndpointsProperty = "Glacier.Starter.Endpoints";
    string glacierStarterEndpoints = properties->getProperty(glacierStarterEndpointsProperty);
    assert (!glacierStarterEndpoints.empty());
    Ice::ObjectPrx starterBase = communicator->stringToProxy("Glacier/starter:" + glacierStarterEndpoints);
    Glacier::StarterPrx starter = Glacier::StarterPrx::checkedCast(starterBase);
    if(!starter)
    {
	cerr << argv[0] << ": endpoints `" << glacierStarterEndpoints
	     << "' do not refer to a glacier router starter" << endl;
	return EXIT_FAILURE;
    }
    
    Ice::ByteSeq privateKey;
    Ice::ByteSeq publicKey;
    Ice::ByteSeq routerCert;
    
    Glacier::RouterPrx router;
    string id;
    string pw;

    while(true)
    {
	cout << "user id: " << flush;
	cin >> id;
	cout << "password: " << flush;
	cin >> pw;
    
	try
	{
	    router = starter->startRouter(id, pw, privateKey, publicKey, routerCert);
	}
	catch(const Glacier::CannotStartRouterException& ex)
	{
	    cerr << argv[0] << ": " << ex << ":\n" << ex.reason << endl;
	    return EXIT_FAILURE;
	}
	catch(const Glacier::PermissionDeniedException&)
	{
	    cout << "password is invalid, try again" << endl;
	    continue;
	}
	break;
    }

    //
    // Required in order to activate the trust relationship with
    // the glacier router.
    //

    //
    // Get the SSL plugin.
    //
    Ice::PluginManagerPtr pluginManager = communicator->getPluginManager();
    Ice::PluginPtr plugin = pluginManager->getPlugin("IceSSL");
    IceSSL::PluginPtr sslPlugin = IceSSL::PluginPtr::dynamicCast(plugin);
    assert(sslPlugin);

    // Configure the client context of the IceSSL Plugin
    sslPlugin->configure(IceSSL::Client);

    // Trust only the router certificate, no other certificate.
    sslPlugin->addTrustedCertificate(IceSSL::Client, routerCert);
    sslPlugin->setCertificateVerifier(IceSSL::Client, sslPlugin->getSingleCertVerifier(routerCert));

    // Set the public and private keys that the Router will accept.
    sslPlugin->setRSAKeys(IceSSL::Client, privateKey, publicKey);
    
    communicator->setDefaultRouter(router);

    Glacier::SessionPrx session = router->createSession();
    HelloSessionPrx base = HelloSessionPrx::checkedCast(session);

    HelloSessionPrx twoway = HelloSessionPrx::checkedCast(base->ice_twoway()->ice_timeout(-1)->ice_secure(false));
    if(!twoway)
    {
	cerr << argv[0] << ": invalid object reference" << endl;
	return EXIT_FAILURE;
    }
    HelloSessionPrx oneway = HelloSessionPrx::uncheckedCast(twoway->ice_oneway());
    HelloSessionPrx batchOneway = HelloSessionPrx::uncheckedCast(twoway->ice_batchOneway());
    HelloSessionPrx datagram = HelloSessionPrx::uncheckedCast(twoway->ice_datagram());
    HelloSessionPrx batchDatagram = HelloSessionPrx::uncheckedCast(twoway->ice_batchDatagram());

    bool secure = false;
    int timeout = -1;

    menu();

    char c;
    do
    {
	try
	{
	    cout << "==> ";
	    cin >> c;
	    if(c == 't')
	    {
		twoway->hello();
	    }
	    else if(c == 'o')
	    {
		oneway->hello();
	    }
	    else if(c == 'O')
	    {
		batchOneway->hello();
	    }
	    else if(c == 'd')
	    {
                if(secure)
                {
                    cout << "secure datagrams are not supported" << endl;
                }
                else
                {
                    datagram->hello();
                }
	    }
	    else if(c == 'D')
	    {
                if(secure)
                {
                    cout << "secure datagrams are not supported" << endl;
                }
                else
                {
                    batchDatagram->hello();
                }
	    }
	    else if(c == 'f')
	    {
		communicator->flushBatchRequests();
	    }
	    else if(c == 'T')
	    {
		if(timeout == -1)
		{
		    timeout = 2000;
		}
		else
		{
		    timeout = -1;
		}
		
		twoway = HelloSessionPrx::uncheckedCast(twoway->ice_timeout(timeout));
		oneway = HelloSessionPrx::uncheckedCast(oneway->ice_timeout(timeout));
		batchOneway = HelloSessionPrx::uncheckedCast(batchOneway->ice_timeout(timeout));
		
		if(timeout == -1)
		{
		    cout << "timeout is now switched off" << endl;
		}
		else
		{
		    cout << "timeout is now set to 2000ms" << endl;
		}
	    }
	    else if(c == 'S')
	    {
		secure = !secure;
		
		twoway = HelloSessionPrx::uncheckedCast(twoway->ice_secure(secure));
		oneway = HelloSessionPrx::uncheckedCast(oneway->ice_secure(secure));
		batchOneway = HelloSessionPrx::uncheckedCast(batchOneway->ice_secure(secure));
		datagram = HelloSessionPrx::uncheckedCast(datagram->ice_secure(secure));
		batchDatagram = HelloSessionPrx::uncheckedCast(batchDatagram->ice_secure(secure));
		
		if(secure)
		{
		    cout << "secure mode is now on" << endl;
		}
		else
		{
		    cout << "secure mode is now off" << endl;
		}
	    }
	    else if(c == 'x')
	    {
		// Nothing to do
	    }
	    else if(c == '?')
	    {
		menu();
	    }
	    else
	    {
		cout << "unknown command `" << c << "'" << endl;
		menu();
	    }
	}
	catch(const Ice::Exception& ex)
	{
	    cerr << ex << endl;
	}
    }
    while(cin.good() && c != 'x');

    //
    // Shutdown the router.
    //
    router->shutdown();

    return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
    int status;
    Ice::CommunicatorPtr communicator;

    try
    {
	Ice::PropertiesPtr properties = Ice::createProperties(argc, argv);
        properties->load("config");
	communicator = Ice::initializeWithProperties(argc, argv, properties);
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
