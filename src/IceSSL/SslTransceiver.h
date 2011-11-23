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

#ifndef ICE_SSL_TRANSCEIVER_H
#define ICE_SSL_TRANSCEIVER_H

#include <Ice/LoggerF.h>
#include <Ice/StatsF.h>
#include <Ice/Transceiver.h>
#include <Ice/Buffer.h>
#include <IceUtil/Mutex.h>
#include <IceSSL/SslTransceiverF.h>
#include <IceSSL/OpenSSLPluginIF.h>
#include <IceSSL/TraceLevelsF.h>
#include <IceSSL/CertificateVerifierF.h>
#include <IceSSL/CertificateVerifierOpenSSL.h>

#include <openssl/ssl.h>
#include <map>

namespace IceSSL
{

class SafeFlag
{
public:

    SafeFlag(bool flagVal = false)
    {
        _flag = flagVal;
    }

    ~SafeFlag()
    {
    }

    bool checkAndSet()
    {
        IceUtil::Mutex::Lock sync(_mutex);

        if(_flag)
        {
            return false;
        }
        else
        {
            _flag = true;
            return true;
        }
    }

    bool check()
    {
        IceUtil::Mutex::Lock sync(_mutex);
        return _flag;
    }

    void set()
    {
        IceUtil::Mutex::Lock sync(_mutex);
        _flag = true;
    }

    void unset()
    {
        IceUtil::Mutex::Lock sync(_mutex);
        _flag = false;
    }

private:

    IceUtil::Mutex _mutex;
    bool _flag;
};

class HandshakeSentinel
{
public:

    HandshakeSentinel(SafeFlag& handshakeFlag) :
	_flag(handshakeFlag)
    {
        _ownHandshake = _flag.checkAndSet();
    }
    
    ~HandshakeSentinel()
    { 
        if(_ownHandshake)
        {
            _flag.unset();
        }
    }

    bool ownHandshake()
    {
        return _ownHandshake;
    }

private:

    bool _ownHandshake;
    SafeFlag& _flag;
};

// NOTE: This is a mapping from SSL* to SslTransceiver*, for use with the verifyCallback.
//       I have purposely not used SslTransceiverPtr here, as connections register themselves
//       with this map on construction and unregister themselves in the destructor.  If
//       this map used SslTransceiverPtr, SslTransceiver instances would never destruct as there
//       would always be a reference to them from the map.
class SslTransceiver;
typedef std::map<SSL*, SslTransceiver*> SslTransceiverMap;

typedef enum
{
    Handshake,  // The connection is negotiating a connection with the peer.
    Shutdown,   // The connection is in the process of shutting down.
    Connected   // The connection is connected - communication may continue.
} ConnectPhase;

class SslTransceiver : public IceInternal::Transceiver
{
public:

    virtual SOCKET fd();
    virtual void close();
    virtual void shutdown();
    virtual void write(IceInternal::Buffer&, int) = 0;
    virtual void read(IceInternal::Buffer&, int);
    virtual std::string toString() const;

    void forceHandshake();
    virtual int handshake(int timeout = 0) = 0;
    void setHandshakeReadTimeout(int);
    void setHandshakeRetries(int);
    static SslTransceiverPtr getTransceiver(SSL*);

    // Callback from OpenSSL for purposes of certificate verification
    int verifyCertificate(int, X509_STORE_CTX*);

protected:

    virtual int internalShutdown(int timeout = 0);

    int connect();
    int accept();
    int renegotiate();
    int initialize(int timeout);

    int pending();
    int getLastError() const;

    int sslRead(char*, int);
    int sslWrite(char*, int);

    int select(int, bool);
    int readSelect(int);
    int writeSelect(int);

    int readSSL(IceInternal::Buffer&, int);

    static void addTransceiver(SSL*, SslTransceiver*);
    static void removeTransceiver(SSL*);

    virtual void showConnectionInfo() = 0;

    void showCertificateChain(BIO*);
    void showPeerCertificate(BIO*, const char*);
    void showSharedCiphers(BIO*);
    void showSessionInfo(BIO*);
    void showSelectedCipherInfo(BIO*);
    void showHandshakeStats(BIO*);
    void showClientCAList(BIO*, const char*);

    void setLastError(int errorCode) { _lastError = errorCode; };

    static SslTransceiverMap _transceiverMap;
    static IceUtil::Mutex _transceiverRepositoryMutex;

    // Pointer to the OpenSSL Connection structure.
    SSL* _sslConnection;

    int _lastError;

    IceUtil::Mutex _handshakeWaitMutex;

    SafeFlag _handshakeFlag;
    int _initWantRead;
    int _initWantWrite;
    int _handshakeReadTimeout;
    int _handshakeRetries;
    int _readTimeout;

    ConnectPhase _phase;
 
    SslTransceiver(const OpenSSLPluginIPtr&, SOCKET, const IceSSL::CertificateVerifierPtr&, SSL*);
    virtual ~SslTransceiver();

    const OpenSSLPluginIPtr _plugin;
    const TraceLevelsPtr _traceLevels;
    const Ice::LoggerPtr _logger;
    const Ice::StatsPtr _stats;
    const std::string _name;
    const std::string _desc;

    SOCKET _fd;
    fd_set _rFdSet;
    fd_set _wFdSet;

    IceSSL::CertificateVerifierPtr _certificateVerifier;
};

}

#endif
