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

#ifndef ICE_CONNECTION_H
#define ICE_CONNECTION_H

#include <IceUtil/Mutex.h>
#include <IceUtil/RecMutex.h>
#include <IceUtil/Monitor.h>
#include <IceUtil/Time.h>
#include <Ice/ConnectionF.h>
#include <Ice/ConnectionFactoryF.h>
#include <Ice/InstanceF.h>
#include <Ice/TransceiverF.h>
#include <Ice/ObjectAdapterF.h>
#include <Ice/ServantManagerF.h>
#include <Ice/EndpointF.h>
#include <Ice/LoggerF.h>
#include <Ice/TraceLevelsF.h>
#include <Ice/OutgoingAsyncF.h>
#include <Ice/EventHandler.h>

namespace Ice
{

class LocalException;

}

namespace IceInternal
{

class Outgoing;

class Connection : public EventHandler, public IceUtil::Monitor<IceUtil::RecMutex>
{
public:

    void validate();
    enum DestructionReason
    {
	ObjectAdapterDeactivated,
	CommunicatorDestroyed
    };
    void activate();
    void hold();
    void destroy(DestructionReason);

    bool isValidated() const;
    bool isDestroyed() const;
    bool isFinished() const;

    void waitUntilHolding() const;
    void waitUntilFinished(); // Not const, as this might close the connection upon timeout.

    void monitor();

    void incProxyCount();
    void decProxyCount();

    void prepareRequest(BasicStream*);
    void sendRequest(Outgoing*, bool);
    void sendAsyncRequest(const OutgoingAsyncPtr&);

    void prepareBatchRequest(BasicStream*);
    void finishBatchRequest(BasicStream*);
    void abortBatchRequest();
    void flushBatchRequest();

    void sendResponse(BasicStream*, Ice::Byte);
    void sendNoResponse();

    int timeout() const;
    EndpointPtr endpoint() const;

    void setAdapter(const Ice::ObjectAdapterPtr&);
    Ice::ObjectAdapterPtr getAdapter() const;

    //
    // Operations from EventHandler
    //
    virtual bool datagram() const;
    virtual bool readable() const;
    virtual void read(BasicStream&);
    virtual void message(BasicStream&, const ThreadPoolPtr&);
    virtual void finished(const ThreadPoolPtr&);
    virtual void exception(const Ice::LocalException&);
    virtual std::string toString() const;

    //
    // Compare endpoints for sorting purposes.
    //
    virtual bool operator==(const Connection&) const;
    virtual bool operator!=(const Connection&) const;
    virtual bool operator<(const Connection&) const;

private:

    Connection(const InstancePtr&, const TransceiverPtr&, const EndpointPtr&, const Ice::ObjectAdapterPtr&);
    virtual ~Connection();
    friend class IncomingConnectionFactory;
    friend class OutgoingConnectionFactory;

    enum State
    {
	StateNotValidated,
	StateActive,
	StateHolding,
	StateClosing,
	StateClosed
    };

    void setState(State, const Ice::LocalException&);
    void setState(State);

    void initiateShutdown() const;

    void registerWithPool();
    void unregisterWithPool();

    static void doCompress(BasicStream&, BasicStream&);
    static void doUncompress(BasicStream&, BasicStream&);

    bool closingOK() const;

    TransceiverPtr _transceiver;
    const EndpointPtr _endpoint;

    Ice::ObjectAdapterPtr _adapter;
    ServantManagerPtr _servantManager;

    const Ice::LoggerPtr _logger;
    const TraceLevelsPtr _traceLevels;

    bool _registeredWithPool;
    ThreadPoolPtr _threadPool;

    bool _warn;

    int _acmTimeout;
    IceUtil::Time _acmAbsoluteTimeout;

    const std::vector<Ice::Byte> _requestHdr;
    const std::vector<Ice::Byte> _requestBatchHdr;
    const std::vector<Ice::Byte> _replyHdr;

    Ice::Int _nextRequestId;
    std::map<Ice::Int, Outgoing*> _requests;
    std::map<Ice::Int, Outgoing*>::iterator _requestsHint;
    std::map<Ice::Int, OutgoingAsyncPtr> _asyncRequests;
    std::map<Ice::Int, OutgoingAsyncPtr>::iterator _asyncRequestsHint;

    std::auto_ptr<Ice::LocalException> _exception;

    BasicStream _batchStream;
    int _batchRequestNum;

    int _dispatchCount;

    int _proxyCount;

    State _state;

    //
    // We need a special mutex for the isDestroyed() and isFinished()
    // functions, because these functions must not block. Using the
    // default mutex is therefore not possible, as the default mutex
    // might be locked for a longer period of time.
    //
    IceUtil::Mutex _queryMutex;
};

}

#endif
