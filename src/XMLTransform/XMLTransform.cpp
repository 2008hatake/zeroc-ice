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

#include <IceUtil/InputUtil.h>
#include <Ice/Ice.h>
#include <XMLTransform/XMLTransform.h>
#include <XMLTransform/ErrorReporter.h>
#include <Freeze/DB.h>

#include <Ice/Xerces.h>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/Janitor.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#include <set>

#include <sys/stat.h>

#ifdef _WIN32
#   include <direct.h>
#   include <io.h>
#   define S_ISDIR(mode) ((mode) & _S_IFDIR)
#   define S_ISREG(mode) ((mode) & _S_IFREG)
#else
#   include <unistd.h>
#   include <dirent.h>
#endif

using namespace std;
using namespace Freeze;
using namespace XMLTransform;

//
// Implementation of exception types.
//
XMLTransform::IllegalTransform::IllegalTransform(const char* file, int line) :
    Exception(file, line)
{
}

string
XMLTransform::IllegalTransform::ice_name() const
{
    return "IllegalTransform";
}

void
XMLTransform::IllegalTransform::ice_print(ostream& out) const
{
    Exception::ice_print(out);
    out << ":\n" << reason;
}

::IceUtil::Exception*
XMLTransform::IllegalTransform::ice_clone() const
{
    return new IllegalTransform(*this);
}

void
XMLTransform::IllegalTransform::ice_throw() const
{
    throw *this;
}

XMLTransform::IncompatibleSchema::IncompatibleSchema(const char* file, int line) :
    Exception(file, line)
{
}

string
XMLTransform::IncompatibleSchema::ice_name() const
{
    return "IncompatibleSchema";
}

void
XMLTransform::IncompatibleSchema::ice_print(ostream& out) const
{
    Exception::ice_print(out);
    out << ":\n" << reason;
}

::IceUtil::Exception*
XMLTransform::IncompatibleSchema::ice_clone() const
{
    return new IncompatibleSchema(*this);
}

void
XMLTransform::IncompatibleSchema::ice_throw() const
{
    throw *this;
}

XMLTransform::InvalidSchema::InvalidSchema(const char* file, int line) :
    Exception(file, line)
{
}

string
XMLTransform::InvalidSchema::ice_name() const
{
    return "InvalidSchema";
}

void
XMLTransform::InvalidSchema::ice_print(ostream& out) const
{
    Exception::ice_print(out);
    out << ":\n" << reason;
}

::IceUtil::Exception*
XMLTransform::InvalidSchema::ice_clone() const
{
    return new InvalidSchema(*this);
}

void
XMLTransform::InvalidSchema::ice_throw() const
{
    throw *this;
}

XMLTransform::SchemaViolation::SchemaViolation(const char* file, int line) :
    Exception(file, line)
{
}

string
XMLTransform::SchemaViolation::ice_name() const
{
    return "SchemaViolation";
}

void
XMLTransform::SchemaViolation::ice_print(ostream& out) const
{
    Exception::ice_print(out);
    out << ":\n" << reason;
}

::IceUtil::Exception*
XMLTransform::SchemaViolation::ice_clone() const
{
    return new SchemaViolation(*this);
}

void
XMLTransform::SchemaViolation::ice_throw() const
{
    throw *this;
}

XMLTransform::MissingTypeException::MissingTypeException(const char* file, int line) :
    Exception(file, line)
{
}

string
XMLTransform::MissingTypeException::ice_name() const
{
    return "MissingTypeException";
}

void
XMLTransform::MissingTypeException::ice_print(ostream& out) const
{
    Exception::ice_print(out);
    out << ":\n" << reason;
}

::IceUtil::Exception*
XMLTransform::MissingTypeException::ice_clone() const
{
    return new MissingTypeException(*this);
}

void
XMLTransform::MissingTypeException::ice_throw() const
{
    throw *this;
}

XMLTransform::TransformException::TransformException(const char* file, int line) :
    Exception(file, line)
{
}

string
XMLTransform::TransformException::ice_name() const
{
    return "TransformException";
}

void
XMLTransform::TransformException::ice_print(ostream& out) const
{
    Exception::ice_print(out);
    out << ":\n" << reason;
}

::IceUtil::Exception*
XMLTransform::TransformException::ice_clone() const
{
    return new TransformException(*this);
}

void
XMLTransform::TransformException::ice_throw() const
{
    throw *this;
}

//
// Helper to convert a DOMString to a native string.
//
static string
toString(const XMLCh* s)
{
    //
    // Some DOM Level 2 operations can return 0.
    //
    if(s)
    {
        char* t = ICE_XERCES_NS XMLString::transcode(s);
        string r(t);
        delete[] t;
        return r;
    }
    else
    {
        return string();
    }
}

//
// Helpers for finding named child nodes.
//
static ICE_XERCES_NS DOMNode*
findChild(ICE_XERCES_NS DOMNode* parent, const string& namespaceURI, const string& localname)
{
    ICE_XERCES_NS DOMNodeList* children = parent->getChildNodes();
    for(unsigned int i = 0; i < children->getLength(); ++i)
    {
	ICE_XERCES_NS DOMNode* child = children->item(i);
	if(toString(child->getNamespaceURI()) == namespaceURI && toString(child->getLocalName()) == localname)
	{
	    return child;
	}
    }

    return 0;
}

static ICE_XERCES_NS DOMNode*
findChild(ICE_XERCES_NS DOMNode* parent, const string& name)
{
    ICE_XERCES_NS DOMNodeList* children = parent->getChildNodes();
    for(unsigned int i = 0; i < children->getLength(); ++i)
    {
	ICE_XERCES_NS DOMNode* child = children->item(i);
	if(toString(child->getNodeName()) == name)
	{
	    return child;
	}
    }

    return 0;
}

//
// Helpers to retrieve a named attribute.
//
static string
getAttributeByName(ICE_XERCES_NS DOMNode* node, const string& name)
{
    ICE_XERCES_NS DOMNamedNodeMap* attributes = node->getAttributes();
    string value;
    ICE_XERCES_NS ArrayJanitor<XMLCh> s(ICE_XERCES_NS XMLString::transcode(name.c_str()));
    ICE_XERCES_NS DOMNode* n = attributes->getNamedItem(s.get());
    if(n)
    {
	value = toString(n->getNodeValue());
    }

    return value;
}

//
// Helpers for specific attributes.
//
static string
getTypeAttribute(ICE_XERCES_NS DOMNode* node)
{
    return getAttributeByName(node, "type");
}

static string
getNameAttribute(ICE_XERCES_NS DOMNode* node)
{
    return getAttributeByName(node, "name");
}

//
// Find a file in one of the given directories.
//
static string
findFile(const string& name, const Ice::StringSeq& paths)
{
    struct stat st;

    //
    // If the file is found in the current working directory,
    // return now.
    //
    if(stat(name.c_str(), &st) == 0)
    {
        return name;
    }

    for(Ice::StringSeq::size_type i = 0; i < paths.size(); i++)
    {
        string file = paths[i] + "/" + name;
        if(stat(file.c_str(), &st) == 0)
        {
            return file;
        }
    }

    return name;
}

//
// DocumentMap. This maps namespace uri -> a set of documents.
//
typedef multimap< string, DocumentInfoPtr> DocumentMap;

//
// Document info holds the DOM root of the document, a map of prefix to namespace URI & the documents
// targetNamespace.
//
XMLTransform::DocumentInfo::DocumentInfo(ICE_XERCES_NS DOMDocument* document, bool releaseDocument,
                                         ICE_XERCES_NS DOMNode* root, const string& targetNamespace) :
    _document(document),
    _releaseDocument(releaseDocument),
    _targetNamespace(targetNamespace)
{
    ICE_XERCES_NS DOMNamedNodeMap* attributes = root->getAttributes();
    unsigned int max = attributes->getLength();
    for(unsigned int i = 0; i < max; ++i)
    {
	ICE_XERCES_NS DOMNode* attribute = attributes->item(i);
	string attrName = toString(attribute->getNodeName());
	if(attrName.substr(0, 5) == "xmlns")
	{
	    string ns;
	    if(attrName.size() > 5)
	    {
		ns = attrName.substr(6);
	    }
	    string uri = toString(attribute->getNodeValue());
	    _nsMap.insert(make_pair(ns, uri));
	}
	else if(attrName == "targetNamespace")
	{
	    _targetNamespace = toString(attribute->getNodeValue());
	}
    }
}

XMLTransform::DocumentInfo::~DocumentInfo()
{
    if(_releaseDocument)
    {
        _document->release();
    }
}

ICE_XERCES_NS DOMDocument*
XMLTransform::DocumentInfo::getDocument() const
{
    return _document;
}

string
XMLTransform::DocumentInfo::findURI(const string& prefix) const
{
    string uri;
    PrefixURIMap::const_iterator q = _nsMap.find(prefix);
    if(q != _nsMap.end())
    {
	uri = q->second;
    }
    return uri;
}

string
XMLTransform::DocumentInfo::getTargetNamespace() const
{
    return _targetNamespace;
}

//
// Convert a QName from prefix:local to local@namespaceURI.
//
static string
convertQName(const string& qname, const DocumentInfoPtr& info)
{
    size_t pos = qname.find(':');
    string prefix;
    string localName;
    if(pos != string::npos)
    {
	prefix = qname.substr(0, pos);
	localName = qname.substr(pos+1);
    }
    else
    {
	localName = qname;
    }

    string uri = info->findURI(prefix);
    if(uri.empty())
    {
	//
	// No namespace - TODO: not InvalidSchema - it's an invalid instance document.
	//
	throw InvalidSchema(__FILE__, __LINE__);
    }
	
    string n = localName;
    n += '@';
    n += uri;

    return n;
}

//
// TODO: These cannot be allocated as static strings since xerces API calls cannot be made until
// XMLPlatformUtils::Initialize has been called.
//
//static DOMString schemaURI("http://www.w3.org/2002/XMLSchema");
//static DOMString schemaElementName("schema");

//
// TODO: All the xs: searching should be replaced by correct searches.
//
static const string sequenceElementName = "xs:sequence";
static const string annotationElementName = "xs:annotation";
static const string appinfoElementName = "xs:appinfo";
static const string importElementName = "xs:import";
static const string includeElementName = "xs:include";
static const string elementElementName = "xs:element";
static const string complexContentElementName = "xs:complexContent";
static const string extensionElementName = "xs:extension";
static const string restrictionElementName = "xs:restriction";
static const string enumerationElementName = "xs:enumeration";

//
// TODO: have print do something sensible.
//

//
// Transform implementations.
//
XMLTransform::Transform::Transform()
{
}

XMLTransform::Transform::~Transform()
{
}

static ::IceUtil::XMLOutput&
operator<<(::IceUtil::XMLOutput& os, ICE_XERCES_NS DOMNode* node)
{
    switch(node->getNodeType())
    {
    case ICE_XERCES_NS DOMNode::TEXT_NODE:
    {
	os << ::IceUtil::startEscapes << toString(node->getNodeValue()) << ::IceUtil::endEscapes;
	break;
    }

    case ICE_XERCES_NS DOMNode::ELEMENT_NODE:
    {
	os << ::IceUtil::se(toString(node->getNodeName()));
	ICE_XERCES_NS DOMNamedNodeMap* attributes = node->getAttributes();
	for(unsigned int i = 0; i < attributes->getLength(); ++i)
	{
	    ICE_XERCES_NS DOMNode* attribute = attributes->item(i);
	    os << ::IceUtil::attr(toString(attribute->getNodeName()), toString(attribute->getNodeValue()));
	}

	ICE_XERCES_NS DOMNode* child = node->getFirstChild();
	while(child)
	{
	    os << child;
	    child = child->getNextSibling();
	}

	os << ::IceUtil::ee;
	break;
    }

    default:
	assert(false);
    }
    return os;
}

namespace XMLTransform
{

//
// Nil transform - this transform does nothing.
//
class NilTransform : public Transform
{
public:

    NilTransform()
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr&, const string&, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap&)
    {
	if(node == 0)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}

	os << node;
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[nil transform]\n";
	return os;
    }
};

//
// Exception transform - this transform should never be invoked,
// since exceptions cannot be members of another type.
//
class ExceptionTransform : public Transform
{
public:

    ExceptionTransform(const string& type) :
        _type(type)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput&, const DocumentInfoPtr&, const string&, ICE_XERCES_NS DOMNode*,
              const MissingTypeMap&)
    {
        throw SchemaViolation(__FILE__, __LINE__);
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[exception transform: " << _type << "]\n";
	return os;
    }

private:

    string _type;
};

typedef ::IceUtil::Handle<NilTransform> NilTransformPtr;

//
// This transform is the root transform for marshaled documents.
//
// TODO: complete, rename
//
class InternalTransform : public Transform
{
public:

    InternalTransform()
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr&, const string&, ICE_XERCES_NS DOMNode*,
              const MissingTypeMap&)
    {
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[internal transform]\n";
	return os;
    }
};

typedef ::IceUtil::Handle<InternalTransform> InternalTransformPtr;

//
// This transform applies to an element. It holds the element name, and applies the contained transform.
//
class ElementTransform : public Transform
{
public:
    
    ElementTransform(const string& namespaceURI, const string& name, TransformPtr transform) :
        _ns(namespaceURI),
	_name(name),
        _transform(transform)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr& info, const string&, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap& map)
    {
	_transform->transform(os, info, _name, node, map);
    }

    virtual void
    checkMissingTypes(const DocumentInfoPtr& info, ICE_XERCES_NS DOMNode* node, const MissingTypeMap& map)
    {
        _transform->checkMissingTypes(info, node, map);
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[element: " << _name << "]\n";
	_transform->print(os);
	os << "[/element: " << _name << "]\n";
	return os;
    }

    const string& namespaceURI() const { return _ns; }
    const string& name() const { return _name; }

private:

    string _ns;
    string _name;

    TransformPtr _transform;
};

typedef ::IceUtil::Handle<ElementTransform> ElementTransformPtr;

//
// This transform validates that the contained enumeration value is still valid.
//
class ValidateEnumerationTransform : public Transform
{
public:

    ValidateEnumerationTransform(const vector<string>& values) :
	_values(values)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr&, const string&, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap&)
    {
	if(node == 0)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}

	ICE_XERCES_NS DOMNode* child = node->getFirstChild();
	if(child == 0 || child->getNodeType() != ICE_XERCES_NS DOMNode::TEXT_NODE)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}
	string value = toString(child->getNodeValue());
	if(find(_values.begin(), _values.end(), value) == _values.end())
	{
	    throw IllegalTransform(__FILE__, __LINE__);
	}
	os << node;
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[validate enumeration]\n";
	return os;
    }

private:
  
    vector<string> _values;
};    

typedef ::IceUtil::Handle<ValidateEnumerationTransform> ValidateEnumerationTransformPtr;

//
// This transform validates the transform of an integer type.
//
class ValidateIntegerTransform : public Transform
{
public:

    ValidateIntegerTransform(const string& from, const string& to) :
	_from(from),
	_to(to)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr&, const string&, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap&)
    {
	if(node == 0)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}

	ICE_XERCES_NS DOMNode* child = node->getFirstChild();
	if(child == 0 || child->getNodeType() != ICE_XERCES_NS DOMNode::TEXT_NODE)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}
	string value = toString(child->getNodeValue());

	string::size_type p;
	Ice::Long v;
	bool rc = IceUtil::stringToInt64(value, v, p);
	assert(rc);
	rc = false; // To keep the compiler happy.

	if(_to == "xs:byte")
	{
	    if(v < SCHAR_MIN || v > SCHAR_MAX)
	    {
		throw IllegalTransform(__FILE__, __LINE__);
	    }
	}
	else if(_to == "xs:short")
	{
	    if(v < SHRT_MIN || v > SHRT_MAX)
	    {
		throw IllegalTransform(__FILE__, __LINE__);
	    }
	}
	else if(_to == "xs:int")
	{
	    if(v < INT_MIN || v > INT_MAX)
	    {
		throw IllegalTransform(__FILE__, __LINE__);
	    }
	}
	else if(_to == "xs:long")
	{
	}
	os << node;
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[validate integer: from=" << _from << " to=" << _to << "]\n";
	return os;
    }

private:
    
    string _from;
    string _to;
};

typedef ::IceUtil::Handle<ValidateIntegerTransform> ValidateIntegerTransformPtr;

//
// This transform validates a transformation of a floating point type.
//
class ValidateFloatTransform : public Transform
{
public:

    ValidateFloatTransform(const string& from, const string& to) :
	_from(from),
	_to(to)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr&, const string&, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap&)
    {
	if(node == 0)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}

	ICE_XERCES_NS DOMNode* child = node->getFirstChild();
	if(child == 0 || child->getNodeType() != ICE_XERCES_NS DOMNode::TEXT_NODE)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}
	os << node;
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[validate float: from=" << _from << " to=" << _to << "]\n";
	return os;
    }

private:
    
    string _from;
    string _to;
};

typedef ::IceUtil::Handle<ValidateFloatTransform> ValidateFloatTransformPtr;

//
// Transform a sequence. The transform holds the transform for the contained elements in the sequence.
//
class SequenceTransform : public Transform
{
public:

    SequenceTransform(TransformPtr transform) :
	_transform(transform)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr& info, const string&, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap& map)
    {
	if(node == 0)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}

	string length = getAttributeByName(node, "length");
	long l = atol(length.c_str());

        //
        // Before we emit anything, we check for missing types.
        // If a MissingTypeException is raised for an element,
        // that element will be skipped. We have to do this in
        // advance in order to properly compute the sequence length.
        //
	ICE_XERCES_NS DOMNodeList* children = node->getChildNodes();
        unsigned int i;
	for(i = 0; i < children->getLength(); ++i)
	{
	    ICE_XERCES_NS DOMNode* child = children->item(i);
	    if(child->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	    {
		continue;
	    }

            try
            {
                _transform->checkMissingTypes(info, child, map);
            }
            catch(const MissingTypeException&)
            {
                --l;
            }
	}

	string name = toString(node->getNodeName());
	os << ::IceUtil::se(name);

        ostringstream lstr;
        lstr << l;

	os << ::IceUtil::attr("length", lstr.str());

	if(l != 0)
	{
	    for(i = 0; i < children->getLength(); ++i)
	    {
		ICE_XERCES_NS DOMNode* child = children->item(i);
		if(child->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
		{
		    continue;
		}

		string nodeName = toString(child->getLocalName());
		if(nodeName != "e")
		{
		    SchemaViolation ex(__FILE__, __LINE__);
		    ex.reason = "invalid sequence element name: "  + nodeName;
		    throw ex;
		}

		try
		{
		    _transform->transform(os, info, nodeName, child, map);
		}
		catch(const MissingTypeException&)
		{
		    // Skip this element
		}
	    }
	}

	os << ::IceUtil::ee;
    }

    virtual void
    checkMissingTypes(const DocumentInfoPtr& info, ICE_XERCES_NS DOMNode* node, const MissingTypeMap& map)
    {
	if(node == 0)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}
	ICE_XERCES_NS DOMNodeList* children = node->getChildNodes();
	for(unsigned int i = 0; i < children->getLength(); ++i)
	{
	    ICE_XERCES_NS DOMNode* child = children->item(i);
	    if(child->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	    {
		continue;
	    }

            _transform->checkMissingTypes(info, child, map);
	}
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[sequence]\n";
	_transform->print(os);
	os << "[/sequence]\n";
	return os;
    }

private:

    TransformPtr _transform;
};

typedef ::IceUtil::Handle<SequenceTransform> SequenceTransformPtr;

//
// Transform the content of a struct. The transform holds a list of name->transform pairs in new instance
// document order. For a given transform the corresponding node is found and the transform is applied.
//
class StructTransform : public Transform
{
public:

    StructTransform()
    {
    }

    StructTransform(const vector<ElementTransformPtr>& transforms) :
	_transforms(transforms)
    {
    }

    void
    setTransforms(const vector<ElementTransformPtr>& transforms)
    {
        _transforms = transforms;
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr& info, const string&, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap& map)
    {
	if(node == 0)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}
	os << ::IceUtil::se(toString(node->getNodeName()));
	ICE_XERCES_NS DOMNamedNodeMap* attributes = node->getAttributes();
	for(unsigned int i = 0; i < attributes->getLength(); ++i)
	{
	    ICE_XERCES_NS DOMNode* attribute = attributes->item(i);
	    os << ::IceUtil::attr(toString(attribute->getNodeName()), toString(attribute->getNodeValue()));
	}

	for(vector<ElementTransformPtr>::const_iterator p = _transforms.begin(); p != _transforms.end(); ++p)
	{
	    ICE_XERCES_NS DOMNode* child = findChild(node, (*p)->namespaceURI(), (*p)->name());
            (*p)->transform(os, info, (*p)->name(), child, map);
	}

	os << ::IceUtil::ee;
    }

    virtual void
    checkMissingTypes(const DocumentInfoPtr& info, ICE_XERCES_NS DOMNode* node, const MissingTypeMap& map)
    {
	if(node == 0)
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}

	for(vector<ElementTransformPtr>::const_iterator p = _transforms.begin(); p != _transforms.end(); ++p)
	{
	    ICE_XERCES_NS DOMNode* child = findChild(node, (*p)->namespaceURI(), (*p)->name());
	    (*p)->checkMissingTypes(info, child, map);
	}
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[struct]\n";
	for(unsigned int i = 0; i < _transforms.size(); ++i)
	{
	    _transforms[i]->print(os);
	}
	os << "[/struct]\n";
	return os;
    }

private:

    vector<ElementTransformPtr> _transforms;
};

typedef ::IceUtil::Handle<StructTransform> StructTransformPtr;

//
// Transform for an object instance. This reads the object type in the xsi:type attribute. Next the specific
// transform is retrieved from the map and applied. The holder of the actual maps owns the lifecycle of the
// transform map.
//
class ClassTransform : public Transform
{
public:

    ClassTransform(TransformMap* transforms) :
	_transforms(transforms)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr& info, const string& name, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap& map)
    {
	//
	// If the object is null simply emit the node as-is.
	//
	string nil = getAttributeByName(node, "xsi:nil");
        if(!nil.empty() && nil == "true")
        {
            os << node;
            return;
        }

        //
        // Otherwise, xsi:type must be present.
        //
	string type = getAttributeByName(node, "xsi:type");
	if(type.empty())
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}

	string n = convertQName(type, info);

	//
	// Technically this is only permitted to be a more derived
	// type - however, this will not be enforced here.
	//
	TransformMap::const_iterator p = _transforms->find(n);
	if(p == _transforms->end())
	{
            //
            // No transformation found for this type. This should
            // have already been discovered by collectMissingTypes.
            //
            string id = getAttributeByName(node, "id");
            assert(!id.empty());
            MissingTypeMap::const_iterator q = map.find(id);
            assert(q != map.end());
            MissingTypeException ex(__FILE__, __LINE__);
            ex.reason = "unable to find a transformation for type `" + q->second + "'";
            throw ex;
	}
	p->second->transform(os, info, name, node, map);
    }

    virtual void
    collectMissingTypes(const DocumentInfoPtr& info, ICE_XERCES_NS DOMNode* node, MissingTypeMap& map)
    {
	//
	// Check if the object is null.
	//
	string nil = getAttributeByName(node, "xsi:nil");
        if(!nil.empty() && nil == "true")
        {
            return;
        }

        //
        // Otherwise, xsi:type must be present.
        //
	string type = getAttributeByName(node, "xsi:type");
	if(type.empty())
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}

	string n = convertQName(type, info);

	//
	// Technically this is only permitted to be a more derived
	// type - however, this will not be enforced here.
	//
	TransformMap::const_iterator p = _transforms->find(n);
	if(p == _transforms->end())
	{
            string id = getAttributeByName(node, "id");
            assert(!id.empty());
            string sliceType = getAttributeByName(node, "type");
            assert(!sliceType.empty());
            map.insert(make_pair(id, sliceType));
	}
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[class]\n";
	return os;
    }

private:

    TransformMap* _transforms;
    
};

typedef ::IceUtil::Handle<ClassTransform> ClassTransformPtr;

//
// Transform for an object reference.
//
class ReferenceTransform : public Transform
{
public:

    ReferenceTransform()
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr& info, const string& name, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap& map)
    {
        //
        // First check for the "href" attribute, which indicates a
        // reference to another object. The referenced object, which
        // is a child of the root node, will be transformed automatically.
        //
        string id = getAttributeByName(node, "href");
        if(!id.empty())
        {
            id.erase(0, 1); // Remove the '#'.
            //
            // If the id is present in the missing type map, then
            // we must raise an exception.
            //
            MissingTypeMap::const_iterator q = map.find(id);
            if(q != map.end())
            {
                MissingTypeException ex(__FILE__, __LINE__);
                ex.reason = "unable to find a transformation for type `" + q->second + "'";
                throw ex;
            }

            os << node;
            return;
        }

	//
	// If there is no "href" attribute, then the object must be nil.
	//
	string nil = getAttributeByName(node, "xsi:nil");
	if(nil.empty())
	{
	    throw SchemaViolation(__FILE__, __LINE__);
	}
        os << node;
    }

    virtual void
    checkMissingTypes(const DocumentInfoPtr& info, ICE_XERCES_NS DOMNode* node, const MissingTypeMap& map)
    {
        //
        // First check for the "href" attribute, which indicates a
        // reference to another object.
        //
        string id = getAttributeByName(node, "href");
        if(!id.empty())
        {
            id.erase(0, 1); // Remove the '#'.
            //
            // If the id is present in the missing types map, then
            // we must raise an exception.
            //
            MissingTypeMap::const_iterator q = map.find(id);
            if(q != map.end())
            {
                MissingTypeException ex(__FILE__, __LINE__);
                ex.reason = "unable to find a transformation for type `" + q->second + "'";
                throw ex;
            }
        }
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[reference]\n";
	return os;
    }
};

typedef ::IceUtil::Handle<ReferenceTransform> ReferenceTransformPtr;

//
// This transform produces a new struct. The contents of the struct will contain whatever default value is
// appropriate.
//
class DefaultInitializedStructTransform : public Transform
{
public:

    DefaultInitializedStructTransform(const vector<ElementTransformPtr>& transforms) :
	_transforms(transforms)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr& info, const string& name, ICE_XERCES_NS DOMNode* node,
              const MissingTypeMap& map)
    {
	os << ::IceUtil::se(name);

	ICE_XERCES_NS DOMNode* child = 0; // Nil

	for(vector<ElementTransformPtr>::const_iterator p = _transforms.begin(); p != _transforms.end(); ++p)
	{
	    (*p)->transform(os, info, (*p)->name(), child, map);
	}

	os << ::IceUtil::ee;
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[empty struct]\n";
	for(unsigned int i = 0; i < _transforms.size(); ++i)
	{
	    _transforms[i]->print(os);
	}
	os << "[/struct end]\n";
	return os;
    }

private:

    vector<ElementTransformPtr> _transforms;
};

typedef ::IceUtil::Handle<DefaultInitializedStructTransform> DefaultInitializedStructTransformPtr;

//
// This transform is used in building up new values. It emits a known tag & string.
//
class EmitStringTransform : public Transform
{
public:

    EmitStringTransform(const string& s) :
	_s(s)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr&, const string& name, ICE_XERCES_NS DOMNode*,
              const MissingTypeMap&)
    {
	os << ::IceUtil::se(name);
	os << ::IceUtil::startEscapes << _s << ::IceUtil::endEscapes;
	os << ::IceUtil::ee;
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[emit string: \"" << _s << "\"]\n";
	return os;
    }

private:

    string _s;
};

typedef ::IceUtil::Handle<EmitStringTransform> EmitStringTransformPtr;

//
// This transform is used in building up new values. It emits a known tag with a given attribute.
//
class EmitAttributeTransform : public Transform
{
public:

    EmitAttributeTransform(const string& attrName, const string& attrValue) :
	_attrName(attrName),
        _attrValue(attrValue)
    {
    }

    virtual void
    transform(::IceUtil::XMLOutput& os, const DocumentInfoPtr&, const string& name, ICE_XERCES_NS DOMNode*,
              const MissingTypeMap&)
    {
	os << ::IceUtil::se(name);
        os << ::IceUtil::attr(_attrName, _attrValue);
	os << ::IceUtil::ee;
    }

    virtual ostream&
    print(ostream& os)
    {
	os << "[emit attribute: " << _attrName << "=\"" << _attrValue << "\"]\n";
	return os;
    }

private:

    string _attrName;
    string _attrValue;

};

typedef ::IceUtil::Handle<EmitAttributeTransform> EmitAttributeTransformPtr;

//
// Transform factory. Process old & new schema document to produce set of transforms between old & new
// instance documents for the described types.
//
class TransformFactory
{
public:

    TransformFactory();
    ~TransformFactory();

    void create(ICE_XERCES_NS DOMDocument*, ICE_XERCES_NS DOMDocument*, const Ice::StringSeq&, const Ice::StringSeq&,
                const Ice::StringSeq&, const Ice::StringSeq&, TransformMap*, TransformMap*);

private:

    enum Type
    {
	TypeBoolean,
	TypeInteger, // byte, short, int, long
	TypeFloat, // float, double
	TypeString,
	TypeEnumeration,
	TypeStruct,
	TypeClass,
	TypeException,
	TypeDictionary,
	TypeSequence,
	TypeProxy,
	TypeReference,
	TypeInternal
    };

    //
    // Load all schemas in a list of directories.
    //
    void load(DocumentMap&, set<string>&, const string&, const Ice::StringSeq&);

    //
    // Schema import/include handling.
    //
    void import(DocumentMap&, set<string>&, const string&, const string&, const Ice::StringSeq&);
    void processImport(ICE_XERCES_NS DOMDocument*, DocumentMap&, set<string>&, const Ice::StringSeq&);

    //
    // Element processing.
    //
    void processElements(const DocumentInfoPtr&);

    //
    // Type searching primitives.
    //
    bool findTypeInDocument(ICE_XERCES_NS DOMDocument*, const string&, ICE_XERCES_NS DOMNode*&);
    bool findType(const DocumentMap&, const DocumentInfoPtr&, const string&, ICE_XERCES_NS DOMNode*&, DocumentInfoPtr&);

    Type getType(ICE_XERCES_NS DOMNode*);
    Type getTypeByName(const DocumentMap&, const DocumentInfoPtr&, const string&, ICE_XERCES_NS DOMNode*&,
                       DocumentInfoPtr&);

    //
    // Top-level transform creation routine.
    //
    TransformPtr createTransform(const DocumentInfoPtr&, const string&,
				 const DocumentInfoPtr&, const string&);

    //
    // Subsequent creation routines.
    //
    TransformPtr createComplexTypeTransform(ICE_XERCES_NS DOMNode*, ICE_XERCES_NS DOMNode*);
    TransformPtr createSimpleTypeTransform(ICE_XERCES_NS DOMNode*, ICE_XERCES_NS DOMNode*);

    TransformPtr createStaticClassTransform(ICE_XERCES_NS DOMNode*, ICE_XERCES_NS DOMNode*);
    void createEnumValues(ICE_XERCES_NS DOMNode*, vector< string>&);
    TransformPtr createEnumerationTransform(ICE_XERCES_NS DOMNode*);

    void createSequenceElementTransform(const DocumentInfoPtr&, ICE_XERCES_NS DOMNode*,
					const DocumentInfoPtr&, ICE_XERCES_NS DOMNode*,
					vector<ElementTransformPtr>&);
    void createClassContentTransform(const DocumentInfoPtr&, ICE_XERCES_NS DOMNode*,
				     const DocumentInfoPtr&, ICE_XERCES_NS DOMNode*,
				     vector<ElementTransformPtr>&);

    //
    // Top-level transform creation routine for new datatypes (that is datatypes which were not contained in
    // the old schema).
    //
    TransformPtr createDefaultInitializedTransform(const DocumentInfoPtr&, const string&);

    //
    // Subsequent creation routines for new datatypes.
    //
    void createDefaultInitializedSequenceElementTransform(const DocumentInfoPtr&, ICE_XERCES_NS DOMNode*,
							  vector<ElementTransformPtr>&);

    //
    // Utility routines.
    //
    ICE_XERCES_NS DOMNode* findSchemaRoot(ICE_XERCES_NS DOMDocument*);

    //
    // Map of local@uri class transforms (based on static type). This information cached for creation of the
    // transform.
    //
    TransformMap* _staticClassTransforms;

    //
    // Map of local@uri element names to transforms. Needed for actual transform.
    //
    TransformMap* _elements;
    
    //
    // Set of documents. The set of all documents is the entire document set.
    //
    // The map maps from targetNamespace to DocumentInfo.
    //
    DocumentMap _fromDocs;
    DocumentMap _toDocs;

    //
    // Map of qualified type names to transform. This information cached for creation of the transform.
    //
    TransformMap _types;

    //
    // Map of local@uri transforms for creating new types. This information cached for creation of the
    // transform.
    //
    TransformMap _defaultInitializedTransforms;

    //
    // Set of already imported file.
    //
    set<string> _fromImportedFiles;
    set<string> _toImportedFiles;

    //
    // Why won't MSVC allow name to be a string?
    //
    struct StringTypeTable
    {
	const char* name; // COMPILERFIX: const string name;
	Type type;
	
	bool operator==(const string& rhs) const
	{
	    return string(name) == rhs;
	}
    };
    
    static const StringTypeTable items[];
    static const StringTypeTable* itemsBegin;
    static const StringTypeTable* itemsEnd;

    static const StringTypeTable itemsByName[];
    static const StringTypeTable* itemsByNameBegin;
    static const StringTypeTable* itemsByNameEnd;
};

} // End of namespace XMLTransform

const TransformFactory::StringTypeTable TransformFactory::items[] =
{
    { "enumeration", TransformFactory::TypeEnumeration },
    { "struct", TransformFactory::TypeStruct },
    { "class", TransformFactory::TypeClass },
    { "exception", TransformFactory::TypeException },
    { "dictionary", TransformFactory::TypeDictionary },
    { "sequence", TransformFactory::TypeSequence },
    { "proxy", TransformFactory::TypeProxy },
    { "reference", TransformFactory::TypeReference },
    { "internal", TransformFactory::TypeInternal }
};
const TransformFactory::StringTypeTable* TransformFactory::itemsBegin = &items[0];
const TransformFactory::StringTypeTable* TransformFactory::itemsEnd = &items[sizeof(items) /
									     sizeof(items[0])];

const TransformFactory::StringTypeTable TransformFactory::itemsByName[] =
{
    { "xs:boolean", TransformFactory::TypeBoolean },
    { "xs:byte", TransformFactory::TypeInteger },
    { "xs:short", TransformFactory::TypeInteger },
    { "xs:int", TransformFactory::TypeInteger },
    { "xs:long", TransformFactory::TypeInteger },
    { "xs:float", TransformFactory::TypeFloat },
    { "xs:double", TransformFactory::TypeFloat },
    { "xs:string", TransformFactory::TypeString },
};
const TransformFactory::StringTypeTable* TransformFactory::itemsByNameBegin = &itemsByName[0];
const TransformFactory::StringTypeTable* TransformFactory::itemsByNameEnd = &itemsByName[sizeof(itemsByName) /
											 sizeof(itemsByName[0])];

//
// Constructor & destructor.
//
XMLTransform::TransformFactory::TransformFactory()
{
}

XMLTransform::TransformFactory::~TransformFactory()
{
}

//
// Member function implementation.
//

//
// Create a transform set for the given old schema (fromDoc) and the new schema (toDoc). The result is two
// maps: A map of local@uri -> element transform and a map of transforms for specific class types.
//
void
XMLTransform::TransformFactory::create(ICE_XERCES_NS DOMDocument* fromDoc, ICE_XERCES_NS DOMDocument* toDoc,
                                       const Ice::StringSeq& loadFrom, const Ice::StringSeq& loadTo,
                                       const Ice::StringSeq& pathFrom, const Ice::StringSeq& pathTo,
                                       TransformMap* elements, TransformMap* staticClassTransforms)
{
    //
    // Setup member state.
    //
    _elements = elements;
    _staticClassTransforms = staticClassTransforms;
    _fromDocs.clear();
    _toDocs.clear();
    _types.clear();
    _defaultInitializedTransforms.clear();

    //
    // Load schemas.
    //
    Ice::StringSeq::size_type i;
    for(i = 0; i < loadFrom.size(); i++)
    {
        load(_fromDocs, _fromImportedFiles, loadFrom[i], pathFrom);
    }
    for(i = 0; i < loadTo.size(); i++)
    {
        load(_toDocs, _toImportedFiles, loadTo[i], pathTo);
    }

    //
    // Create both of the document infos for the old & new schemas.
    // Add the root documents to the document map.
    //
    ICE_XERCES_NS DOMNode* fromSchema = findSchemaRoot(fromDoc);
    assert(fromSchema);
    DocumentInfoPtr fromInfo = new DocumentInfo(fromDoc, false, fromSchema);

    ICE_XERCES_NS DOMNode* toSchema = findSchemaRoot(toDoc);
    assert(toSchema);
    DocumentInfoPtr toInfo = new DocumentInfo(toDoc, false, toSchema);
    
    _fromDocs.insert(make_pair(fromInfo->getTargetNamespace(), fromInfo));
    _toDocs.insert(make_pair(toInfo->getTargetNamespace(), toInfo));

    //
    // Process the import/include declarations for the source schema documents.
    //
    processImport(fromDoc, _fromDocs, _fromImportedFiles, pathFrom);
    processImport(toDoc, _toDocs, _toImportedFiles, pathTo);

    //
    // Finally process each element from the old schema document.
    //
    for(DocumentMap::const_iterator q = _fromDocs.begin(); q != _fromDocs.end(); ++q)
    {
	processElements(q->second);
    }
}

void
XMLTransform::TransformFactory::load(DocumentMap& documents, set<string>& importedFiles, const string& path, 
				     const Ice::StringSeq& paths)
{
    //
    // If the path ends in ".xsd", then assume it's a schema file
    // which we should import. We don't try to stat() the file,
    // because it may be relative to a directory in paths.
    //
    // If the path doesn't in ".xsd", we assume it's a directory
    // and attempt to recursively import all schema files in the
    // directory.
    //
    if(path.rfind(".xsd") != string::npos)
    {
        import(documents, importedFiles, "", path, paths);
    }
    else
    {
#ifdef _WIN32

        struct _finddata_t data;
        long h = _findfirst((path + "/*").c_str(), &data);
        if(h == -1)
        {
            InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "cannot read directory `" + path + "': " + strerror(errno);
            throw ex;
        }
        
        while(true)
        {
            string name = data.name;
            assert(!name.empty());

            struct stat buf;
            string fullPath = path + '/' + name;
            if(::stat(fullPath.c_str(), &buf) == -1)
            {
                InvalidSchema ex(__FILE__, __LINE__);
                ex.reason = "cannot stat `" + fullPath + "': " + strerror(errno);
                throw ex;
            }

            if(S_ISDIR(buf.st_mode))
            {
                if(name != ".." && name != ".")
                {
                    load(documents, importedFiles, fullPath, paths); // Recurse through subdirectories
                }
            }
            else if(S_ISREG(buf.st_mode) && name.rfind(".xsd") != string::npos)
            {
                import(documents, importedFiles, "", fullPath, paths);
            }

            if(_findnext(h, &data) == -1)
            {
                if(errno == ENOENT)
                {
                    break;
                }

                InvalidSchema ex(__FILE__, __LINE__);
                ex.reason = "cannot read directory `" + path + "': " + strerror(errno);
                _findclose(h);
                throw ex;
            }
        }

        _findclose(h);

#else

        struct dirent **namelist;
        int n = ::scandir(path.c_str(), &namelist, 0, alphasort);
        if(n < 0)
        {
            InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "cannot read directory `" + path + "': " + strerror(errno);
            throw ex;
        }

        for(int i = 0; i < n; ++i)
        {
            string name = namelist[i]->d_name;
            assert(!name.empty());

            free(namelist[i]);

            struct stat buf;
            string fullPath = path + '/' + name;
            if(::stat(fullPath.c_str(), &buf) == -1)
            {
                InvalidSchema ex(__FILE__, __LINE__);
                ex.reason = "cannot stat `" + fullPath + "': " + strerror(errno);
                throw ex;
            }

            if(S_ISDIR(buf.st_mode))
            {
                if(name != ".." && name != ".")
                {
                    load(documents, importedFiles, fullPath, paths); // Recurse through subdirectories
                }
            }
            else if(S_ISREG(buf.st_mode) && name.rfind(".xsd") != string::npos)
            {
                import(documents, importedFiles, "", fullPath, paths);
            }
        }
        
        free(namelist);

#endif
    }
}

void
XMLTransform::TransformFactory::import(DocumentMap& documents, set<string>& importedFiles, const string& ns, 
				       const string& loc, const Ice::StringSeq& paths)
{
    //
    // Find the file and ensure we didn't already imported it.
    //
    string file = findFile(loc, paths);

    DOMTreeErrorReporter errorReporter;
    ICE_XERCES_NS XercesDOMParser parser;
    parser.setValidationScheme(ICE_XERCES_NS AbstractDOMParser::Val_Never);
    parser.setDoNamespaces(true);
    parser.setErrorHandler(&errorReporter);

    string systemId;

    try
    {
	ICE_XERCES_NS ArrayJanitor<XMLCh> s(ICE_XERCES_NS XMLString::transcode(file.c_str()));
	ICE_XERCES_NS LocalFileInputSource source(s.get());

	systemId = toString(source.getSystemId());

	if(importedFiles.find(systemId) != importedFiles.end())
	{
	    //
	    // Already imported, nothing to do.
	    //
	    return;
	}

        parser.parse(source);
    }
    catch(const ICE_XERCES_NS XMLException& ex)
    {
	InvalidSchema e(__FILE__, __LINE__);
        e.reason = "XML exception while importing " + loc + ":\n" + toString(ex.getMessage());
	throw e;
    }
    catch(const ICE_XERCES_NS SAXParseException& ex)
    {
	InvalidSchema e(__FILE__, __LINE__);
        e.reason = "SAX exception while importing " + loc + ":\n" + toString(ex.getMessage());
	throw e;
    }
    catch(...)
    {
	InvalidSchema e(__FILE__, __LINE__);
        e.reason = "unknown exception while importing " + loc;
	throw e;
    }

    if(errorReporter.getSawErrors())
    {
	InvalidSchema ex(__FILE__, __LINE__);
	ex.reason = "errors while importing " + loc + ":\n" + errorReporter.getErrors();
	throw ex;
    }

    ICE_XERCES_NS DOMDocument* document = parser.getDocument();
    ICE_XERCES_NS DOMNode* schema = findSchemaRoot(document);

    //
    // For exception safety, we don't call adoptDocument() until after findSchemaRoot().
    // We need to adopt the document because otherwise the parser would release it
    // when the parser is destroyed.
    //
    parser.adoptDocument();

    DocumentInfoPtr info = new DocumentInfo(document, true, schema, ns);
    documents.insert(make_pair(info->getTargetNamespace(), info));

    //
    // Add the file to the list of imported files.
    //
    importedFiles.insert(systemId);

    //
    // Process any imports or includes in the imported document.
    //
    processImport(document, documents, importedFiles, paths);
}

void
XMLTransform::TransformFactory::processImport(ICE_XERCES_NS DOMDocument* parent, DocumentMap& documents, 
					      set<string>& importedFiles, const Ice::StringSeq& paths)
{
    ICE_XERCES_NS DOMNode* schema = findSchemaRoot(parent);
    assert(schema);

    ICE_XERCES_NS DOMNode* child = schema->getFirstChild();
    while(child)
    {
	string nodeName = toString(child->getNodeName());
	if(nodeName == importElementName)
	{
	    string ns = getAttributeByName(child, "namespace");
	    string loc = getAttributeByName(child, "schemaLocation");
	    
	    import(documents, importedFiles, ns, loc, paths);
	}
        else if(nodeName == includeElementName)
	{
	    string loc = getAttributeByName(child, "schemaLocation");
	    
	    import(documents, importedFiles, "", loc, paths);
	}
	child = child->getNextSibling();
    }
}

void
XMLTransform::TransformFactory::processElements(const DocumentInfoPtr& info)
{
    ICE_XERCES_NS DOMNode* schema = findSchemaRoot(info->getDocument());

    ICE_XERCES_NS DOMNodeList* children = schema->getChildNodes();
    for(unsigned int i = 0; i < children->getLength(); ++i)
    {
	ICE_XERCES_NS DOMNode* child = children->item(i);
	if(child->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	{
	    continue;
	}

	string nodeName = toString(child->getNodeName());
	if(nodeName != elementElementName)
	{
	    continue;
	}

	//
	// The element name must be an unqualified name.
	//
	string nameAttr = getNameAttribute(child);
	assert(nameAttr.find(':') == string::npos);

	ICE_XERCES_NS DOMNode* to;
	DocumentInfoPtr toNodeInfo; // Overrides the top-level toInfo.
	if(!findType(_toDocs, info, nameAttr, to, toNodeInfo))
	{
	    //
	    // No equivalent in the new schema.
	    //
	    continue;
	}
	assert(to && toNodeInfo);

	string toNodeName = toString(to->getNodeName());
	string toNameAttr = getNameAttribute(to);
	assert(toNameAttr == nameAttr); // Sanity check.
	
	//
	// Construct the full element name - local@uri.
 	//
	string fullElementName = nameAttr;
	fullElementName += '@';
	fullElementName += info->getTargetNamespace();
	
/*
 * We need to allow redefinitions, otherwise importing
 * will fail. For example, this occurs if we import two
 * schemas which both internally import the same schema.
 *
	//
	// Redefinitions of elements is not permitted.
	//
	if(_elements->find(fullElementName) != _elements->end())
	{
	    InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "redefinition of element " + nameAttr;
	    throw ex;
	}
 */
	
	string fromTypeName = getTypeAttribute(child);
	string toTypeName = getTypeAttribute(to);

	//
	// Ignore anonymous elements (operation contents).
	//
	if(fromTypeName.empty() && toTypeName.empty())
	{
	    continue;
	}

	//
	// However, it's not legal for an element to change type.
	//
	if(fromTypeName.empty() || toTypeName.empty())
	{
	    InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "element " + nameAttr + " has changed type";
	    throw ex;
	}

	TransformPtr transform = createTransform(info, fromTypeName, toNodeInfo, toTypeName);

	//
	// Add association between name & transform
	//
	_elements->insert(make_pair(fullElementName, transform));
    }
}

//
// Search for a type in a particular document. Return true if the type is present, false otherwise.
//
bool
XMLTransform::TransformFactory::findTypeInDocument(ICE_XERCES_NS DOMDocument* doc, const string& local,
                                                   ICE_XERCES_NS DOMNode*& target)
{
    ICE_XERCES_NS DOMNode* schema = findSchemaRoot(doc);

    ICE_XERCES_NS DOMNodeList* children = schema->getChildNodes();
    unsigned int i;
    for(i = 0; i < children->getLength(); ++i)
    {
	ICE_XERCES_NS DOMNode* child = children->item(i);
	if(child->getNodeType() == ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	{
	    string na = getNameAttribute(child);
	    if(na == local)
	    {
		target = child;
		return true;
	    }
	}
    }

    return false;
}

//
// This find method takes the type QName and then maps the prefix portion to a namespace in the current
// document (info). Next the documents which have the given targetNamespace are searched for an element node
// with that name. Return true if the type is found, false otherwise.
//
bool
XMLTransform::TransformFactory::findType(const DocumentMap& docs, const DocumentInfoPtr& info, const string& type,
                                         ICE_XERCES_NS DOMNode*& n, DocumentInfoPtr& nInfo)
{
    string uri;
    string local;

    size_t pos = type.find(':');
    if(pos != string::npos)
    {
	string prefix = type.substr(0, pos);
	uri = info->findURI(prefix);
	if(uri.empty())
	{
	    InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "no URI found for prefix " + prefix;
	    throw ex;
	}
	local = type.substr(pos + 1);
    }
    else
    {
	uri = info->getTargetNamespace();
	local = type;
    }

    //
    // Run through each document with targetNamespace == uri. If the type is contained in the document, then
    // return the node & the document info.
    //
    pair<DocumentMap::const_iterator, DocumentMap::const_iterator> range = docs.equal_range(uri);
    for(DocumentMap::const_iterator q = range.first; q != range.second; ++q)
    {
	if(findTypeInDocument(q->second->getDocument(), local, n))
	{
	    nInfo = q->second;
	    return true;
	}
    }

    return false;
}

TransformFactory::Type
XMLTransform::TransformFactory::getType(ICE_XERCES_NS DOMNode* node)
{
    //
    // Check the appinfo element for the actual type.
    //
    string type;

    //
    // Locate the annotation/appinfo/type node.
    //
    ICE_XERCES_NS DOMNode* child = findChild(node, annotationElementName);
    if(child)
    {
	child = findChild(child, appinfoElementName);
	if(child)
	{
	    child = findChild(child, "type");
	    if(child)
	    {
		child = child->getFirstChild();
		if(child)
		{
		    type = toString(child->getNodeValue());
		}
	    }
	}
    }

    const StringTypeTable* p = find(itemsBegin, itemsEnd, type);
    if(p == itemsEnd)
    {
        InvalidSchema ex(__FILE__, __LINE__);
        ex.reason = "no type found for element " + getNameAttribute(node);
	throw ex;
    }

    return p->type;
}

TransformFactory::Type
XMLTransform::TransformFactory::getTypeByName(const DocumentMap& docs, const DocumentInfoPtr& info, const string& type,
                                              ICE_XERCES_NS DOMNode*& n, DocumentInfoPtr& nInfo)
{
    //
    // First check to see if the type is a primitive schema type.
    //
    const StringTypeTable* p = find(itemsByNameBegin, itemsByNameEnd, type);
    if(p != itemsByNameEnd)
    {
	return p->type;
    }

    if(!findType(docs, info, type, n, nInfo))
    {
	InvalidSchema ex(__FILE__, __LINE__);
        ex.reason = "no node found for type " + type;
	throw ex;
    }
    assert(n && nInfo);

    return getType(n);
}

TransformPtr
XMLTransform::TransformFactory::createTransform(const DocumentInfoPtr& fromTypeInfo, const string& fromTypeName,
                                                const DocumentInfoPtr& toTypeInfo, const string& toTypeName)
{
    ICE_XERCES_NS DOMNode* from;
    DocumentInfoPtr fromInfo;
    Type fromType = getTypeByName(_fromDocs, fromTypeInfo, fromTypeName, from, fromInfo);

    ICE_XERCES_NS DOMNode* to;
    DocumentInfoPtr toInfo;
    Type toType = getTypeByName(_toDocs, toTypeInfo, toTypeName, to, toInfo);

    TransformMap::const_iterator p = _types.find(fromTypeName);
    if(p != _types.end())
    {
	return p->second;
    }

    TransformPtr transform = 0;

    //
    // First handle transforms where the types are equivalent.
    //
    if(fromType == toType)
    {
	switch(fromType)
	{
	case TypeBoolean:
	{
	    //
	    // Don't cache this transform.
	    //
	    return new NilTransform();
	}

	case TypeInteger:
	{
            //
            // Don't cache this transform.
            //
	    return new ValidateIntegerTransform(fromTypeName, toTypeName);
	}

	case TypeFloat:
	{
            //
            // Don't cache this transform.
            //
	    return new ValidateFloatTransform(fromTypeName, toTypeName);
	}

	case TypeString:
	case TypeProxy: // Same as string
	{
	    transform = new NilTransform();
	    break;
	}

	case TypeReference:
	{
	    transform = new ReferenceTransform();
	    break;
	}

	case TypeEnumeration:
	{
	    //
	    // If the type names are not the same then it's illegal.
	    //
	    // TODO: This doesn't allow the renaming of types. By removing this comparison renaming of types
	    // would be permitted. Should this be permitted?
	    //
	    if(fromTypeName != toTypeName)
	    {
		IllegalTransform ex(__FILE__, __LINE__);
                ex.reason = "renaming types is not supported (from " + fromTypeName + " to " + toTypeName + ")";
		throw ex;
	    }

	    transform = createEnumerationTransform(to);
	    break;
	}

	case TypeStruct:
	{
	    //
	    // If the type names are not the same then it's illegal.
	    //
	    // TODO: This doesn't allow the renaming of types. By removing this comparison renaming of types
	    // would be permitted. Should this be permitted?
	    //
	    if(fromTypeName != toTypeName)
	    {
		IllegalTransform ex(__FILE__, __LINE__);
                ex.reason = "renaming types is not supported (from " + fromTypeName + " to " + toTypeName + ")";
		throw ex;
	    }

	    vector<ElementTransformPtr> v;
	    createSequenceElementTransform(fromInfo, findChild(from, sequenceElementName),
					   toInfo, findChild(to, sequenceElementName), v);
	    transform = new StructTransform(v);
	    break;
	}

        //
        // A dictionary is treated as a sequence of struct.
        //
	case TypeDictionary:
	case TypeSequence:
	{
	    //
	    // If the type names are not the same then it's illegal.
	    //
	    // TODO: This doesn't allow the renaming of types. By removing this comparison renaming of types
	    // would be permitted. Should this be permitted?
	    //
	    if(fromTypeName != toTypeName)
	    {
		IllegalTransform ex(__FILE__, __LINE__);
                ex.reason = "renaming types is not supported (from " + fromTypeName + " to " + toTypeName + ")";
		throw ex;
	    }

	    ICE_XERCES_NS DOMNode* fromSeq = findChild(from, sequenceElementName);
	    ICE_XERCES_NS DOMNode* toSeq = findChild(to, sequenceElementName);
	    if(fromSeq == 0 || toSeq == 0)
	    {
		InvalidSchema ex(__FILE__, __LINE__);
                ex.reason = "missing sequence element in " + fromTypeName;
		throw ex;
	    }
	    
	    //
	    // Sequences have one element - which contains the type of the sequence.
	    //
	    ICE_XERCES_NS DOMNode* fromElement = findChild(fromSeq, elementElementName);
	    ICE_XERCES_NS DOMNode* toElement = findChild(toSeq, elementElementName);
	    if(fromElement == 0 || toElement == 0)
	    {
		InvalidSchema ex(__FILE__, __LINE__);
                ex.reason = "invalid sequence element in " + fromTypeName;
		throw ex;
	    }

	    transform = new SequenceTransform(createTransform(fromInfo, getTypeAttribute(fromElement),
							      toInfo, getTypeAttribute(toElement)));
	    break;
	}

	case TypeClass:
	{
	    //
	    // If the type names are not the same then it's illegal.
	    //
	    // TODO: This doesn't allow the renaming of types. By removing this comparison renaming of types
	    // would be permitted. Should this be permitted?
	    //
	    if(fromTypeName != toTypeName)
	    {
		IllegalTransform ex(__FILE__, __LINE__);
                ex.reason = "renaming types is not supported (from " + fromTypeName + " to " + toTypeName + ")";
		throw ex;
	    }

	    string type = getNameAttribute(to);
	    type += '@';
	    type += toInfo->getTargetNamespace();
	    
	    if(_staticClassTransforms->find(type) == _staticClassTransforms->end())
	    {
                StructTransform* st = new StructTransform;
                _staticClassTransforms->insert(make_pair(type, st));

                vector<ElementTransformPtr> v;
                createClassContentTransform(fromInfo, from, toInfo, to, v);

                st->setTransforms(v);
            }

	    return new ClassTransform(_staticClassTransforms);
	}

	case TypeInternal:
	{
	    //
	    // No transformation created for internal stuff.
	    //
	    transform = new InternalTransform();
	    break;
	}

	case TypeException:
	{
            //
            // Return a transform which should never be invoked.
            //
	    transform = new ExceptionTransform(fromTypeName);
	    break;
	}

	default:
        {
	    IllegalTransform ex(__FILE__, __LINE__);
	    ex.reason = "invalid type";
	    throw ex;
        }
	}
    }

    if(!transform)
    {
	//
	// Next we have transforms from type-to-type.
	//
	if(fromType == TypeString && toType == TypeEnumeration)
	{
	    //
	    // String to enumeration transform needs to validate the string as a member of the enumeration
	    // values.
	    //
	    transform = createEnumerationTransform(to);
	}
	else if(fromType == TypeEnumeration && toType == TypeString)
	{
	    //
	    // Enumeration to string transform is nil transform.
	    //
	    transform = new NilTransform();
	}
    }

    //
    // TODO: struct->class, class->struct.
    //
    
    if(!transform)
    {
	IllegalTransform ex(__FILE__, __LINE__);
        ex.reason = "cannot transform from " + fromTypeName + " to " + toTypeName;
	throw ex;
    }

    _types.insert(make_pair(fromTypeName, transform));

    return transform;
}

void
XMLTransform::TransformFactory::createSequenceElementTransform(const DocumentInfoPtr& fromInfo,
                                                               ICE_XERCES_NS DOMNode* from,
                                                               const DocumentInfoPtr& toInfo,
                                                               ICE_XERCES_NS DOMNode* to,
                                                               vector<ElementTransformPtr>& v)
{
    //
    // Precondition: The nodes must not be nil.
    //
    if(from == 0 || to == 0)
    {
	throw InvalidSchema(__FILE__, __LINE__);
    }

    //
    // Allowable transforms:
    //
    // * Node added.
    // * Node removed.
    // * Node moved.
    //

    ICE_XERCES_NS DOMNodeList* fromSeqChildren = from->getChildNodes();
    ICE_XERCES_NS DOMNodeList* toSeqChildren = to->getChildNodes();

    //
    // First run through the to set. This loop handles the node
    // remove, and node changed transforms (plus allowable type
    // changes).
    //
    for(unsigned int i = 0; i < toSeqChildren->getLength(); ++i)
    {
	ElementTransformPtr transform;
	ICE_XERCES_NS DOMNode* toChild = toSeqChildren->item(i);
	if(toChild->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	{
	    continue;
	}

	if(toString(toChild->getNodeName()) != elementElementName)
	{
	    InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "expected " + elementElementName + " but found " + toString(toChild->getNodeName());
	    throw ex;
	}

	string toElementName = getNameAttribute(toChild);
	string toTypeName = getTypeAttribute(toChild);

	//
	// Search for the node in the fromSeqChildren list.
	//
	for(unsigned int j = 0; j < fromSeqChildren->getLength(); ++j)
	{
	    ICE_XERCES_NS DOMNode* fromChild = fromSeqChildren->item(j);

	    if(fromChild->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	    {
		// Skip non element nodes.
		continue;
	    }

	    if(toString(fromChild->getNodeName()) != elementElementName)
	    {
		InvalidSchema ex(__FILE__, __LINE__);
                ex.reason = "expected " + elementElementName + " but found " + toString(fromChild->getNodeName());
		throw ex;
	    }

	    string fromElementName = getNameAttribute(fromChild);

	    if(fromElementName == toElementName)
	    {
		transform = new ElementTransform(toInfo->getTargetNamespace(), toElementName,
						 createTransform(fromInfo, getTypeAttribute(fromChild),
								 toInfo, toTypeName));
	    }
	}

	//
	// If there is no transform then this is a new node type. Create a transform to add an empty element
	// of the appropriate type.
	//
	if(!transform)
	{
	    transform = new ElementTransform(toInfo->getTargetNamespace(), toElementName,
					     createDefaultInitializedTransform(toInfo, toTypeName));
	}

	v.push_back(transform);
    }
}

void
XMLTransform::TransformFactory::createClassContentTransform(const DocumentInfoPtr& fromInfo,
                                                            ICE_XERCES_NS DOMNode* from,
                                                            const DocumentInfoPtr& toInfo,
                                                            ICE_XERCES_NS DOMNode* to,
                                                            vector<ElementTransformPtr>& v)
{
    ICE_XERCES_NS DOMNode* fromContent = findChild(from, complexContentElementName);
    ICE_XERCES_NS DOMNode* toContent = findChild(to, complexContentElementName);

    if(fromContent == 0 && toContent == 0)
    {
	//
	// Must be base of a class hierarchy (while this implementation is a little more flexible, with Ice it
	// is limited to ::Ice::Object).
	//
	createSequenceElementTransform(fromInfo, findChild(from, sequenceElementName),
				       toInfo, findChild(to, sequenceElementName), v);
	return;
    }

    if(fromContent == 0 || toContent == 0)
    {
	throw InvalidSchema(__FILE__, __LINE__);
    }

    ICE_XERCES_NS DOMNode* fromExtension = findChild(fromContent, extensionElementName);
    ICE_XERCES_NS DOMNode* toExtension = findChild(toContent, extensionElementName);

    if(fromExtension == 0 || toExtension == 0)
    {
	throw InvalidSchema(__FILE__, __LINE__);
    }

    string fromBaseName = getAttributeByName(fromExtension, "base");
    string toBaseName = getAttributeByName(toExtension, "base");

    //
    // It's not legal to change the base class.
    //
    if(fromBaseName != toBaseName)
    {
	IllegalTransform ex(__FILE__, __LINE__);
        ex.reason = "can't change base class from " + fromBaseName + " to " + toBaseName;
	throw ex;
    }

    ICE_XERCES_NS DOMNode* fromBaseNode;
    DocumentInfoPtr fromBaseInfo;
    if(!findType(_fromDocs, fromInfo, fromBaseName, fromBaseNode, fromBaseInfo))
    {
	SchemaViolation ex(__FILE__, __LINE__);
        ex.reason = "unable to find type for " + fromBaseName;
	throw ex;
    }
    assert(fromBaseNode && fromBaseInfo);

    ICE_XERCES_NS DOMNode* toBaseNode;
    DocumentInfoPtr toBaseInfo;
    if(!findType(_toDocs, toInfo, toBaseName, toBaseNode, toBaseInfo))
    {
	SchemaViolation ex(__FILE__, __LINE__);
        ex.reason = "unable to find type for " + toBaseName;
	throw ex;
    }
    assert(toBaseNode && toBaseInfo);

    //
    // Find the content transform for the base type.
    //
    createClassContentTransform(fromBaseInfo, fromBaseNode, toBaseInfo, toBaseNode, v);

    createSequenceElementTransform(fromInfo, findChild(fromExtension, sequenceElementName),
				   toInfo, findChild(toExtension, sequenceElementName), v);
}

void
XMLTransform::TransformFactory::createEnumValues(ICE_XERCES_NS DOMNode* to, vector<string>& values)
{
    ICE_XERCES_NS DOMNode* toRes = findChild(to, restrictionElementName);
    if(toRes == 0)
    {
	InvalidSchema ex(__FILE__, __LINE__);
        ex.reason = "didn't find " + restrictionElementName + " in " + toString(to->getNodeName());
	throw ex;
    }

    //
    // Gather up a list of allowable values.
    //
    ICE_XERCES_NS DOMNodeList* toResChildren = toRes->getChildNodes();

    for(unsigned int i = 0; i < toResChildren->getLength(); ++i)
    {
	ICE_XERCES_NS DOMNode* toChild = toResChildren->item(i);

	if(toChild->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	{
	    continue;
	}

	if(toString(toChild->getNodeName()) != enumerationElementName)
	{
	    InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "expected " + enumerationElementName + " but found " + toString(toChild->getNodeName());
	    throw ex;
	}

	string value = getAttributeByName(toChild, "value");
	if(value.empty())
	{
	    InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "didn't find value attribute in " + toString(toChild->getNodeName());
	    throw ex;
	}

	values.push_back(value);
    }
}

TransformPtr
XMLTransform::TransformFactory::createEnumerationTransform(ICE_XERCES_NS DOMNode* to)
{
    vector<string> values;
    createEnumValues(to, values);

    return new ValidateEnumerationTransform(values);
}

TransformPtr
XMLTransform::TransformFactory::createDefaultInitializedTransform(const DocumentInfoPtr& info, const string& typeName)
{
    string fullTypeName = convertQName(typeName, info);
    TransformMap::const_iterator p = _defaultInitializedTransforms.find(fullTypeName);
    if(p != _defaultInitializedTransforms.end())
    {
	//
	// Return cached empty transform.
	//
	return p->second;
    }

    ICE_XERCES_NS DOMNode* n;
    DocumentInfoPtr nInfo;
    Type type = getTypeByName(_toDocs, info, typeName, n, nInfo);

    TransformPtr transform = 0;
    switch(type)
    {
    case TypeBoolean:
	transform = new EmitStringTransform("false");
	break;

    case TypeInteger:
	transform = new EmitStringTransform("0");
	break;

    case TypeFloat:
	transform = new EmitStringTransform("0.0");
	break;

    case TypeProxy:
    case TypeReference:
    case TypeString:
	//
	// Default string, reference & proxy is empty.
	//
	transform = new EmitStringTransform("");
	break;

    case TypeEnumeration:
    {
	if(n == 0)
	{
	    throw InvalidSchema(__FILE__, __LINE__);
	}

	vector<string> values;
	createEnumValues(n, values);
	transform = new EmitStringTransform(values[0]);
	break;
    }
	
    case TypeStruct:
    {
	if(n == 0)
	{
	    throw InvalidSchema(__FILE__, __LINE__);
	}

	ICE_XERCES_NS DOMNode* seq = findChild(n, sequenceElementName);
	if(seq == 0)
	{
	    InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "didn't find " + sequenceElementName + " in struct";
	    throw ex;
	}
	
	vector<ElementTransformPtr> v;
	createDefaultInitializedSequenceElementTransform(info, seq, v);
	transform = new DefaultInitializedStructTransform(v);
	break;
    }

    case TypeDictionary:
    case TypeSequence:
    {
	if(n == 0)
	{
	    throw InvalidSchema(__FILE__, __LINE__);
	}
	transform = new EmitAttributeTransform("length", "0");
	break;
    }

    case TypeInternal:
    case TypeException:
    case TypeClass:
    default:
    {
	IllegalTransform ex(__FILE__, __LINE__);
        ex.reason = "cannot create default initialized value";
	throw ex;
    }
    }

    if(!transform)
    {
	IllegalTransform ex(__FILE__, __LINE__);
        ex.reason = "can't create default transform for " + typeName;
	throw ex;
    }

    _defaultInitializedTransforms.insert(make_pair(fullTypeName, transform));

    return transform;
}

void
XMLTransform::TransformFactory::createDefaultInitializedSequenceElementTransform(const DocumentInfoPtr& info,
                                                                                 ICE_XERCES_NS DOMNode* node,
                                                                                 vector<ElementTransformPtr>& v)
{
    //
    // Allowable transforms:
    //
    // * Node added.
    // * Node removed.
    // * Node moved.
    //

    ICE_XERCES_NS DOMNodeList* seqChild = node->getChildNodes();
    for(unsigned int i = 0; i < seqChild->getLength(); ++i)
    {
	ICE_XERCES_NS DOMNode* child = seqChild->item(i);
	if(child->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	{
	    continue;
	}

	if(toString(child->getNodeName()) != elementElementName)
	{
	    InvalidSchema ex(__FILE__, __LINE__);
            ex.reason = "expected " + elementElementName + " but found " + toString(child->getNodeName());
	    throw ex;
	}

	v.push_back(new ElementTransform(info->getTargetNamespace(), getNameAttribute(child),
					 createDefaultInitializedTransform(info, getTypeAttribute(child))));
    }
}

ICE_XERCES_NS DOMNode*
XMLTransform::TransformFactory::findSchemaRoot(ICE_XERCES_NS DOMDocument* root)
{
    ICE_XERCES_NS ArrayJanitor<XMLCh> schemaURI(ICE_XERCES_NS XMLString::transcode("http://www.w3.org/2002/XMLSchema"));
    ICE_XERCES_NS ArrayJanitor<XMLCh> schemaLocalName(ICE_XERCES_NS XMLString::transcode("schema"));

    ICE_XERCES_NS DOMNodeList* nodes = root->getElementsByTagNameNS(schemaURI.get(), schemaLocalName.get());
    if(nodes->getLength() != 1)
    {
        InvalidSchema ex(__FILE__, __LINE__);
        ex.reason = "unable to find schema root";
	throw ex;
    }
    return nodes->item(0);
}


XMLTransform::Transformer::Transformer(const Ice::StringSeq& loadFrom, const Ice::StringSeq& loadTo,
                                       const Ice::StringSeq& pathFrom, const Ice::StringSeq& pathTo,
                                       ICE_XERCES_NS DOMDocument* fromDoc, ICE_XERCES_NS DOMDocument* toDoc)
{
    TransformFactory factory;
    factory.create(fromDoc, toDoc, loadFrom, loadTo, pathFrom, pathTo, &_elements, &_staticClassTransforms);
}

XMLTransform::Transformer::~Transformer()
{
}

void
XMLTransform::Transformer::transform(::IceUtil::XMLOutput& os, ICE_XERCES_NS DOMDocument* doc,
                                     const string& primaryElement, bool force, bool emitRoot)
{
    ICE_XERCES_NS DOMNode* root = doc->getFirstChild();

    unsigned int i;

    if(emitRoot)
    {
        os << ::IceUtil::se(toString(root->getNodeName()));
        ICE_XERCES_NS DOMNamedNodeMap* attributes = root->getAttributes();
        for(i = 0; i < attributes->getLength(); ++i)
        {
            ICE_XERCES_NS DOMNode* attribute = attributes->item(i);
            os << ::IceUtil::attr(toString(attribute->getNodeName()), toString(attribute->getNodeValue()));
        }
    }

    DocumentInfoPtr info = new DocumentInfo(doc, false, root);
    
    ICE_XERCES_NS DOMNodeList* children = root->getChildNodes();

    //
    // Collect the missing types from all elements before
    // transforming.
    //
    Transform::MissingTypeMap map;
    for(i = 0; i < children->getLength(); ++i)
    {
	ICE_XERCES_NS DOMNode* child = children->item(i);
	if(child->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	{
	    continue;
	}

	string nodeName = toString(child->getNodeName());

	//
	// Create local@namespace version of the element name.
	//
	string n = convertQName(nodeName, info);

	TransformMap::const_iterator p = _elements.find(n);
	if(p == _elements.end())
	{
	    SchemaViolation ex(__FILE__, __LINE__);
	    ex.reason = "cannot find element " + n;
	    throw ex;
	}

        p->second->collectMissingTypes(info, child, map);
    }

    //
    // If there are missing types, and we are not told to
    // force removal of objects, then raise an exception.
    //
    if(!map.empty() && !force)
    {
        set<string> types;
        for(Transform::MissingTypeMap::const_iterator q = map.begin(); q != map.end(); ++q)
        {
            types.insert(q->second);
        }
        ostringstream ostr;
        ostr << "The following types could not be found:";
        for(set<string>::const_iterator r = types.begin(); r != types.end(); ++r)
        {
            ostr << endl << "  " << *r;
        }
        MissingTypeException ex(__FILE__, __LINE__);
        ex.reason = ostr.str();
        throw ex;
    }

    //
    // Transform.
    //
    for(i = 0; i < children->getLength(); ++i)
    {
	ICE_XERCES_NS DOMNode* child = children->item(i);
	if(child->getNodeType() != ICE_XERCES_NS DOMNode::ELEMENT_NODE)
	{
	    continue;
	}

	string nodeName = toString(child->getNodeName());

	//
	// Create local@namespace version of the element name.
	//
	string n = convertQName(nodeName, info);

	TransformMap::const_iterator p = _elements.find(n);
	if(p == _elements.end())
	{
	    SchemaViolation ex(__FILE__, __LINE__);
	    ex.reason = "cannot find element " + n;
	    throw ex;
	}

        try
        {
            p->second->transform(os, info, nodeName, child, map);
        }
        catch(const MissingTypeException&)
        {
            //
            // If a missing type is encountered in the primary element,
            // we cannot ignore it. Otherwise, we ignore the exception
            // if force is true.
            //
            if(nodeName == primaryElement || !force)
            {
                throw;
            }
        }
    }

    if(emitRoot)
    {
        os << ::IceUtil::ee;
    }
}

XMLTransform::DBTransformer::DBTransformer(const DBEnvironmentPtr& dbEnv, const DBPtr& db,
                                           const Ice::StringSeq& loadOld, const Ice::StringSeq& loadNew,
                                           const Ice::StringSeq& pathOld, const Ice::StringSeq& pathNew,
                                           bool force) :
    _dbEnv(dbEnv), _db(db), _loadOld(loadOld), _loadNew(loadNew), _pathOld(pathOld), _pathNew(pathNew),
    _force(force)
{
    try
    {
        ICE_XERCES_NS XMLPlatformUtils::Initialize();
    }
    catch(const ICE_XERCES_NS XMLException& ex)
    {
        cerr << "Error during xerces initialization: " << toString(ex.getMessage()) << endl;
        throw Ice::SyscallException(__FILE__, __LINE__); // TODO: Better exception?
    }
}

XMLTransform::DBTransformer::~DBTransformer()
{
    ICE_XERCES_NS XMLPlatformUtils::Terminate();
}

void
XMLTransform::DBTransformer::transform(ICE_XERCES_NS DOMDocument* oldSchema, ICE_XERCES_NS DOMDocument* newSchema)
{
    DOMTreeErrorReporter errReporter;

    ICE_XERCES_NS XercesDOMParser parser;
    parser.setValidationScheme(ICE_XERCES_NS AbstractDOMParser::Val_Auto);
    parser.setDoNamespaces(true);
    parser.setErrorHandler(&errReporter);

    DBCursorPtr cursor;
    DBTransactionPtr txn;
    string reason;
    try
    {
        Transformer transformer(_loadOld, _loadNew, _pathOld, _pathNew, oldSchema, newSchema);

        //
        // Header and footer for instance documents.
        //
        const string header = "<ice:data xmlns=\"http://www.noorg.org/schemas\""
                              " xmlns:ice=\"http://www.zeroc.com/schemas\""
                              " xmlns:xsi=\"http://www.w3.org/2002/XMLSchema-instance\""
                              " xsi:schemaLocation=\"http://www.noorg.org/schemas Dummy.xsd\">";
        const string footer = "</ice:data>";

        //
        // The database will be modified in place. Since keys can be changed,
        // we can't use a cursor to perform the changes. We collect all of the
        // keys first, then update the records.
        //
        vector<Key> keys;
        cursor = _db->getCursor();
        do
        {
            Key k;
            Value v;
            cursor->curr(k, v);
            keys.push_back(k);
        }
        while(cursor->next());
        cursor->close();
        cursor = 0;

        txn = _dbEnv->startTransaction();

        vector<Key>::const_iterator p;
        for(p = keys.begin(); p != keys.end(); ++p)
        {
            const Key& k = *p;

            try
            {
                //
                // Transform key
                //
                string fullKey;
                fullKey.append(header);
                fullKey.append(&k[0], k.size());
                fullKey.append(footer);
                ICE_XERCES_NS MemBufInputSource keySource((const XMLByte*)fullKey.data(), fullKey.size(), "key");
                parser.parse(keySource);
                ICE_XERCES_NS DOMDocument* keyDoc = parser.getDocument();

                ostringstream keyStream;
                IceUtil::XMLOutput keyOut(keyStream);
                transformer.transform(keyOut, keyDoc, "Key", _force, false);

                Key newKey;
                const std::string& keyStr = keyStream.str();
                newKey.resize(keyStr.size());
                std::copy(keyStr.begin(), keyStr.end(), newKey.begin());

                //
                // Transform value
                //
                Value value = _db->getWithTxn(txn, k);
                string fullValue;
                fullValue.append(header);
                fullValue.append(&value[0], value.size());
                fullValue.append(footer);
                ICE_XERCES_NS MemBufInputSource valueSource((const XMLByte*)fullValue.data(), fullValue.size(),
                                                            "value");
                parser.parse(valueSource);
                ICE_XERCES_NS DOMDocument* valueDoc = parser.getDocument();

                ostringstream valueStream;
                IceUtil::XMLOutput valueOut(valueStream);
                transformer.transform(valueOut, valueDoc, "Value", _force, false);

                Value newValue;
                const std::string& valueStr = valueStream.str();
                newValue.resize(valueStr.size());
                std::copy(valueStr.begin(), valueStr.end(), newValue.begin());

                //
                // Update database - only insert new key,value pair if the transformed
                // key doesn't match an existing key.
                //
                _db->delWithTxn(txn, k);
                if(_db->containsWithTxn(txn, newKey))
                {
                    reason = "transformed key matches an existing record:\n" + keyStr;
                    txn->abort();
                    txn = 0;
                    break;
                }
                _db->putWithTxn(txn, newKey, newValue);
            }
            catch(const MissingTypeException&)
            {
                //
                // If a missing type is encountered and _force is true,
                // then remove the key,value pair from the database.
                //
                if(_force)
                {
                    _db->delWithTxn(txn, k);
                }
                else
                {
                    throw;
                }
            }
        }

        if(txn)
        {
            txn->commit();
            txn = 0;
        }
    }
    catch(const DBNotFoundException&)
    {
        // Database is empty
    }
    catch(const ICE_XERCES_NS XMLException& ex)
    {
        reason = "XML exception: " + toString(ex.getMessage());
    }   
    catch(const ICE_XERCES_NS SAXException& ex)
    {
        reason = "SAX exception: " + toString(ex.getMessage());
    }
    catch(ICE_XERCES_NS DOMException& ex)
    {
        ostringstream out;
        out << "DOM exception (" << ex.code << ") " << toString(ex.msg);
        reason = out.str();
    }
    catch(...)
    {
        if(cursor)
        {
            cursor->close();
        }
        if(txn)
        {
            txn->abort();
        }
        throw;
    }

    if(cursor)
    {
        cursor->close();
    }

    if(txn)
    {
        txn->abort();
    }

    if(!reason.empty())
    {
        TransformException ex(__FILE__, __LINE__);
        ex.reason = reason;
        throw ex;
    }
}

void
XMLTransform::DBTransformer::transform(const string& oldSchemaFile, const string& newSchemaFile)
{
    DOMTreeErrorReporter errReporter;

    ICE_XERCES_NS XercesDOMParser parser;
    parser.setValidationScheme(ICE_XERCES_NS AbstractDOMParser::Val_Auto);
    parser.setDoNamespaces(true);
    parser.setErrorHandler(&errReporter);

    ICE_XERCES_NS DOMDocument* oldSchema;
    ICE_XERCES_NS DOMDocument* newSchema;

    try
    {
        parser.parse(oldSchemaFile.c_str());
        oldSchema = parser.getDocument();

        parser.parse(newSchemaFile.c_str());
        newSchema = parser.getDocument();
    }
    catch(const ICE_XERCES_NS XMLException& ex)
    {
        InvalidSchema e(__FILE__, __LINE__);
        e.reason = "XML exception: " + toString(ex.getMessage());
        throw e;
    }   
    catch(const ICE_XERCES_NS SAXException& ex)
    {
        InvalidSchema e(__FILE__, __LINE__);
        e.reason = "SAX exception: " + toString(ex.getMessage());
        throw e;
    }
    catch(...)
    {   
        InvalidSchema e(__FILE__, __LINE__);
        e.reason = "Unexpected exception";
        throw e;
    }

    transform(oldSchema, newSchema);
}

void
XMLTransform::DBTransformer::transform(const string& schemaStr)
{
    DOMTreeErrorReporter errReporter;

    ICE_XERCES_NS XercesDOMParser parser;
    parser.setValidationScheme(ICE_XERCES_NS AbstractDOMParser::Val_Auto);
    parser.setDoNamespaces(true);
    parser.setErrorHandler(&errReporter);

    ICE_XERCES_NS DOMDocument* schema;

    try
    {
        ICE_XERCES_NS MemBufInputSource source((const XMLByte*)schemaStr.data(), schemaStr.size(), "schema");
        parser.parse(source);
        schema = parser.getDocument();
    }
    catch(const ICE_XERCES_NS XMLException& ex)
    {
        InvalidSchema e(__FILE__, __LINE__);
        e.reason = "XML exception: " + toString(ex.getMessage());
        throw e;
    }   
    catch(const ICE_XERCES_NS SAXException& ex)
    {
        InvalidSchema e(__FILE__, __LINE__);
        e.reason = "SAX exception: " + toString(ex.getMessage());
        throw e;
    }
    catch(...)
    {   
        InvalidSchema e(__FILE__, __LINE__);
        e.reason = "Unexpected exception";
        throw e;
    }

    transform(schema, schema);
}
