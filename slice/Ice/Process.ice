// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_PROCESS_ICE
#define ICE_PROCESS_ICE

module Ice
{

/**
 *
 * An administrative interface for process management. Managed servers must
 * implement this interface and invoke [ObjectAdapter::setProcess] to register
 * the process proxy.
 *
 * <note><para> A servant implementing this interface is a potential target
 * for denial-of-service attacks, therefore proper security precautions
 * should be taken. For example, the servant can use a UUID to make its
 * identity harder to guess, and be registered in an object adapter with
 * a secured endpoint.</para></note>
 *
 **/
interface Process
{
    /**
     *
     * Initiate a graceful shutdown.
     *
     * @see Communicator::shutdown
     *
     **/
    idempotent void shutdown();
};

};

#endif
