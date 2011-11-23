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
#include <IceSSL/Plugin.h>
#include <IceSSL/Exception.h>
#include <TestCommon.h>
#include <Pinger.h>

using namespace std;

int
run(int argc, char* argv[], const Ice::CommunicatorPtr& communicator)
{
    string ref = "pinger:ssl -p 12345 -t 2000";

    KeyManagerPrx km = KeyManagerPrx::checkedCast(communicator->stringToProxy("keyManager:tcp -p 12344 -t 2000"));

    Ice::ByteSeq serverTrustedCert;
    Ice::ByteSeq serverUntrustedCert;
    Ice::ByteSeq clientTrustedKey;
    Ice::ByteSeq clientTrustedCert;
    Ice::ByteSeq clientUntrustedKey;
    Ice::ByteSeq clientUntrustedCert;

    km->getServerCerts(serverTrustedCert,serverUntrustedCert);
    km->getTrustedClientKeys(clientTrustedKey,clientTrustedCert);
    km->getUntrustedClientKeys(clientUntrustedKey,clientUntrustedCert);

    Ice::PluginPtr plugin = communicator->getPluginManager()->getPlugin("IceSSL");
    IceSSL::PluginPtr sslPlugin = IceSSL::PluginPtr::dynamicCast(plugin);

    Ice::PropertiesPtr properties = communicator->getProperties();

    // Use test related paths - override values in TestUtil.py
    std::string clientCertPath = properties->getProperty("TestSSL.Client.CertPath");
    std::string serverCertPath = properties->getProperty("TestSSL.Server.CertPath");
    properties->setProperty("IceSSL.Client.CertPath", clientCertPath);
    properties->setProperty("IceSSL.Server.CertPath", serverCertPath);

    bool singleCertVerifier = false;
    if(properties->getProperty("TestSSL.Client.CertificateVerifier") == "singleCert")
    {
        singleCertVerifier = true;
    }

    if(!singleCertVerifier)
    {
        cout << "client and server trusted, client using stock certificate... ";

        properties->setProperty("IceSSL.Client.Config", "sslconfig_6.xml");
        sslPlugin->configure(IceSSL::Client);
        sslPlugin->addTrustedCertificate(IceSSL::Client, serverTrustedCert);
        try
        {
            PingerPrx pinger = PingerPrx::checkedCast(communicator->stringToProxy(ref));
            pinger->ping();
            cout << "ok" << endl;
        }
        catch(const Ice::LocalException& ex)
        {
            cout << ex << endl;
            km->shutdown();
            test(false);
        }
    }

    properties->setProperty("IceSSL.Client.Config", "sslconfig_7.xml");

    cout << "client and server do not trust each other... " << flush;

    // Neither Client nor Server will trust.
    sslPlugin->configure(IceSSL::Client);
    sslPlugin->addTrustedCertificate(IceSSL::Client, serverUntrustedCert);
    if(singleCertVerifier)
    {
        IceSSL::CertificateVerifierPtr certVerifier = sslPlugin->getSingleCertVerifier(serverUntrustedCert);
        sslPlugin->setCertificateVerifier(IceSSL::Client, certVerifier);
    }
    sslPlugin->setRSAKeys(IceSSL::Client, clientUntrustedKey, clientUntrustedCert);
    try
    {
        PingerPrx pinger = PingerPrx::checkedCast(communicator->stringToProxy(ref));
        pinger->ping();
        km->shutdown();
        test(false);
    }
    catch(const IceSSL::CertificateVerificationException&)
    {
	cout << "ok" << endl;
    }
    catch(const Ice::LocalException&)
    {
        km->shutdown();
        test(false);
    }

    cout << "client trusted, server not trusted... " << flush;

    // Client will not trust Server, but Server will trust Client.
    sslPlugin->setRSAKeys(IceSSL::Client, clientTrustedKey, clientTrustedCert);
    try
    {
        PingerPrx pinger = PingerPrx::checkedCast(communicator->stringToProxy(ref));
        pinger->ping();
        km->shutdown();
        test(false);
    }
    catch(const IceSSL::CertificateVerificationException&)
    {
	cout << "ok" << endl;
    }
    catch(const Ice::LocalException&)
    {
        km->shutdown();
        test(false);
    }

    cout << "client trusts server, server does not trust client... " << flush;

    // Client trusts, Server does not.
    sslPlugin->configure(IceSSL::Client);
    sslPlugin->addTrustedCertificate(IceSSL::Client, serverTrustedCert);
    if(singleCertVerifier)
    {
        IceSSL::CertificateVerifierPtr certVerifier = sslPlugin->getSingleCertVerifier(serverTrustedCert);
        sslPlugin->setCertificateVerifier(IceSSL::Client, certVerifier);
    }
    sslPlugin->setRSAKeys(IceSSL::Client, clientUntrustedKey, clientUntrustedCert);
    try
    {
        PingerPrx pinger = PingerPrx::checkedCast(communicator->stringToProxy(ref));
        pinger->ping();
        km->shutdown();
        test(false);
    }
    catch(const IceSSL::ProtocolException&)
    {
        // Note: We expect that the server will send an alert 48 back to the client,
        //       generating this exception.
	cout << "ok" << endl;
    }
    catch(const Ice::LocalException&)
    {
        km->shutdown();
        test(false);
    }

    cout << "both client and server trust each other... " << flush;

    // Both Client and Server trust.
    sslPlugin->setRSAKeys(IceSSL::Client, clientTrustedKey, clientTrustedCert);

    try
    {
        PingerPrx pinger = PingerPrx::checkedCast(communicator->stringToProxy(ref));
	pinger->ping();
	cout << "ok" << endl;
    }
    catch(const Ice::LocalException&)
    {
        km->shutdown();
        test(false);
    }

    cout << "shutting down... " << flush;
    km->shutdown();
    cout << "ok" << endl;

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
        Ice::PropertiesPtr properties = communicator->getProperties();
        Ice::StringSeq args = Ice::argsToStringSeq(argc, argv);
        args = properties->parseCommandLineOptions("TestSSL", args);
        Ice::stringSeqToArgs(args, argc, argv);
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
