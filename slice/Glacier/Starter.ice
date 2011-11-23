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

#ifndef GLACIER_ICE
#define GLACIER_ICE

#include <Glacier/Router.ice>
#include <Ice/BuiltinSequences.ice>

/**
 *
 * &Glacier; is a firewall solution for Ice. &Glacier; authenticates
 * and filters client requests and allows callbacks to the client in a
 * secure fashion. In combination with &IceSSL;, Glacier provides a
 * security solution that is both non-intrusive and easy to configure.
 *
 **/
module Glacier
{

/**
 *
 * This exception is raised if router access is denied.
 *
 **/
exception PermissionDeniedException
{
    /**
     * Details as to why access was denied.
     **/
    string reason;
};

/**
 *
 * This exception is raised if the router cannot be started.
 *
 **/
exception CannotStartRouterException
{
    /**
     * Details as to why the router could not be started.
     **/
    string reason;
};

/**
 *
 * The &Glacier; router starter.
 *
 **/
interface Starter
{
    /**
     *
     * Start a new &Glacier; router. If the password for the given user
     * id is incorrect, or if the user isn't allowed access, an
     * [PermissionDeniedException] is raised. Otherwise a new router is
     * started, and a proxy to that router is returned to the caller.
     *
     * @param userId The user id for which to check the password.
     *
     * @param password The password for the given user id.
     *
     * @param privateKey The RSA Private Key (DER encoded) for the
     * client to use. (Only for SSL.)
     *
     * @param publicKey The RSA Public Key (DER encoded) for the
     * client to use. (Only for SSL.)
     *
     * @param routerCert The trusted certificate of the router.
     * (Only for SSL.)
     *
     * @return A proxy to the router that has been started.
     *
     * @throws PermissionDeniedException Raised if the password for the
     * given user id is not correct or if the user isn't allowed access.
     *
     **/
    Glacier::Router* startRouter(string userId, string password,
				 out Ice::ByteSeq privateKey, out Ice::ByteSeq publicKey, out Ice::ByteSeq routerCert)
	throws PermissionDeniedException, CannotStartRouterException;
};

/**
 *
 * The &Glacier; router starter permissions verifier.
 *
 **/
interface PermissionsVerifier
{
    /**
     *
     * Check whether user has permission to access router.
     *
     * @param userId The user id for which to check permissions.
     *
     * @param password The user's password.
     *
     * @param reason The reason access was denied.
     *
     * @return true if access is allowed, or false otherwise.
     *
     **/
    nonmutating
    bool checkPermissions(string userId, string password, out string reason);
};

};

#endif
