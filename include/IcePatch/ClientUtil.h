// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_PATCH_CLIENT_UTIL_H
#define ICE_PATCH_CLIENT_UTIL_H

#include <Ice/Ice.h>
#include <IcePatch/IcePatch.h>
#include <IceUtil/Exception.h>

namespace IcePatch
{

class ICE_PATCH_API ProgressCB
{
public:
    virtual void startDownload(Ice::Int, Ice::Int) = 0;
    virtual void updateDownload(Ice::Int, Ice::Int) = 0;
    virtual void finishedDownload(Ice::Int) = 0;

    virtual void startUncompress(Ice::Int, Ice::Int) = 0;
    virtual void updateUncompress(Ice::Int, Ice::Int) = 0;
    virtual void finishedUncompress(Ice::Int) = 0;
};

ICE_PATCH_API std::string pathToName(const std::string&);
ICE_PATCH_API Ice::ByteSeq getRegular(const IcePatch::RegularPrx&, ProgressCB&);
ICE_PATCH_API void checkDirectory(const std::string&, bool, const Ice::LoggerPtr& = 0);

}

#endif
