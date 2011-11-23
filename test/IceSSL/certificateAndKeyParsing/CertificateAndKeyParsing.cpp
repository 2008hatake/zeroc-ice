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
#include <IceSSL/Exception.h>
#include <IceSSL/RSAKeyPair.h>
#include <IceSSL/RSACertificateGen.h>
#include <IceSSL/Plugin.h>
#include <IceUtil/Base64.h>

#include <fstream>

using namespace std;
using namespace Ice;

void
testExpectCertificateAndPrivateKeyParseException(const IceSSL::PluginPtr& plugin,
                                                 const string& key,
                                                 const string& cert)
{
    try
    {
        plugin->setRSAKeysBase64(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::CertificateParseException&)
    {
    }
    catch(const IceSSL::PrivateKeyParseException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectCertificateAndPrivateKeyParseException(const IceSSL::PluginPtr& plugin,
                                                 const Ice::ByteSeq& key,
                                                 const Ice::ByteSeq& cert)
{
    try
    {
        plugin->setRSAKeys(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::CertificateParseException&)
    {
    }
    catch(const IceSSL::PrivateKeyParseException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectPrivateKeyParseException(const IceSSL::PluginPtr& plugin, const string& key, const string& cert)
{
    try
    {
        plugin->setRSAKeysBase64(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::PrivateKeyParseException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectPrivateKeyParseException(const IceSSL::PluginPtr& plugin, const Ice::ByteSeq& key, const Ice::ByteSeq& cert)
{
    try
    {
        plugin->setRSAKeys(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::PrivateKeyParseException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectCertificateParseException(const IceSSL::PluginPtr& plugin, const string& key, const string& cert)
{
    try
    {
        plugin->setRSAKeysBase64(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::CertificateParseException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectCertificateParseException(const IceSSL::PluginPtr& plugin, const Ice::ByteSeq& key, const Ice::ByteSeq& cert)
{
    try
    {
        plugin->setRSAKeys(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::CertificateParseException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectCertificateParseException(const IceSSL::PluginPtr& plugin, const string& cert)
{
    try
    {
        plugin->addTrustedCertificateBase64(IceSSL::Client, cert);
        test(false);
    }
    catch(const IceSSL::CertificateParseException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectCertificateParseException(const IceSSL::PluginPtr& plugin, const Ice::ByteSeq& cert)
{
    try
    {
        plugin->addTrustedCertificate(IceSSL::Client, cert);
        test(false);
    }
    catch(const IceSSL::CertificateParseException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectContextNotConfiguredException(const IceSSL::PluginPtr& plugin,
                                        const string& key,
                                        const string& cert)
{
    try
    {
        plugin->setRSAKeysBase64(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::ContextNotConfiguredException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectContextNotConfiguredException(const IceSSL::PluginPtr& plugin,
                                        const Ice::ByteSeq& key,
                                        const Ice::ByteSeq& cert)
{
    try
    {
        plugin->setRSAKeys(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::ContextNotConfiguredException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectContextNotConfiguredException(const IceSSL::PluginPtr& plugin, const string& cert)
{
    try
    {
        plugin->addTrustedCertificateBase64(IceSSL::Client, cert);
        test(false);
    }
    catch(const IceSSL::ContextNotConfiguredException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectContextNotConfiguredException(const IceSSL::PluginPtr& plugin, const Ice::ByteSeq& cert)
{
    try
    {
        plugin->addTrustedCertificate(IceSSL::Client, cert);
        test(false);
    }
    catch(const IceSSL::ContextNotConfiguredException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectCertificateKeyMatchException(const IceSSL::PluginPtr& plugin,
                                       const string& key,
                                       const string& cert)
{
    try
    {
        plugin->setRSAKeysBase64(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::CertificateKeyMatchException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testExpectCertificateKeyMatchException(const IceSSL::PluginPtr& plugin,
                                       const Ice::ByteSeq& key,
                                       const Ice::ByteSeq& cert)
{
    try
    {
        plugin->setRSAKeys(IceSSL::Client, key, cert);
        test(false);
    }
    catch(const IceSSL::CertificateKeyMatchException&)
    {
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testNoException(const IceSSL::PluginPtr& plugin, const string& key, const string& cert)
{
    try
    {
        plugin->setRSAKeysBase64(IceSSL::Client, key, cert);
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testNoException(const IceSSL::PluginPtr& plugin, const Ice::ByteSeq& key, const Ice::ByteSeq& cert)
{
    try
    {
        plugin->setRSAKeys(IceSSL::Client, key, cert);
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testNoException(const IceSSL::PluginPtr& plugin, const string& cert)
{
    try
    {
        plugin->addTrustedCertificateBase64(IceSSL::Client, cert);
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

void
testNoException(const IceSSL::PluginPtr& plugin, const Ice::ByteSeq& cert)
{
    try
    {
        plugin->addTrustedCertificate(IceSSL::Client, cert);
    }
    catch(const Ice::LocalException&)
    {
        //
        // Any other exception is bad.
        //

        test(false);
    }
    catch(...)
    {
        //
        // Unknown exceptions are always bad.
        //

        test(false);
    }

    cout << "ok" << endl;
}

int
run(int argc, char* argv[], const Ice::CommunicatorPtr& communicator)
{
    PropertiesPtr properties = communicator->getProperties();

    Ice::PluginPtr plugin = communicator->getPluginManager()->getPlugin("IceSSL");
    IceSSL::PluginPtr sslPlugin = IceSSL::PluginPtr::dynamicCast(plugin);

    string clientTestCertPath = properties->getProperty("IceSSL.Test.Client.CertPath");

    IceSSL::RSACertificateGen certGen;

    IceSSL::RSAKeyPairPtr goodKeyPair1;
    IceSSL::RSAKeyPairPtr goodKeyPair2;

    string goodKey1File = clientTestCertPath + "/goodKey_1.pem";
    string goodCert1File = clientTestCertPath + "/goodCert_1.pem";
    string goodKey2File = clientTestCertPath + "/goodKey_2.pem";
    string goodCert2File = clientTestCertPath + "/goodCert_2.pem";

    goodKeyPair1 = certGen.loadKeyPair(goodKey1File, goodCert1File);
    goodKeyPair2 = certGen.loadKeyPair(goodKey2File, goodCert2File);

    Ice::ByteSeq gcert1;
    Ice::ByteSeq gkey1;
    string gcert1b64;
    string gkey1b64;

    Ice::ByteSeq gcert2;
    Ice::ByteSeq gkey2;
    string gcert2b64;
    string gkey2b64;

    Ice::ByteSeq badCert;
    Ice::ByteSeq badKey;
    string badCertb64;
    string badKeyb64;

    goodKeyPair1->certToByteSeq(gcert1);
    goodKeyPair1->keyToByteSeq(gkey1);
    goodKeyPair1->certToBase64(gcert1b64);
    goodKeyPair1->keyToBase64(gkey1b64);

    goodKeyPair2->certToByteSeq(gcert2);
    goodKeyPair2->keyToByteSeq(gkey2);
    goodKeyPair2->certToBase64(gcert2b64);
    goodKeyPair2->keyToBase64(gkey2b64);

    string badKeyFile = clientTestCertPath + "/badKey.b64"; 
    string badCertFile = clientTestCertPath + "/badCert.b64";

    ifstream keyStream(badKeyFile.c_str());
    ifstream certStream(badCertFile.c_str());

    keyStream >> badKeyb64;
    certStream >> badCertb64;

    keyStream.close();
    certStream.close();

    badKey = IceUtil::Base64::decode(badKeyb64);
    badCert = IceUtil::Base64::decode(badCertb64);

    cout << "testing certificate and key parsing failures." << endl;

    cout << "bad private key and certificate (Base64)... " << flush;
    testExpectCertificateAndPrivateKeyParseException(sslPlugin, badKeyb64, badCertb64);

    cout << "bad private key and certificate... " << flush;
    testExpectCertificateAndPrivateKeyParseException(sslPlugin, badKey, badCert);

    cout << "bad private key and good certificate (Base64)... " << flush;
    testExpectPrivateKeyParseException(sslPlugin, badKeyb64, gcert1b64);

    cout << "bad private key and good certificate... " << flush;
    testExpectPrivateKeyParseException(sslPlugin, badKey, gcert1);

    cout << "good private key and bad certificate (Base64)... " << flush;
    testExpectCertificateParseException(sslPlugin, gkey1b64, badCertb64);

    cout << "good private key and bad certificate... " << flush;
    testExpectCertificateParseException(sslPlugin, gkey1, badCert);

    cout << "bad certificate as a trusted certificate... " << flush;
    testExpectCertificateParseException(sslPlugin, badCert);
    
    cout << "bad certificate as a trusted certificate (Base64)... " << flush;
    testExpectCertificateParseException(sslPlugin, badCertb64);
    
    cout << "testing setting good certificates and keys on a unconfigured context." << endl;

    cout << "good private key and certificate... " << flush;
    testExpectContextNotConfiguredException(sslPlugin, gkey1, gcert1);
    
    cout << "good private key and certificate (Base64)... " << flush;
    testExpectContextNotConfiguredException(sslPlugin, gkey1b64, gcert1b64);

    cout << "good private key and certificate (again)... " << flush;
    testExpectContextNotConfiguredException(sslPlugin, gkey2, gcert2);
    
    cout << "good private key and certificate (Base64) (again)... " << flush;
    testExpectContextNotConfiguredException(sslPlugin, gkey2b64, gcert2b64);

    cout << "good certificate as a trusted certificate... " << flush;
    testExpectContextNotConfiguredException(sslPlugin, gcert1);
    
    cout << "good certificate as a trusted certificate (Base64)... " << flush;
    testExpectContextNotConfiguredException(sslPlugin, gcert1b64);

    properties->setProperty("IceSSL.Client.CertPath", clientTestCertPath);
    properties->setProperty("IceSSL.Client.Config", "sslconfig_6.xml");
    sslPlugin->configure(IceSSL::Client);

    cout << "testing mismatched certificates and keys failures on a configured context." << endl;

    cout << "good private key and certificate, mismatched... " << flush;
    testExpectCertificateKeyMatchException(sslPlugin, gkey1, gcert2);
    
    cout << "good private key and certificate, mismatched (Base64)... " << flush;
    testExpectCertificateKeyMatchException(sslPlugin, gkey1b64, gcert2b64);

    cout << "good private key and certificate, mismatched (again)... " << flush;
    testExpectCertificateKeyMatchException(sslPlugin, gkey2, gcert1);
    
    cout << "good private key and certificate, mismatched (Base64) (again)... " << flush;
    testExpectCertificateKeyMatchException(sslPlugin, gkey2b64, gcert1b64);

    cout << "testing setting good certificates and keys on a configured context." << endl;

    cout << "good private key and certificate... " << flush;
    testNoException(sslPlugin, gkey1, gcert1);
    
    cout << "good private key and certificate (Base64)... " << flush;
    testNoException(sslPlugin, gkey1b64, gcert1b64);

    cout << "good private key and certificate (again)... " << flush;
    testNoException(sslPlugin, gkey2, gcert2);
    
    cout << "good private key and certificate (Base64) (again)... " << flush;
    testNoException(sslPlugin, gkey2b64, gcert2b64);

    cout << "good certificate as trusted certificate... " << flush;
    testNoException(sslPlugin, gcert1);
    
    cout << "good certificate as trusted certificate (Base64)... " << flush;
    testNoException(sslPlugin, gcert2b64);

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
