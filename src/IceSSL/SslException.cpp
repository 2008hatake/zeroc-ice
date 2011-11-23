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

#include <Ice/LocalException.h>
#include <IceSSL/Exception.h>

using namespace std;
using namespace Ice;

void
IceSSL::SslException::ice_print(ostream& out) const
{
    Exception::ice_print(out);
    if(!message.empty())
    {
        out << ":\n" << message;
    }
}

void
IceSSL::ConfigurationLoadingException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::ConfigParseException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::ShutdownException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::ProtocolException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::CertificateVerificationException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::CertificateException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::CertificateSigningException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::CertificateSignatureException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::CertificateParseException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::PrivateKeyException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::PrivateKeyParseException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::CertificateVerifierTypeException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::ContextException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::ContextInitializationException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::ContextNotConfiguredException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::UnsupportedContextException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::CertificateLoadException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::PrivateKeyLoadException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::CertificateKeyMatchException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}

void
IceSSL::TrustedCertificateAddException::ice_print(ostream& out) const
{
    SslException::ice_print(out);
}
