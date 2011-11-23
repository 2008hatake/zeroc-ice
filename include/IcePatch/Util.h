// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_PATCH_UTIL_H
#define ICE_PATCH_UTIL_H

#include <Ice/Ice.h>
#include <IcePatch/IcePatch.h>

namespace IcePatch
{

ICE_PATCH_API std::string identityToPath(const Ice::Identity&);
ICE_PATCH_API Ice::Identity pathToIdentity(const std::string&);

ICE_PATCH_API std::string getSuffix(const std::string&);
ICE_PATCH_API bool ignoreSuffix(const std::string&);
ICE_PATCH_API std::string removeSuffix(const std::string&);

enum FileType
{
    FileTypeNotExist,
    FileTypeDirectory,
    FileTypeRegular
};

struct FileInfo
{
    FileType type;
    off_t size;
    time_t time;
};

ICE_PATCH_API FileInfo getFileInfo(const std::string&, bool, const Ice::LoggerPtr& = 0);

ICE_PATCH_API void removeRecursive(const std::string&, const Ice::LoggerPtr& = 0);

ICE_PATCH_API Ice::StringSeq readDirectory(const std::string&);
ICE_PATCH_API Ice::StringSeq readDirectory(const std::string&, const std::string&);
ICE_PATCH_API void createDirectory(const std::string&);

ICE_PATCH_API Ice::ByteSeq getMD5(const std::string&);
ICE_PATCH_API void putMD5(const std::string&, const Ice::ByteSeq&);
ICE_PATCH_API void createMD5(const std::string&, bool, const Ice::LoggerPtr& = 0);
ICE_PATCH_API Ice::ByteSeq calcMD5(const std::string&, bool, const Ice::LoggerPtr& = 0);
ICE_PATCH_API Ice::ByteSeq calcPartialMD5(const std::string&, Ice::Int, const Ice::LoggerPtr& = 0);

ICE_PATCH_API Ice::ByteSeq getBZ2(const std::string&, Ice::Int, Ice::Int);
ICE_PATCH_API void createBZ2(const std::string&, const Ice::LoggerPtr&);

typedef std::map<Ice::ByteSeq, Ice::Long> TotalMap;

ICE_PATCH_API TotalMap getTotalMap(const Ice::CommunicatorPtr&, const std::string&);
ICE_PATCH_API void putTotalMap(const Ice::CommunicatorPtr&, const std::string&, const TotalMap&);

#ifdef _WIN32

//
// Function object to do case-insensitive string comparison.
//
class ICE_PATCH_API CICompare : public std::binary_function<std::string, std::string, bool>
{
public:

    bool operator()(const std::string&, const std::string&) const;
};

#endif

}

#endif
