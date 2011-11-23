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

#include <Ice/FactoryTable.h>
#include <Ice/UserExceptionFactory.h>

//
// This constructor initializes the single global Ice::factoryTable instance
// from the outside (if it hasn't been initialized yet). The constructor here
// is triggered by a file-static instance of FactoryTable in each
// slice2cpp-generated header file that uses non-local exceptions or non-abstract classes.
// This ensures that Ice::factoryTable is always initialized before it is used.
//
Ice::FactoryTable::FactoryTable()
{

    Ice::factoryTableWrapper.initialize();
}

//
// Similarly, the destructor calls the finalize() method on the factory table wrapper which,
// once the tables reference count drops to zero, deletes the table.
//
Ice::FactoryTable::~FactoryTable()
{
    Ice::factoryTableWrapper.finalize();
}
