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

#ifndef FREEZE_UTIL_H
#define FREEZE_UTIL_H

#include <Ice/Ice.h>
#include <Freeze/DB.h>
#include <db_cxx.h>
#include <sys/stat.h>

#ifdef _WIN32
#   define FREEZE_DB_MODE 0
#else
#   define FREEZE_DB_MODE (S_IRUSR | S_IWUSR)
#endif


namespace Freeze
{

inline void 
initializeInDbt(const std::vector<Ice::Byte>& v, Dbt& dbt)
{
    dbt.set_data(const_cast<Ice::Byte*>(&v[0]));
    dbt.set_size(static_cast<u_int32_t>(v.size()));
    dbt.set_ulen(0);
    dbt.set_dlen(0);
    dbt.set_doff(0);
    dbt.set_flags(DB_DBT_USERMEM);
}

inline void 
initializeOutDbt(std::vector<Ice::Byte>& v, Dbt& dbt)
{
    v.resize(v.capacity());
    dbt.set_data(&v[0]);
    dbt.set_size(0);
    dbt.set_ulen(static_cast<u_int32_t>(v.size()));
    dbt.set_dlen(0);
    dbt.set_doff(0);
    dbt.set_flags(DB_DBT_USERMEM);
}

void
handleMemoryException(const DbMemoryException&, Key&, Dbt&);

void
handleMemoryException(const DbMemoryException&, Key&, Dbt&, Value&, Dbt&);

}


#endif

