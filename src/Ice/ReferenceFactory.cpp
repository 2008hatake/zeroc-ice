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

#include <Ice/ReferenceFactory.h>
#include <Ice/ProxyFactory.h>
#include <Ice/LocalException.h>
#include <Ice/Instance.h>
#include <Ice/IdentityUtil.h>
#include <Ice/Endpoint.h>
#include <Ice/EndpointFactoryManager.h>
#include <Ice/RouterInfo.h>
#include <Ice/LocatorInfo.h>
#include <Ice/StringUtil.h>

using namespace std;
using namespace Ice;
using namespace IceInternal;

void IceInternal::incRef(::IceInternal::ReferenceFactory* p) { p->__incRef(); }
void IceInternal::decRef(::IceInternal::ReferenceFactory* p) { p->__decRef(); }

ReferencePtr
IceInternal::ReferenceFactory::create(const Identity& ident,
				      const vector<string>& facet,
				      Reference::Mode mode,
				      bool secure,
				      const string& adapterId,
				      const vector<EndpointPtr>& endpoints,
				      const RouterInfoPtr& routerInfo,
				      const LocatorInfoPtr& locatorInfo,
				      const ObjectAdapterPtr& reverseAdapter,
				      bool collocationOptimization)
{
    Mutex::Lock sync(*this);

    if(!_instance)
    {
	throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    if(ident.name.empty() && ident.category.empty())
    {
        return 0;
    }

    //
    // Create new reference
    //
    ReferencePtr ref = new Reference(_instance, ident, facet, mode, secure, adapterId,
				     endpoints, routerInfo, locatorInfo, reverseAdapter, collocationOptimization);


//
// This code is currently not used, because the eviction code below is
// too slow when there are a large number of references. The savings
// are also rather questionable.
//
/*
    //
    // If we already have an equivalent reference, use such equivalent
    // reference. Otherwise add the new reference to the reference
    // set.
    //
    set<ReferencePtr>::iterator p = _references.end();
    
    if(_referencesHint != _references.end())
    {
	if(*_referencesHint == ref)
	{
	    p = _referencesHint;
	}
    }
    
    if(p == _references.end())
    {
	p = _references.find(ref);
    }
    
    if(p == _references.end())
    {
	_referencesHint = _references.insert(_referencesHint, ref);
    }
    else
    {
	_referencesHint = p;
	ref = *_referencesHint;
    }

    //
    // At every 10th call, evict references which are not in use anymore.
    //
    if(++_evict >= 10)
    {
	_evict = 0;
	p = _references.begin();
	while(p != _references.end())
	{
	    if((*p)->__getRef() == 1)
	    {
		assert(p != _referencesHint);
		_references.erase(p++);
	    }
	    else
	    {
		++p;
	    }
	}
    }
*/

    return ref;
}

ReferencePtr
IceInternal::ReferenceFactory::create(const string& str)
{
    if(str.empty())
    {
        return 0;
    }

    const string delim = " \t\n\r";

    string s(str);
    string::size_type beg;
    string::size_type end = 0;

    beg = s.find_first_not_of(delim, end);
    if(beg == string::npos)
    {
	ProxyParseException ex(__FILE__, __LINE__);
	ex.str = str;
	throw ex;
    }
    
    //
    // Extract the identity, which may be enclosed in single
    // or double quotation marks.
    //
    string idstr;
    end = checkQuote(s, beg);
    if(end == string::npos)
    {
	ProxyParseException ex(__FILE__, __LINE__);
	ex.str = str;
	throw ex;
    }
    else if(end == 0)
    {
        end = s.find_first_of(delim + ":@", beg);
        if(end == string::npos)
        {
            end = s.size();
        }
        idstr = s.substr(beg, end - beg);
    }
    else
    {
        beg++; // Skip leading quote
        idstr = s.substr(beg, end - beg);
        end++; // Skip trailing quote
    }

    if(beg == end)
    {
	ProxyParseException ex(__FILE__, __LINE__);
	ex.str = str;
	throw ex;
    }

    //
    // Parsing the identity may raise IdentityParseException.
    //
    Identity ident = stringToIdentity(idstr);

    if(ident.name.empty())
    {
        //
        // An identity with an empty name and a non-empty
        // category is illegal.
        //
        if(!ident.category.empty())
        {
            IllegalIdentityException e(__FILE__, __LINE__);
            e.id = ident;
            throw e;
        }
        //
        // Treat a stringified proxy containing two double
        // quotes ("") the same as an empty string, i.e.,
        // a null proxy, but only if nothing follows the
        // quotes.
        //
        else if(s.find_first_not_of(delim, end) != string::npos)
        {
            ProxyParseException ex(__FILE__, __LINE__);
            ex.str = str;
            throw ex;
        }
        else
        {
            return 0;
        }
    }

    vector<string> facet;
    Reference::Mode mode = Reference::ModeTwoway;
    bool secure = false;
    string adapter;

    while(true)
    {
	beg = s.find_first_not_of(delim, end);
	if(beg == string::npos)
	{
	    break;
	}

        if(s[beg] == ':' || s[beg] == '@')
        {
            break;
        }
        
	end = s.find_first_of(delim + ":@", beg);
	if(end == string::npos)
	{
	    end = s.length();
	}

	if(beg == end)
	{
	    break;
	}
	
	string option = s.substr(beg, end - beg);
	if(option.length() != 2 || option[0] != '-')
	{
	    ProxyParseException ex(__FILE__, __LINE__);
	    ex.str = str;
	    throw ex;
	}

        //
        // Check for the presence of an option argument. The
        // argument may be enclosed in single or double
        // quotation marks.
        //
	string argument;
	string::size_type argumentBeg = s.find_first_not_of(delim, end);
	if(argumentBeg != string::npos)
	{
            if(s[argumentBeg] != '@' && s[argumentBeg] != ':' && s[argumentBeg] != '-')
            {
                beg = argumentBeg;
                end = checkQuote(s, beg);
                if(end == string::npos)
                {
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
                }
                else if(end == 0)
                {
                    end = s.find_first_of(delim + ":@", beg);
                    if(end == string::npos)
                    {
                        end = s.size();
                    }
                    argument = s.substr(beg, end - beg);
                }
                else
                {
                    beg++; // Skip leading quote
                    argument = s.substr(beg, end - beg);
                    end++; // Skip trailing quote
                }
	    }
	}

	//
	// If any new options are added here,
	// IceInternal::Reference::toString() must be updated as well.
	//
	switch(option[1])
	{
	    case 'f':
	    {
		if(argument.empty())
		{
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
		}

                const string::size_type argLen = argument.size();

                string::size_type argBeg = 0;
                while(argBeg < argLen)
                {
                    //
                    // Skip slashes
                    //
                    argBeg = argument.find_first_not_of("/", argBeg);
                    if(argBeg == string::npos)
                    {
                        break;
                    }

                    //
                    // Find unescaped slash
                    //
                    string::size_type argEnd = argBeg;
                    while((argEnd = argument.find('/', argEnd)) != string::npos)
                    {
                        if(argument[argEnd - 1] != '\\')
                        {
                            break;
                        }
                        argEnd++;
                    }

                    if(argEnd == string::npos)
                    {
                        argEnd = argLen;
                    }

                    string token;
                    if(!decodeString(argument, argBeg, argEnd, token))
                    {
			ProxyParseException ex(__FILE__, __LINE__);
			ex.str = str;
			throw ex;
                    }
                    facet.push_back(token);
                    argBeg = argEnd + 1;
                }

                if(facet.size() == 0)
                {
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
                }

		break;
	    }

	    case 't':
	    {
		if(!argument.empty())
		{
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
		}
		mode = Reference::ModeTwoway;
		break;
	    }

	    case 'o':
	    {
		if(!argument.empty())
		{
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
		}
		mode = Reference::ModeOneway;
		break;
	    }

	    case 'O':
	    {
		if(!argument.empty())
		{
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
		}
		mode = Reference::ModeBatchOneway;
		break;
	    }

	    case 'd':
	    {
		if(!argument.empty())
		{
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
		}
		mode = Reference::ModeDatagram;
		break;
	    }

	    case 'D':
	    {
		if(!argument.empty())
		{
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
		}
		mode = Reference::ModeBatchDatagram;
		break;
	    }

	    case 's':
	    {
		if(!argument.empty())
		{
		    ProxyParseException ex(__FILE__, __LINE__);
		    ex.str = str;
		    throw ex;
		}
		secure = true;
		break;
	    }

	    default:
	    {
		ProxyParseException ex(__FILE__, __LINE__);
		ex.str = str;
		throw ex;
	    }
	}
    }

    vector<EndpointPtr> endpoints;
    if(beg != string::npos)
    {
	if(s[beg] == ':')
	{
	    end = beg;
	    
	    while(end < s.length() && s[end] == ':')
	    {
		beg = end + 1;
		
		end = s.find(':', beg);
		if(end == string::npos)
		{
		    end = s.length();
		}
		
		string es = s.substr(beg, end - beg);
		EndpointPtr endp = _instance->endpointFactoryManager()->create(es);
		endpoints.push_back(endp);
	    }
	}
	else if(s[beg] == '@')
	{
	    beg = s.find_first_not_of(delim, beg + 1);
	    if(beg == string::npos)
	    {
		ProxyParseException ex(__FILE__, __LINE__);
		ex.str = str;
		throw ex;
	    }

            end = checkQuote(s, beg);
            if(end == string::npos)
            {
		ProxyParseException ex(__FILE__, __LINE__);
		ex.str = str;
		throw ex;
            }
            else if(end == 0)
            {
                end = s.find_first_of(delim, beg);
                if(end == string::npos)
                {
                    end = s.size();
                }
            }
            else
            {
                beg++; // Skip leading quote
            }

            if(!decodeString(s, beg, end, adapter) || adapter.size() == 0)
            {
		ProxyParseException ex(__FILE__, __LINE__);
		ex.str = str;
		throw ex;
            }
	}
    }

    RouterInfoPtr routerInfo = _instance->routerManager()->get(getDefaultRouter());
    LocatorInfoPtr locatorInfo = _instance->locatorManager()->get(getDefaultLocator());
    return create(ident, facet, mode, secure, adapter, endpoints, routerInfo, locatorInfo, 0, true);
}

ReferencePtr
IceInternal::ReferenceFactory::create(const Identity& ident, BasicStream* s)
{
    //
    // Don't read the identity here. Operations calling this
    // constructor read the identity, and pass it as a parameter.
    //

    if(ident.name.empty() && ident.category.empty())
    {
	return 0;
    }

    vector<string> facet;
    s->read(facet);

    Byte modeAsByte;
    s->read(modeAsByte);
    Reference::Mode mode = static_cast<Reference::Mode>(modeAsByte);
    if(mode < 0 || mode > Reference::ModeLast)
    {
	throw ProxyUnmarshalException(__FILE__, __LINE__);
    }

    bool secure;
    s->read(secure);

    vector<EndpointPtr> endpoints;
    string adapterId;

    Ice::Int sz;
    s->readSize(sz);
    
    if(sz > 0)
    {
	endpoints.reserve(sz);
	while(sz--)
	{
	    EndpointPtr endpoint = _instance->endpointFactoryManager()->read(s);
	    endpoints.push_back(endpoint);
	}
    }
    else
    {
	s->read(adapterId);
    }

    RouterInfoPtr routerInfo = _instance->routerManager()->get(getDefaultRouter());
    LocatorInfoPtr locatorInfo = _instance->locatorManager()->get(getDefaultLocator());
    return create(ident, facet, mode, secure, adapterId, endpoints, routerInfo, locatorInfo, 0, true);
}

void
IceInternal::ReferenceFactory::setDefaultRouter(const RouterPrx& defaultRouter)
{
    IceUtil::Mutex::Lock sync(*this);
    _defaultRouter = defaultRouter;
}

RouterPrx
IceInternal::ReferenceFactory::getDefaultRouter() const
{
    IceUtil::Mutex::Lock sync(*this);
    return _defaultRouter;
}

void
IceInternal::ReferenceFactory::setDefaultLocator(const LocatorPrx& defaultLocator)
{
    IceUtil::Mutex::Lock sync(*this);
    _defaultLocator = defaultLocator;
}

LocatorPrx
IceInternal::ReferenceFactory::getDefaultLocator() const
{
    IceUtil::Mutex::Lock sync(*this);
    return _defaultLocator;
}

IceInternal::ReferenceFactory::ReferenceFactory(const InstancePtr& instance) :
    _instance(instance),
    _referencesHint(_references.end()),
    _evict(0)
{
}

void
IceInternal::ReferenceFactory::destroy()
{
    Mutex::Lock sync(*this);

    if(!_instance)
    {
	throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    _instance = 0;
    _defaultRouter = 0;
    _defaultLocator = 0;
    _references.clear();
    _referencesHint = _references.end();
}
