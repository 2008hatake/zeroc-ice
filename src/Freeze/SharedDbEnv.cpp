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

#include <Freeze/SharedDbEnv.h>
#include <IceUtil/StaticMutex.h>
#include <IceUtil/Thread.h>
#include <Freeze/Exception.h>
#include <Freeze/Util.h>

#include <cstdlib>
#include <map>
#include <memory>


using namespace std;
using namespace IceUtil;
using namespace Ice;

namespace Freeze
{

class CheckpointThread : public Thread, public Monitor<Mutex>
{
public:

    CheckpointThread(SharedDbEnv&, const Time&, Int, bool, Int);

    virtual void run();
  
    void terminate();

private:
    SharedDbEnv& _dbEnv;
    bool _done;
    Time _checkpointPeriod;
    Int _kbyte;
    bool _autoDelete;
    Int _trace;
};

}

namespace
{

struct MapKey
{
    string envName;
    Ice::CommunicatorPtr communicator;
};

inline bool 
operator<(const MapKey& lhs, const MapKey& rhs)
{
    return (lhs.communicator < rhs.communicator) ||
	((lhs.communicator == rhs.communicator) && (lhs.envName < rhs.envName));
}

void
dbErrCallback(const char* prefix, char* msg)
{
    const Freeze::SharedDbEnv* env = reinterpret_cast<const Freeze::SharedDbEnv*>(prefix);
    assert(env != 0);
    
    Ice::Trace out(env->getCommunicator()->getLogger(), "Berkeley DB");
    out << "DbEnv \"" << env->getEnvName() << "\": " << msg;
}


StaticMutex _mapMutex = ICE_STATIC_MUTEX_INITIALIZER;
StaticMutex _refCountMutex = ICE_STATIC_MUTEX_INITIALIZER;  

typedef map<MapKey, Freeze::SharedDbEnv*> Map;
Map* sharedDbEnvMap;

}


Freeze::SharedDbEnvPtr 
Freeze::SharedDbEnv::get(const Ice::CommunicatorPtr& communicator,
			 const std::string& envName)
{
    StaticMutex::Lock lock(_mapMutex);

    if(sharedDbEnvMap == 0)
    {
	sharedDbEnvMap = new Map;
    }

    MapKey key;
    key.envName = envName;
    key.communicator = communicator;

    {
	Map::iterator p = sharedDbEnvMap->find(key);
	if(p != sharedDbEnvMap->end())
	{
	    return p->second;
	}
    }

    //
    // MapKey not found, let's create and open a new DbEnv
    //
    auto_ptr<SharedDbEnv> result(new SharedDbEnv(envName, communicator));
    
    //
    // Insert it into the map
    //
    pair<Map::iterator, bool> insertResult;
    insertResult = sharedDbEnvMap->insert(Map::value_type(key, result.get()));
    assert(insertResult.second);
    
    return result.release();
}

Freeze::SharedDbEnv::~SharedDbEnv()
{
    if(_trace >= 1)
    {
	Trace out(_communicator->getLogger(), "Freeze.DbEnv");
	out << "closing database environment \"" << _envName << "\"";
    }

    //
    // First terminate checkpointing thread
    //
    _thread->terminate();
    _thread = 0;
    
    try
    {
	close(0);
    }
    catch(const ::DbException& dx)
    {
	DatabaseException ex(__FILE__, __LINE__);
	ex.message = dx.what();
	throw ex;
    }
}

void Freeze::SharedDbEnv::__incRef()
{
    IceUtil::StaticMutex::Lock lock(_refCountMutex);
    _refCount++;
}

void Freeze::SharedDbEnv::__decRef()
{
    IceUtil::StaticMutex::Lock lock(_refCountMutex);
    if(--_refCount == 0)
    {
        IceUtil::StaticMutex::TryLock mapLock(_mapMutex);
	if(!mapLock.acquired())
	{
	    //
	    // Reacquire mutex in proper order and check again
	    //
	    lock.release();
	    mapLock.acquire();
	    lock.acquire();
	    if(_refCount > 0)
	    {
		return;
	    }
	}

	//
	// Remove from map
	//

        MapKey key;
        key.envName = _envName;
        key.communicator = _communicator;
	size_t one;
	one = sharedDbEnvMap->erase(key);
	assert(one == 1);

	if(sharedDbEnvMap->size() == 0)
	{
	    delete sharedDbEnvMap;
	    sharedDbEnvMap = 0;
	}

	//
	// Keep lock to prevent somebody else to re-open this DbEnv
	// before it's closed.
	//
	delete this;
    }
}

void 
Freeze::SharedDbEnv::deleteOldLogs()
{
    IceUtil::Mutex::Lock lock(_oldLogsMutex);
    
    char** list = 0;

    try
    {
	log_archive(&list, DB_ARCH_ABS);
	
	if(list != 0)
	{
	    for(int i = 0; list[i] != 0; i++)
	    {
		//
		// Remove each file
		//

		if(_trace >= 2)
		{
		    Trace out(_communicator->getLogger(), "Freeze.DbEnv");
		    out << "removing \"" << list[i] << "\" from DbEnv \"" << _envName << "\"";
		}

#ifdef _WIN32

#if defined(_MSC_VER) && (_MSC_VER <= 1200)
		BOOL ok = DeleteFile(list[i]);
#else
		BOOL ok = DeleteFileA(list[i]);
#endif
		if(!ok)
		{
		    DWORD err = GetLastError();
		    Warning out(_communicator->getLogger());
		    out << "could not delete file: \"" << list[i] << "\" error number: " << err;
		}
#else
		int err = unlink(list[i]);
		if(err != 0)
		{
		    Warning out(_communicator->getLogger());
		    out << "could not unlink file: \"" << list[i] << "\": " << strerror(err);
		}
#endif
	    
	    }
	}
    }
    catch(const ::DbException& dx)
    {
	free(list);
	DatabaseException ex(__FILE__, __LINE__);
	ex.message = dx.what();
	throw ex;
    }
    catch(...)
    {
	free(list);
	throw;
    }   
    free(list);
}

Freeze::SharedDbEnv::SharedDbEnv(const std::string& envName,
				 const Ice::CommunicatorPtr& communicator) :
     DbEnv(0),
    _envName(envName),
     _communicator(communicator),
     _refCount(0)
{
    Ice::PropertiesPtr properties = _communicator->getProperties();

    _trace = properties->getPropertyAsInt("Freeze.Trace.DbEnv");

    if(_trace >= 1)
    {
	Trace out(_communicator->getLogger(), "Freeze.DbEnv");
	out << "opening database environment \"" << envName << "\"";
    }

    string propertyPrefix = string("Freeze.DbEnv.") + envName;

    try
    {
	set_errpfx(reinterpret_cast<char*>(this));
	set_errcall(dbErrCallback);
    
#ifdef _WIN32
	//
	// Berkeley DB may use a different C++ runtime
	//
	set_alloc(::malloc, ::realloc, ::free);
#endif

	//
	// Deadlock detection
	//
	set_lk_detect(DB_LOCK_MINLOCKS);
	
	u_int32_t flags = DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN;

	if(properties->getPropertyAsInt(propertyPrefix + ".DbRecoverFatal") != 0)
	{
	    flags |= DB_RECOVER_FATAL | DB_CREATE;
	}
	else
	{
	    flags |= DB_RECOVER | DB_CREATE;
	}
	
	if(properties->getPropertyAsIntWithDefault(propertyPrefix + ".DbPrivate", 1) != 0)
	{
	    flags |= DB_PRIVATE;
	}

	/*
	  
	//
	// Does not seem to work reliably in 4.1.25
	//
	
	time_t timeStamp = properties->getPropertyAsIntWithDefault(propertyPrefix + ".TxTimestamp", 0);
	
	if(timeStamp != 0)
	{
	    try
	    {
		set_tx_timestamp(&timeStamp);
	    }
	    catch(const ::DbException& dx)
	    {
		DatabaseException ex(__FILE__, __LINE__);
		ex.message = dx.what();
		throw ex;
	    }
	}
	*/
	
	//
	// Threading
	// 
	flags |= DB_THREAD;

	string dbHome = properties->getPropertyWithDefault(
	    propertyPrefix + ".DbHome", envName);
	
	open(dbHome.c_str(), flags, FREEZE_DB_MODE);
    }
    catch(const ::DbException& dx)
    {
	DatabaseException ex(__FILE__, __LINE__);
	ex.message = dx.what();
	throw ex;
    }

    //
    // Default checkpoint period is every 120 seconds
    //
    Int checkpointPeriod = properties->getPropertyAsIntWithDefault(
	propertyPrefix + ".CheckpointPeriod", 120);
    Int kbyte = properties->getPropertyAsIntWithDefault(propertyPrefix + ".PeriodicCheckpointMinSize", 0);

    bool autoDelete = (properties->getPropertyAsIntWithDefault(
			   propertyPrefix + ".OldLogsAutoDelete", 1) != 0); 
    
    _thread = new CheckpointThread(*this, Time::seconds(checkpointPeriod), kbyte, autoDelete, _trace);
}



Freeze::CheckpointThread::CheckpointThread(SharedDbEnv& dbEnv, const Time& checkpointPeriod, Int kbyte, bool autoDelete, Int trace) : 
    _dbEnv(dbEnv), 
    _done(false), 
    _checkpointPeriod(checkpointPeriod), 
    _kbyte(kbyte),
    _autoDelete(autoDelete),
    _trace(trace)
{
    start();
}

void
Freeze::CheckpointThread::terminate()
{
    {
	Lock sync(*this);
	_done = true;
	notify();
    }
    
    getThreadControl().join();
}


void 
Freeze::CheckpointThread::run()
{
    for(;;)
    {
	{
	    Lock sync(*this);
	    while(!_done && timedWait(_checkpointPeriod))
	    {
		//
		// Loop
		//
	    }
	    if(_done)
	    {
		return;
	    }
	}

	try
	{
	    if(_trace >= 2)
	    {
		Trace out(_dbEnv.getCommunicator()->getLogger(), "Freeze.DbEnv");
		out << "checkpointing environment \"" << _dbEnv.getEnvName() << "\"";
	    }
	    _dbEnv.txn_checkpoint(_kbyte, 0, 0);
	}
	catch(const DbException& dx)
	{
	    Warning out(_dbEnv.getCommunicator()->getLogger());
	    out << "checkpoint on DbEnv \"" << _dbEnv.getEnvName() << "\" raised DbException: " << dx.what();
	}
	
	if(_autoDelete)
	{
	    try
	    {
		_dbEnv.deleteOldLogs();
	    }
	    catch(const IceUtil::Exception& ex)
	    {
		Warning out(_dbEnv.getCommunicator()->getLogger());
		out << "deleteOldLogs on DbEnv \"" << _dbEnv.getEnvName() << "\" raised: " << ex;
	    }
	}
    }
}
