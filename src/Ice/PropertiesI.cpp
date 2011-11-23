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

#include <Ice/PropertiesI.h>
#include <Ice/LocalException.h>
#include <fstream>

using namespace std;
using namespace Ice;
using namespace IceInternal;

string
Ice::PropertiesI::getProperty(const string& key)
{
    IceUtil::Mutex::Lock sync(*this);

    map<string, string>::const_iterator p = _properties.find(key);
    if(p != _properties.end())
    {
	return p->second;
    }
    else
    {
	return string();
    }
}

string
Ice::PropertiesI::getPropertyWithDefault(const string& key, const string& value)
{
    IceUtil::Mutex::Lock sync(*this);

    map<string, string>::const_iterator p = _properties.find(key);
    if(p != _properties.end())
    {
	return p->second;
    }
    else
    {
	return value;
    }
}

Int
Ice::PropertiesI::getPropertyAsInt(const string& key)
{
    return getPropertyAsIntWithDefault(key, 0);
}

Int
Ice::PropertiesI::getPropertyAsIntWithDefault(const string& key, Int value)
{
    IceUtil::Mutex::Lock sync(*this);
    
    map<string, string>::const_iterator p = _properties.find(key);
    if(p != _properties.end())
    {
	istringstream v(p->second);
	if(!(v >> value) || !v.eof())
	{
	    return 0;
	}
    }

    return value;
}

PropertyDict
Ice::PropertiesI::getPropertiesForPrefix(const string& prefix)
{
    IceUtil::Mutex::Lock sync(*this);

    PropertyDict result;
    map<string, string>::const_iterator p;
    for(p = _properties.begin(); p != _properties.end(); ++p)
    {
        if(prefix.empty() || p->first.compare(0, prefix.size(), prefix) == 0)
        {
            result.insert(*p);
        }
    }

    return result;
}

void
Ice::PropertiesI::setProperty(const string& key, const string& value)
{
    IceUtil::Mutex::Lock sync(*this);

    if(!key.empty())
    {
	if(!value.empty())
	{
	    _properties[key] = value;
	}
	else
	{
	    _properties.erase(key);
	}
    }
}

StringSeq
Ice::PropertiesI::getCommandLineOptions()
{
    IceUtil::Mutex::Lock sync(*this);

    StringSeq result;
    result.reserve(_properties.size());
    map<string, string>::const_iterator p;
    for(p = _properties.begin(); p != _properties.end(); ++p)
    {
	result.push_back("--" + p->first + "=" + p->second);
    }
    return result;
}

StringSeq
Ice::PropertiesI::parseCommandLineOptions(const string& prefix, const StringSeq& options)
{
    string pfx = prefix;
    if(!pfx.empty() && pfx[pfx.size() - 1] != '.')
    {
	pfx += '.';
    }
    pfx = "--" + pfx;
    
    StringSeq result;
    StringSeq::size_type i;
    for(i = 0; i < options.size(); i++)
    {
        string opt = options[i];
        if(opt.find(pfx) == 0)
        {
            if(opt.find('=') == string::npos)
            {
                opt += "=1";
            }
            
            parseLine(opt.substr(2));
        }
        else
        {
            result.push_back(opt);
        }
    }
    return result;
}

void
Ice::PropertiesI::load(const std::string& file)
{
    ifstream in(file.c_str());
    if(!in)
    {
        SyscallException ex(__FILE__, __LINE__);
        ex.error = getSystemErrno();
        throw ex;
    }

    char line[1024];
    while(in.getline(line, 1024))
    {
	parseLine(line);
    }
}

PropertiesPtr
Ice::PropertiesI::clone()
{
    IceUtil::Mutex::Lock sync(*this);

    PropertiesI* p = new PropertiesI();
    p->_properties = _properties;
    return p;
}

Ice::PropertiesI::PropertiesI()
{
    loadConfig();
}

Ice::PropertiesI::PropertiesI(StringSeq& args)
{
    StringSeq::iterator q = args.begin();
    while(q != args.end())
    {
        string s = *q;
        if(s.find("--Ice.Config") == 0)
        {
            if(s.find('=') == string::npos)
            {
                s += "=1";
            }
            parseLine(s.substr(2));
            args.erase(q);
        }
        else
        {
            ++q;
        }
    }

    loadConfig();
}

Ice::PropertiesI::PropertiesI(int& argc, char* argv[])
{
    for(int i = 1; i < argc; ++i)
    {
        if(strncmp(argv[i], "--Ice.Config", 12) == 0)
        {
            string line = argv[i];
            if(line.find('=') == string::npos)
            {
                line += "=1";
            }
            parseLine(line.substr(2));
            for(int j = i; j + 1 < argc; ++j)
            {
                argv[j] = argv[j + 1];
            }
            --argc;
        }
    }

    loadConfig();

    if(argc > 0)
    {
        string name = getProperty("Ice.ProgramName");
        if(name.empty())
        {
            setProperty("Ice.ProgramName", argv[0]);
        }
    }
}

void
Ice::PropertiesI::parseLine(const string& line)
{
    const string delim = " \t\r\n";
    string s = line;
    
    string::size_type idx = s.find('#');
    if(idx != string::npos)
    {
	s.erase(idx);
    }
    
    idx = s.find_last_not_of(delim);
    if(idx != string::npos && idx + 1 < s.length())
    {
	s.erase(idx + 1);
    }
    
    string::size_type beg = s.find_first_not_of(delim);
    if(beg == string::npos)
    {
	return;
    }
    
    string::size_type end = s.find_first_of(delim + "=", beg);
    if(end == string::npos)
    {
	return;
    }
    
    string key = s.substr(beg, end - beg);
    
    end = s.find('=', end);
    if(end == string::npos)
    {
	return;
    }
    ++end;

    string value;
    beg = s.find_first_not_of(delim, end);
    if(beg != string::npos)
    {
	end = s.length();
	value = s.substr(beg, end - beg);
    }
    
    setProperty(key, value);
}

void
Ice::PropertiesI::loadConfig()
{
    string value = getProperty("Ice.Config");

    if(value.empty() || value == "1")
    {
        const char* s = getenv("ICE_CONFIG");
        if(s && *s != '\0')
        {
            value = s;
        }
    }

    if(!value.empty())
    {
        const string delim = " \t\r\n";
        string::size_type beg = value.find_first_not_of(delim);
        while(beg != string::npos)
        {
            string::size_type end = value.find(",", beg);
            string file;
            if(end == string::npos)
            {
                file = value.substr(beg);
                beg = end;
            }
            else
            {
                file = value.substr(beg, end - beg);
                beg = value.find_first_not_of("," + delim, end);
            }
            load(file);
        }
    }

    setProperty("Ice.Config", value);
}
