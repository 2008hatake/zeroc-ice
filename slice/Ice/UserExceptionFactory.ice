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

#ifndef ICE_USER_EXCEPTION_FACTORY_ICE
#define ICE_USER_EXCEPTION_FACTORY_ICE

module Ice
{

/**
 *
 * A factory for user exceptions. User exception factories are
 * necessary in order to unmarshal an exception that is derived
 * from an exception declared in the operation's
 * <literal>throws</literal> clause. &Ice; does not support
 * exception truncation, therefore if the actual type of the
 * exception does not match those in the <literal>throws</literal>
 * clause and a factory is not found, the client receives
 * [NoUserExceptionFactoryException].
 *
 * @see ObjectFactory
 *
 **/
local interface UserExceptionFactory
{
    /**
     *
     * Create and throw a new user exception for a given user
     * exception type. The type is the absolute &Slice; type name,
     * i.e., the the name relative to the unnamed top-level &Slice;
     * module. For example, the absolute Slice type name for a user
     * exception [Bar] in the module [Foo] is [::Foo::Bar].
     *
     * <note><para>The leading "[::]" is required.</para></note>
     *
     * @param type The user exception type.
     *
     **/
    void createAndThrow(string type);

    /**
     *
     * Called when the factory is removed from the communicator, or if
     * the communicator is destroyed.
     *
     * @see Communicator::removeUserExceptionFactory
     * @see Communicator::destroy
     *
     **/
    void destroy();
};

};

#endif
