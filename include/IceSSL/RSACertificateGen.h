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

#ifndef ICE_SSL_CERTIFICATE_GEN_H
#define ICE_SSL_CERTIFICATE_GEN_H

#include <IceSSL/RSAKeyPairF.h>

#ifdef _WIN32
#   ifdef ICE_SSL_API_EXPORTS
#       define ICE_SSL_API __declspec(dllexport)
#   else
#       define ICE_SSL_API __declspec(dllimport)
#   endif
#else
#   define ICE_SSL_API /**/
#endif

namespace IceSSL
{

class ICE_SSL_API RSACertificateGenContext
{
public:

    RSACertificateGenContext();
    ~RSACertificateGenContext();

    // Conversion helper functions.
    static long minutesToSeconds(long);
    static long hoursToSeconds(long);
    static long daysToSeconds(long);
    static long weeksToSeconds(long);
    static long yearsToSeconds(long);

    // Distinguished Name (setter) methods.
    void setCountry(const std::string&);
    void setStateProvince(const std::string&);
    void setLocality(const std::string&);
    void setOrganization(const std::string&);
    void setOrgainizationalUnit(const std::string&);
    void setCommonName(const std::string&);

    void setBitStrength(int);
    void setSecondsValid(long);
    void setIssuedAdjustment(long);

    // Distinguished Name (getters) methods.
    unsigned char* getCountry() const;
    unsigned char* getStateProvince() const;
    unsigned char* getLocality() const;
    unsigned char* getOrganization() const;
    unsigned char* getOrganizationalUnit() const;
    unsigned char* getCommonName() const;

    int getModulusLength() const;
    long getSecondsValid() const;
    long getIssuedAdjustment() const;

private:

    std::string _country;
    std::string _stateProvince;
    std::string _locality;
    std::string _organization;
    std::string _organizationalUnit;
    std::string _commonName;
    int _modulusLength;
    long _secondsValid;
    long _issuedAdjustment;
};

class ICE_SSL_API RSACertificateGen
{
public:

    RSACertificateGen();
    ~RSACertificateGen();

    RSAKeyPairPtr generate(const RSACertificateGenContext&);
    RSAKeyPairPtr loadKeyPair(const std::string&, const std::string&);
};

}

#endif
