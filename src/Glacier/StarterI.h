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

#ifndef GLACIER_STARTER_I_H
#define GLACIER_STARTER_I_H

#include <Ice/Ice.h>
#include <IceSSL/RSACertificateGen.h>
#include <Glacier/Starter.h>

namespace Glacier
{

using IceSSL::RSACertificateGenContext;
using IceSSL::RSACertificateGen;

class StarterI : public Starter
{
public:

    StarterI(const Ice::CommunicatorPtr&, const PermissionsVerifierPrx&);

    void destroy();

    RouterPrx startRouter(const std::string&,
			  const std::string&,
			  Ice::ByteSeq&,
			  Ice::ByteSeq&,
			  Ice::ByteSeq&,
			  const Ice::Current&);

private:

    Ice::CommunicatorPtr _communicator;
    Ice::LoggerPtr _logger;
    Ice::PropertiesPtr _properties;
    PermissionsVerifierPrx _verifier;
    int _traceLevel;
    RSACertificateGenContext _certContext;
    RSACertificateGen _certificateGenerator;
};

typedef IceUtil::Handle<StarterI> StarterIPtr;

class CryptPasswordVerifierI : public PermissionsVerifier, public IceUtil::Mutex
{
public:

    CryptPasswordVerifierI(const std::map<std::string, std::string>&);

    virtual bool checkPermissions(const std::string&, const std::string&, std::string&, const Ice::Current&) const;
    virtual void destroy(const Ice::Current&);

private:

    const std::map<std::string, std::string> _passwords;
};

}

#endif
