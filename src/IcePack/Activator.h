// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_PACK_ACTIVATOR_H
#define ICE_PACK_ACTIVATOR_H

#include <IceUtil/Thread.h>
#include <IcePack/Internal.h>

namespace IcePack
{

class TraceLevels;
typedef IceUtil::Handle<TraceLevels> TraceLevelsPtr;

class Activator : public IceUtil::Monitor< IceUtil::Mutex>, public IceUtil::Shared
{
public:

    Activator(const TraceLevelsPtr&, const Ice::PropertiesPtr&);
    virtual ~Activator();

    virtual bool activate(const std::string&, const std::string&, const std::string&, const Ice::StringSeq&, 
			  const Ice::StringSeq&, const ServerPrx&);
    virtual void deactivate(const std::string&, const Ice::ProcessPrx&);
    virtual void kill(const std::string&);
    virtual void sendSignal(const std::string&, const std::string&);
  
    virtual Ice::Int getServerPid(const std::string&);
    
    virtual void start();
    virtual void waitForShutdown();
    virtual void shutdown();
    virtual void destroy();
    
    
    void sendSignal(const std::string&, int);
    void runTerminationListener();

private:

    void deactivateAll();    

    void terminationListener();
    void clearInterrupt();
    void setInterrupt();

    struct Process
    {
#ifdef _WIN32
        DWORD pid;
        HANDLE hnd;
#else
	pid_t pid;
	int pipeFd;
#endif
	ServerPrx server;
    };

    TraceLevelsPtr _traceLevels;
    Ice::PropertiesPtr _properties;
    std::map<std::string, Process> _processes;
    bool _deactivating;

#ifdef _WIN32
    HANDLE _hIntr;
#else
    int _fdIntrRead;
    int _fdIntrWrite;
#endif

    std::vector<std::string> _propertiesOverride;

    std::string _outputDir;
    bool _redirectErrToOut;

    IceUtil::ThreadPtr _thread;
};
typedef IceUtil::Handle<Activator> ActivatorPtr;

}

#endif
