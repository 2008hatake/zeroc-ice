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

#include <IceUtil/Mutex.h>

#include <Ice/LoggerUtil.h>
#include <Ice/Properties.h>
#include <Ice/ProtocolPluginFacade.h>
#include <Ice/Communicator.h>

#include <IceSSL/OpenSSLPluginI.h>
#include <IceSSL/TraceLevels.h>
#include <IceSSL/Exception.h>
#include <IceSSL/ConfigParser.h>
#include <IceSSL/OpenSSLJanitors.h>
#include <IceSSL/OpenSSLUtils.h>
#include <IceSSL/SslTransceiver.h>
#include <IceSSL/DefaultCertificateVerifier.h>
#include <IceSSL/SingleCertificateVerifier.h>
#include <IceSSL/SslEndpoint.h>
#include <IceSSL/RSAPrivateKey.h>
#include <IceSSL/DHParams.h>

#include <openssl/rand.h>
#include <openssl/err.h>

#include <sstream>

#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>
#if OPENSSL_VERSION_NUMBER < 0x0090700fL
#   if !defined(THREADS)
#      error "Thread support not enabled"
#   endif
#else
#   if !defined(OPENSSL_THREADS)
#      error "Thread support not enabled"
#   endif
#endif

using namespace std;
using namespace Ice;
using namespace IceInternal;
using namespace IceSSL;

void IceInternal::incRef(::IceSSL::OpenSSLPluginI* p) { p->__incRef(); }
void IceInternal::decRef(::IceSSL::OpenSSLPluginI* p) { p->__decRef(); }

//
// Plugin factory function
//
extern "C"
{

ICE_SSL_API Ice::Plugin*
create(const CommunicatorPtr& communicator, const string& name, const StringSeq& args)
{
    ProtocolPluginFacadePtr facade = getProtocolPluginFacade(communicator);

    IceSSL::OpenSSLPluginI* plugin = new IceSSL::OpenSSLPluginI(facade);
    try
    {
        plugin->configure();

        //
        // Install the SSL endpoint factory
        //
        EndpointFactoryPtr sslEndpointFactory = new SslEndpointFactory(plugin);
        facade->addEndpointFactory(sslEndpointFactory);
    }
    catch(const Exception& ex)
    {
        Ice::PluginPtr ptr = plugin; // Reclaim the plug-in instance

        Error out(communicator->getLogger());
        out << "exception in IceSSL plug-in:\n" << ex;

        // Can't throw from an extern "C" function
        return 0;
    }
    catch(...)
    {
        Ice::PluginPtr ptr = plugin; // Reclaim the plug-in instance

        Error out(communicator->getLogger());
        out << "unknown exception in IceSSL plug-in";

        // Can't throw from an extern "C" function
        return 0;
    }

    return plugin;
}

}


//
// Thread safety implementation for OpenSSL
//
namespace IceSSL
{

class SslLockKeeper
{
public:

    SslLockKeeper();
    ~SslLockKeeper();

    IceUtil::Mutex sslLocks[CRYPTO_NUM_LOCKS];

};

SslLockKeeper lockKeeper;

}

extern "C"
{
    
static void lockingCallback(int mode, int type, const char *file, int line)
{
    if(mode & CRYPTO_LOCK)
    {
        lockKeeper.sslLocks[type].lock();
    }
    else
    {
        lockKeeper.sslLocks[type].unlock();
    }
}

static unsigned long
idFunction()
{
#ifdef _WIN32
    return static_cast<unsigned long>(GetCurrentThreadId());
#else
    return static_cast<unsigned long>(pthread_self());
#endif
}
}

IceSSL::SslLockKeeper::SslLockKeeper()
{
    CRYPTO_set_id_callback(idFunction);
    CRYPTO_set_locking_callback(lockingCallback);
}

IceSSL::SslLockKeeper::~SslLockKeeper()
{
    CRYPTO_set_locking_callback(0);
    CRYPTO_set_id_callback(0);
}

//
// PluginI implementation
//

IceUtil::Mutex IceSSL::OpenSSLPluginI::_threadIdCacheMutex;
std::vector<unsigned long> IceSSL::OpenSSLPluginI::_threadIdCache;

//
// Public Methods
//
//
IceSSL::OpenSSLPluginI::OpenSSLPluginI(const ProtocolPluginFacadePtr& protocolPluginFacade) :
    _protocolPluginFacade(protocolPluginFacade),
    _traceLevels(new TraceLevels(_protocolPluginFacade)),
    _properties(_protocolPluginFacade->getCommunicator()->getProperties()),
    _serverContext(new TraceLevels(protocolPluginFacade), protocolPluginFacade->getCommunicator()),
    _clientContext(new TraceLevels(protocolPluginFacade), protocolPluginFacade->getCommunicator()),
    _randSeeded(0)
{
    SSL_load_error_strings();

    OpenSSL_add_ssl_algorithms();
}

IceSSL::OpenSSLPluginI::~OpenSSLPluginI()
{
    unregisterThreads();
    ERR_free_strings();
}

IceSSL::SslTransceiverPtr
IceSSL::OpenSSLPluginI::createTransceiver(ContextType connectionType, int socket, int timeout)
{
    IceUtil::RecMutex::Lock sync(_configMutex);

    if(connectionType == ClientServer)
    {
        UnsupportedContextException unsupportedException(__FILE__, __LINE__);

        unsupportedException.message = "unable to create client/server connections";

        throw unsupportedException;
    }

    // Configure the context if need be.
    if(!isConfigured(connectionType))
    {
        configure(connectionType);
    }

    IceSSL::SslTransceiverPtr transceiver;

    if(connectionType == Client)
    {
        transceiver = _clientContext.createTransceiver(socket, this, timeout);
    }
    else if(connectionType == Server)
    {
        transceiver = _serverContext.createTransceiver(socket, this, timeout);
    }

    return transceiver;
}

bool
IceSSL::OpenSSLPluginI::isConfigured(ContextType contextType)
{
    IceUtil::RecMutex::Lock sync(_configMutex);

    bool retCode = false;

    switch(contextType)
    {
        case Client :
        {
            retCode = _clientContext.isConfigured();
            break;
        }

        case Server :
        {
            retCode = _serverContext.isConfigured();
            break;
        }

        case ClientServer :
        {
            retCode = _clientContext.isConfigured() && _serverContext.isConfigured();
            break;
        }
    }

    return retCode;
}

void
IceSSL::OpenSSLPluginI::configure()
{
    string clientConfigFile = _properties->getProperty("IceSSL.Client.Config");
    string serverConfigFile = _properties->getProperty("IceSSL.Server.Config");
    
    bool clientConfig = (clientConfigFile.empty() ? false : true);
    bool serverConfig = (serverConfigFile.empty() ? false : true);

    if(clientConfig && serverConfig)
    {
        configure(ClientServer);
    }
    else if(clientConfig)
    {
        configure(Client);
    }
    else if(serverConfig)
    {
        configure(Server);
    }
}

void
IceSSL::OpenSSLPluginI::configure(ContextType contextType)
{
    IceUtil::RecMutex::Lock sync(_configMutex);

    switch(contextType)
    {
        case Client :
        {
            string configFile = _properties->getProperty("IceSSL.Client.Config");
            string certPath   = _properties->getProperty("IceSSL.Client.CertPath");
            loadConfig(Client, configFile, certPath);
            break;
        }

        case Server :
        {
            string configFile = _properties->getProperty("IceSSL.Server.Config");
            string certPath   = _properties->getProperty("IceSSL.Server.CertPath");
            loadConfig(Server, configFile, certPath);
            break;
        }

        case ClientServer :
        {
            string clientConfigFile = _properties->getProperty("IceSSL.Client.Config");
            string clientCertPath   = _properties->getProperty("IceSSL.Client.CertPath");
            string serverConfigFile = _properties->getProperty("IceSSL.Server.Config");
            string serverCertPath   = _properties->getProperty("IceSSL.Server.CertPath");

            // Short cut, so that we only have to load the file once.
            if((clientConfigFile == serverConfigFile) && (clientCertPath == serverCertPath))
            {
                loadConfig(ClientServer, clientConfigFile, clientCertPath);
            }
            else
            {
                loadConfig(Client, clientConfigFile, clientCertPath);
                loadConfig(Server, serverConfigFile, serverCertPath);
            }
            break;
        }
    }
}

void
IceSSL::OpenSSLPluginI::loadConfig(ContextType contextType,
                                     const std::string& configFile,
                                     const std::string& certPath)
{
    if(configFile.empty())
    {
        ConfigurationLoadingException configEx(__FILE__, __LINE__);

        string contextString;

        switch(contextType)
        {
            case Client :
            {
                contextString = "client";
                break;
            }

            case Server :
            {
                contextString = "server";
                break;
            }

            case ClientServer :
            {
                contextString = "client/server";
                break;
            }
        }

        configEx.message = "no ssl configuration file specified for ";
        configEx.message += contextString;

        throw configEx;
    }

    ConfigParser sslConfig(configFile, certPath, _traceLevels, getLogger());

    // Actually parse the file now.
    sslConfig.process();

    if((contextType == Client || contextType == ClientServer))
    {
        GeneralConfig clientGeneral;
        CertificateAuthority clientCertAuth;
        BaseCertificates clientBaseCerts;

        // Walk the parse tree, get the Client configuration.
        if(sslConfig.loadClientConfig(clientGeneral, clientCertAuth, clientBaseCerts))
        {
            initRandSystem(clientGeneral.getRandomBytesFiles());

            _clientContext.configure(clientGeneral, clientCertAuth, clientBaseCerts);
        }
    }

    if((contextType == Server || contextType == ClientServer))
    {
        GeneralConfig serverGeneral;
        CertificateAuthority serverCertAuth;
        BaseCertificates serverBaseCerts;
        TempCertificates serverTempCerts;

        // Walk the parse tree, get the Server configuration.
        if(sslConfig.loadServerConfig(serverGeneral, serverCertAuth, serverBaseCerts, serverTempCerts))
        {
            initRandSystem(serverGeneral.getRandomBytesFiles());

            loadTempCerts(serverTempCerts);

            _serverContext.configure(serverGeneral, serverCertAuth, serverBaseCerts);

            if(_traceLevels->security >= IceSSL::SECURITY_PROTOCOL)
            {
                Trace out(getLogger(), _traceLevels->securityCat);

                out << "temporary certificates (server)\n";
                out << "-------------------------------\n";
                out << serverTempCerts << "\n";
            }
        }
    }
}

RSA*
IceSSL::OpenSSLPluginI::getRSAKey(int isExport, int keyLength)
{
    IceUtil::Mutex::Lock sync(_tempRSAKeysMutex);

    RSA* rsa_tmp = 0;

    RSAMap::iterator retVal = _tempRSAKeys.find(keyLength);

    // Does the key already exist?
    if(retVal != _tempRSAKeys.end())
    {
        // Yes!  Use it.
        rsa_tmp = (*retVal).second->get();

        assert(rsa_tmp != 0);
    }
    else
    {
        const RSACertMap::iterator& it = _tempRSAFileMap.find(keyLength);

        // First we try to load a private and public key from specified files
        if(it != _tempRSAFileMap.end())
        {
            CertificateDesc& rsaKeyCert = (*it).second;

            const string& privKeyFile = rsaKeyCert.getPrivate().getFileName();
            const string& pubCertFile = rsaKeyCert.getPublic().getFileName();

            RSA* rsaCert = 0;
            RSA* rsaKey = 0;
            BIO* bio = 0;

            if((bio = BIO_new_file(pubCertFile.c_str(), "r")) != 0)
            {
                BIOJanitor bioJanitor(bio);

                rsaCert = PEM_read_bio_RSAPublicKey(bio, 0, 0, 0);
            }

            if(rsaCert != 0)
            {
                if((bio = BIO_new_file(privKeyFile.c_str(), "r")) != 0)
                {
                    BIOJanitor bioJanitor(bio);

                    rsaKey = PEM_read_bio_RSAPrivateKey(bio, &rsaCert, 0, 0);
                }
            }

            // Now, if all was well, the Certificate and Key should both be loaded into
            // rsaCert. We check to ensure that both are not 0, because if either are,
            // one of the reads failed.

            if((rsaCert != 0) && (rsaKey != 0))
            {
                rsa_tmp = rsaCert;
            }
            else
            {
                if(rsaCert != 0)
                {
                    RSA_free(rsaCert);
                    rsaCert = 0;
                }
            }
        }

        // Couldn't load file, last ditch effort - generate a key on the fly.
        if(rsa_tmp == 0)
        {
            rsa_tmp = RSA_generate_key(keyLength, RSA_F4, 0, 0);
        }

        // Save in our temporary key cache.
        if(rsa_tmp != 0)
        {
            _tempRSAKeys[keyLength] = new RSAPrivateKey(rsa_tmp);
        }
        else if(_traceLevels->security >= IceSSL::SECURITY_WARNINGS)
        {
            Trace out(getLogger(), _traceLevels->securityCat);
            out << "WRN Unable to obtain a " << dec << keyLength << "-bit RSA key.\n";
        }
    }

    return rsa_tmp;
}

DH*
IceSSL::OpenSSLPluginI::getDHParams(int isExport, int keyLength)
{
    IceUtil::Mutex::Lock sync(_tempDHKeysMutex);

    DH* dh_tmp = 0;

    const DHMap::iterator& retVal = _tempDHKeys.find(keyLength);

    // Does the key already exist?
    if(retVal != _tempDHKeys.end())
    {
        // Yes!  Use it.
        dh_tmp = (*retVal).second->get();
    }
    else
    {
        const DHParamsMap::iterator& it = _tempDHParamsFileMap.find(keyLength);

        // First we try to load params from specified files
        if(it != _tempDHParamsFileMap.end())
        {
            DiffieHellmanParamsFile& dhParamsFile = (*it).second;

            string dhFile = dhParamsFile.getFileName();

            dh_tmp = loadDHParam(dhFile.c_str());
        }

        // If that doesn't work, use a compiled-in group.
        if(dh_tmp == 0)
        {
            switch(keyLength)
            {
                case 512 :
                {
                    dh_tmp = getTempDH512();
                    break;
                }
        
                case 1024 :
                {
                    dh_tmp = getTempDH1024();
                    break;
                }

                case 2048 :
                {
                    dh_tmp = getTempDH2048();
                    break;
                }

                case 4096 :
                {
                    dh_tmp = getTempDH4096();
                    break;
                }
            }
        }

        if(dh_tmp != 0)
        {
            // Cache the dh params for quick lookup - no
            // extra processing required then.
            _tempDHKeys[keyLength] = new DHParams(dh_tmp);
        }
        else if(_traceLevels->security >= IceSSL::SECURITY_WARNINGS)
        {
            Trace out(getLogger(), _traceLevels->securityCat);
            out << "WRN Unable to obtain a " << dec << keyLength << "-bit Diffie-Hellman parameter group.\n";
        }
    }

    return dh_tmp;
}

void
IceSSL::OpenSSLPluginI::setCertificateVerifier(ContextType contextType,
                                               const IceSSL::CertificateVerifierPtr& verifier)
{
    IceUtil::RecMutex::Lock sync(_configMutex);

    IceSSL::CertificateVerifierOpenSSLPtr castVerifier;
    castVerifier = IceSSL::CertificateVerifierOpenSSLPtr::dynamicCast(verifier);

    if(!castVerifier.get())
    {
        IceSSL::CertificateVerifierTypeException cvtEx(__FILE__, __LINE__);
        throw cvtEx;
    }

    if(contextType == Client || contextType == ClientServer)
    {
        _clientContext.setCertificateVerifier(castVerifier);
    }

    if(contextType == Server || contextType == ClientServer)
    {
        _serverContext.setCertificateVerifier(castVerifier);
    }
}

void
IceSSL::OpenSSLPluginI::addTrustedCertificateBase64(ContextType contextType, const string& certString)
{
    IceUtil::RecMutex::Lock sync(_configMutex);

    if(contextType == Client || contextType == ClientServer)
    {
        _clientContext.addTrustedCertificateBase64(certString);
    }

    if(contextType == Server || contextType == ClientServer)
    {
        _serverContext.addTrustedCertificateBase64(certString);
    }
}

void
IceSSL::OpenSSLPluginI::addTrustedCertificate(ContextType contextType, const Ice::ByteSeq& certSeq)
{
    IceUtil::RecMutex::Lock sync(_configMutex);

    if(contextType == Client || contextType == ClientServer)
    {
        _clientContext.addTrustedCertificate(certSeq);
    }

    if(contextType == Server || contextType == ClientServer)
    {
        _serverContext.addTrustedCertificate(certSeq);
    }
}

void
IceSSL::OpenSSLPluginI::setRSAKeysBase64(ContextType contextType,
                                           const std::string& privateKey,
                                           const std::string& publicKey)
{
    IceUtil::RecMutex::Lock sync(_configMutex);

    if(contextType == Client || contextType == ClientServer)
    {
        _clientContext.setRSAKeysBase64(privateKey, publicKey);
    }

    if(contextType == Server || contextType == ClientServer)
    {
        _serverContext.setRSAKeysBase64(privateKey, publicKey);
    }
}

void
IceSSL::OpenSSLPluginI::setRSAKeys(ContextType contextType,
                                     const ::Ice::ByteSeq& privateKey,
                                     const ::Ice::ByteSeq& publicKey)
{
    IceUtil::RecMutex::Lock sync(_configMutex);

    if(contextType == Client || contextType == ClientServer)
    {
        _clientContext.setRSAKeys(privateKey, publicKey);
    }

    if(contextType == Server || contextType == ClientServer)
    {
        _serverContext.setRSAKeys(privateKey, publicKey);
    }
}

IceSSL::CertificateVerifierPtr
IceSSL::OpenSSLPluginI::getDefaultCertVerifier()
{
    return new DefaultCertificateVerifier(getTraceLevels(), _protocolPluginFacade->getCommunicator());
}

IceSSL::CertificateVerifierPtr
IceSSL::OpenSSLPluginI::getSingleCertVerifier(const ByteSeq& certSeq)
{
    return new SingleCertificateVerifier(certSeq);
}

void
IceSSL::OpenSSLPluginI::destroy()
{
}

TraceLevelsPtr
IceSSL::OpenSSLPluginI::getTraceLevels() const
{
    return _traceLevels;
}

LoggerPtr
IceSSL::OpenSSLPluginI::getLogger() const
{
    return _protocolPluginFacade->getCommunicator()->getLogger();
}

StatsPtr
IceSSL::OpenSSLPluginI::getStats() const
{
    //
    // Don't cache the stats object. It might not be set on the
    // communicator when the plug-in is initialized.
    //
    return _protocolPluginFacade->getCommunicator()->getStats();
}

PropertiesPtr
IceSSL::OpenSSLPluginI::getProperties() const
{
    return _properties;
}

ProtocolPluginFacadePtr
IceSSL::OpenSSLPluginI::getProtocolPluginFacade() const
{
    return _protocolPluginFacade;
}

//
// Private
//

int
IceSSL::OpenSSLPluginI::seedRand()
{
#ifdef WINDOWS
    RAND_screen();
#endif

    char buffer[1024];
    const char* file = RAND_file_name(buffer, sizeof(buffer));

    if(file == 0)
    {
        return 0;
    }
    
    return RAND_load_file(file, -1);
}

long
IceSSL::OpenSSLPluginI::loadRandFiles(const string& names)
{
    if(!names.empty())
    {
        return 0;
    }

    long tot = 0;
    int egd;

    // Make a modifiable copy of the string.
    char* namesString = new char[names.length() + 1];
    assert(namesString != 0);

    strcpy(namesString, names.c_str());

#ifdef _WIN32
    const char* seps = ";";
#else
    const char* seps = ":";
#endif

    char* token = strtok(namesString, seps);

    while(token != 0)
    {
        egd = RAND_egd(token);

        if(egd > 0)
        {
            tot += egd;
        }
        else
        {
            tot += RAND_load_file(token, -1);
        }

        token = strtok(0, seps);
    }

    if(tot > 512)
    {
        _randSeeded = 1;
    }

    delete []namesString;

    return tot;
}

void
IceSSL::OpenSSLPluginI::initRandSystem(const string& randBytesFiles)
{
    if(_randSeeded)
    {
        return;
    }

    long randBytesLoaded = seedRand();

    if(!randBytesFiles.empty())
    {
        randBytesLoaded += loadRandFiles(randBytesFiles);
    }

    if(!randBytesLoaded && !RAND_status() && (_traceLevels->security >= IceSSL::SECURITY_WARNINGS))
    {
        // In this case, there are two options open to us - specify a random data file using the
        // RANDFILE environment variable, or specify additional random data files in the
        // SSL configuration file.
        Trace out(getLogger(), _traceLevels->securityCat);
        out << "WRN there is a lack of random data, consider specifying additional random data files";
    }

    _randSeeded = (randBytesLoaded > 0 ? 1 : 0);
}

void
IceSSL::OpenSSLPluginI::loadTempCerts(TempCertificates& tempCerts)
{
    RSAVector::iterator iRSA = tempCerts.getRSACerts().begin();
    RSAVector::iterator eRSA = tempCerts.getRSACerts().end();

    while(iRSA != eRSA)
    {
        _tempRSAFileMap[(*iRSA).getKeySize()] = *iRSA;
        iRSA++;
    }

    DHVector::iterator iDHP = tempCerts.getDHParams().begin();
    DHVector::iterator eDHP = tempCerts.getDHParams().end();

    while(iDHP != eDHP)
    {
        _tempDHParamsFileMap[(*iDHP).getKeySize()] = *iDHP;
        iDHP++;
    }
}

//
// Note: These two methods are used to remember each thread that uses the IceSSL plugin,
//       and then clean up the thread-specific error queue on plugin shutdown.
//

void
IceSSL::OpenSSLPluginI::registerThread()
{
    unsigned long threadID = idFunction();

    IceUtil::Mutex::Lock sync(_threadIdCacheMutex);

    if(find(_threadIdCache.begin(), _threadIdCache.end(), threadID) == _threadIdCache.end())
    {
        _threadIdCache.push_back(threadID);
    }
}

void
IceSSL::OpenSSLPluginI::unregisterThreads()
{
    IceUtil::Mutex::Lock sync(_threadIdCacheMutex);

    for_each(_threadIdCache.begin(), _threadIdCache.end(), ERR_remove_state);
}

