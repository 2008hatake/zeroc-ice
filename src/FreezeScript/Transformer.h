// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef FREEZE_SCRIPT_TRANSFORMER_H
#define FREEZE_SCRIPT_TRANSFORMER_H

#include <Ice/Ice.h>
#include <Slice/Parser.h>

class Db;
class DbTxn;

namespace FreezeScript
{

void
transformDatabase(const Ice::CommunicatorPtr&, const Slice::UnitPtr&, const Slice::UnitPtr&,
                  Db*, Db*, DbTxn*, const std::string&, bool, std::ostream&, bool, std::istream&);

} // End of namespace FreezeScript

#endif
