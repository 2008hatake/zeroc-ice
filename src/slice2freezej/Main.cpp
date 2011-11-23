// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Slice/Preprocessor.h>
#include <Slice/JavaUtil.h>

using namespace std;
using namespace Slice;
using namespace IceUtil;

struct Dict
{
    string name;
    string key;
    string value;
};

struct Index
{
    string name;
    string type;
    string member;
    bool caseSensitive;
};

class FreezeGenerator : public JavaGenerator
{
public:
    FreezeGenerator(const string&, const string&);

    bool generate(UnitPtr&, const Dict&);

    bool generate(UnitPtr&, const Index&);

private:
    string _prog;
};

FreezeGenerator::FreezeGenerator(const string& prog, const string& dir)
    : JavaGenerator(dir),
      _prog(prog)
{
}

bool
FreezeGenerator::generate(UnitPtr& u, const Dict& dict)
{
    static const char* builtinTable[] =
    {
        "java.lang.Byte",
        "java.lang.Boolean",
        "java.lang.Short",
        "java.lang.Integer",
        "java.lang.Long",
        "java.lang.Float",
        "java.lang.Double",
        "java.lang.String",
        "Ice.Object",
        "Ice.ObjectPrx",
        "Ice.LocalObject"
    };

    string name;
    string::size_type pos = dict.name.rfind('.');
    if(pos == string::npos)
    {
        name = dict.name;
    }
    else
    {
        name = dict.name.substr(pos + 1);
    }

    TypeList keyTypes = u->lookupType(dict.key, false);
    if(keyTypes.empty())
    {
        cerr << _prog << ": `" << dict.key << "' is not a valid type" << endl;
        return false;
    }
    TypePtr keyType = keyTypes.front();
    
    TypeList valueTypes = u->lookupType(dict.value, false);
    if(valueTypes.empty())
    {
        cerr << _prog << ": `" << dict.value << "' is not a valid type" << endl;
        return false;
    }
    TypePtr valueType = valueTypes.front();

    if(!open(dict.name))
    {
        cerr << _prog << ": unable to open class " << dict.name << endl;
        return false;
    }

    Output& out = output();

    out << sp << nl << "public class " << name << " extends Freeze.Map";
    out << sb;

    //
    // Constructor
    //
    out << sp << nl << "public" << nl << name << "(Freeze.Connection connection, String dbName, boolean createDb)";
    out << sb;
    out << nl << "super(connection, dbName, createDb);";
    out << eb;

    //
    // encode/decode
    //
    for(int i = 0; i < 2; i++)
    {
        string keyValue;
        TypePtr type;
        bool encaps;

        if(i == 0)
        {
            keyValue = "Key";
            type = keyType;
            //
            // Do not encapsulate keys.
            //
            encaps = false;
        }
        else
        {
            keyValue = "Value";
            type = valueType;
            encaps = true;
        }

        string typeS, valS;
        BuiltinPtr b = BuiltinPtr::dynamicCast(type);
        if(b)
        {
            typeS = builtinTable[b->kind()];
            switch(b->kind())
            {
                case Builtin::KindByte:
                {
                    valS = "((java.lang.Byte)o).byteValue()";
                    break;
                }
                case Builtin::KindBool:
                {
                    valS = "((java.lang.Boolean)o).booleanValue()";
                    break;
                }
                case Builtin::KindShort:
                {
                    valS = "((java.lang.Short)o).shortValue()";
                    break;
                }
                case Builtin::KindInt:
                {
                    valS = "((java.lang.Integer)o).intValue()";
                    break;
                }
                case Builtin::KindLong:
                {
                    valS = "((java.lang.Long)o).longValue()";
                    break;
                }
                case Builtin::KindFloat:
                {
                    valS = "((java.lang.Float)o).floatValue()";
                    break;
                }
                case Builtin::KindDouble:
                {
                    valS = "((java.lang.Double)o).doubleValue()";
                    break;
                }
                case Builtin::KindString:
                case Builtin::KindObject:
                case Builtin::KindObjectProxy:
                case Builtin::KindLocalObject:
                {
                    valS = "((" + typeS + ")o)";
                    break;
                }
            }
        }
        else
        {
            typeS = typeToString(type, TypeModeIn);
            valS = "((" + typeS + ")o)";
        }

        int iter;

        //
        // encode
        //
        out << sp << nl << "public byte[]" << nl << "encode" << keyValue
            << "(Object o, Ice.Communicator communicator)";
        out << sb;
        out << nl << "assert(o instanceof " << typeS << ");";
        out << nl << "IceInternal.BasicStream __os = "
            << "new IceInternal.BasicStream(Ice.Util.getInstance(communicator));";
        out << nl << "try";
        out << sb;
        if(encaps)
        {
            out << nl << "__os.startWriteEncaps();";
        }
        iter = 0;
        writeMarshalUnmarshalCode(out, "", type, valS, true, iter, false);
        if(type->usesClasses())
        {
            out << nl << "__os.writePendingObjects();";
        }
        if(encaps)
        {
            out << nl << "__os.endWriteEncaps();";
        }
        out << nl << "java.nio.ByteBuffer __buf = __os.prepareWrite();";
        out << nl << "byte[] __r = new byte[__buf.limit()];";
        out << nl << "__buf.get(__r);";
        out << nl << "return __r;";
        out << eb;
        out << nl << "finally";
        out << sb;
        out << nl << "__os.destroy();";
        out << eb;
        out << eb;

        //
        // decode
        //
        out << sp << nl << "public Object" << nl << "decode" << keyValue
            << "(byte[] b, Ice.Communicator communicator)";
        out << sb;
        out << nl << "IceInternal.BasicStream __is = new IceInternal.BasicStream(Ice.Util.getInstance(communicator));";
        if(type->usesClasses())
        {
            out << nl << "__is.sliceObjects(false);";
        }
        out << nl << "try";
        out << sb;
        out << nl << "__is.resize(b.length, true);";
        out << nl << "java.nio.ByteBuffer __buf = __is.prepareRead();";
        out << nl << "__buf.position(0);";
        out << nl << "__buf.put(b);";
        out << nl << "__buf.position(0);";
        if(encaps)
        {
            out << nl << "__is.startReadEncaps();";
        }
        iter = 0;
        list<string> metaData;
        string patchParams;
        if((b && b->kind() == Builtin::KindObject) || ClassDeclPtr::dynamicCast(type))
        {
            out << nl << "Patcher __p = new Patcher();";
            patchParams = "__p";
        }
        else
        {
            out << nl << typeS << " __r;";
        }
        if(b)
        {
            switch(b->kind())
            {
                case Builtin::KindByte:
                {
                    out << nl << "__r = new java.lang.Byte(__is.readByte());";
                    break;
                }
                case Builtin::KindBool:
                {
                    out << nl << "__r = new java.lang.Boolean(__is.readBool());";
                    break;
                }
                case Builtin::KindShort:
                {
                    out << nl << "__r = new java.lang.Short(__is.readShort());";
                    break;
                }
                case Builtin::KindInt:
                {
                    out << nl << "__r = new java.lang.Integer(__is.readInt());";
                    break;
                }
                case Builtin::KindLong:
                {
                    out << nl << "__r = new java.lang.Long(__is.readLong());";
                    break;
                }
                case Builtin::KindFloat:
                {
                    out << nl << "__r = new java.lang.Float(__is.readFloat());";
                    break;
                }
                case Builtin::KindDouble:
                {
                    out << nl << "__r = new java.lang.Double(__is.readDouble());";
                    break;
                }
                case Builtin::KindString:
                case Builtin::KindObject:
                case Builtin::KindObjectProxy:
                case Builtin::KindLocalObject:
                {
                    writeMarshalUnmarshalCode(out, "", type, "__r", false, iter, false, metaData, patchParams);
                    break;
                }
            }
        }
        else
        {
            writeMarshalUnmarshalCode(out, "", type, "__r", false, iter, false, metaData, patchParams);
        }
        if(type->usesClasses())
        {
            out << nl << "__is.readPendingObjects();";
        }
        if(encaps)
        {
            out << nl << "__is.endReadEncaps();";
        }
        if((b && b->kind() == Builtin::KindObject) || ClassDeclPtr::dynamicCast(type))
        {
            out << nl << "return __p.value;";
        }
        else
        {
            out << nl << "return __r;";
        }
        out << eb;
        out << nl << "finally";
        out << sb;
        out << nl << "__is.destroy();";
        out << eb;
        out << eb;
    }

    //
    // Patcher class.
    //
    BuiltinPtr b = BuiltinPtr::dynamicCast(valueType);
    if((b && b->kind() == Builtin::KindObject) || ClassDeclPtr::dynamicCast(valueType))
    {
        string typeS = typeToString(valueType, TypeModeIn);
        out << sp << nl << "private static class Patcher implements IceInternal.Patcher";
        out << sb;
        out << sp << nl << "public void" << nl << "patch(Ice.Object v)";
        out << sb;
        if(b)
        {
            out << nl << "value = v;";
        }
        else
        {
            out << nl << "value = (" << typeS << ")v;";
        }
        out << eb;
        out << sp << nl << "public String" << nl << "type()";
        out << sb;
        if(b)
        {
            out << nl << "return \"::Ice::Object\";";
        }
        else
        {
            ClassDeclPtr decl = ClassDeclPtr::dynamicCast(valueType);
            out << nl << "return \"" << decl->scoped() << "\";";
        }
        out << eb;
        out << sp << nl << typeS << " value;";
        out << eb;
    }

    out << eb;

    close();

    return true;
}

bool
FreezeGenerator::generate(UnitPtr& u, const Index& index)
{
    string name;
    string::size_type pos = index.name.rfind('.');
    if(pos == string::npos)
    {
        name = index.name;
    }
    else
    {
        name = index.name.substr(pos + 1);
    }

    TypeList types = u->lookupType(index.type, false);
    if(types.empty())
    {
        cerr << _prog << ": `" << index.type << "' is not a valid type" << endl;
        return false;
    }
    TypePtr type = types.front();

    ClassDeclPtr classDecl = ClassDeclPtr::dynamicCast(type);
    if(classDecl == 0)
    {
	cerr << _prog << ": `" << index.type << "' is not a class" << endl;
        return false;
    }

    DataMemberList dataMembers = classDecl->definition()->allDataMembers();
    DataMemberPtr dataMember = 0;
    DataMemberList::const_iterator p = dataMembers.begin();
    while(p != dataMembers.end() && dataMember == 0)
    {
	if((*p)->name() == index.member)
	{
	    dataMember = *p;
	}
	else
	{
	    ++p;
	}
    }

    if(dataMember == 0)
    {
	cerr << _prog << ": `" << index.type << "' has no data member named `" << index.member << "'" << endl;
        return false;
    }
    
    if(index.caseSensitive == false)
    {
	//
	// Let's check member is a string
	//
	BuiltinPtr memberType = BuiltinPtr::dynamicCast(dataMember->type());
	if(memberType == 0 || memberType->kind() != Builtin::KindString)
	{
	    cerr << _prog << ": `" << index.member << "'is not a string " << endl;
	    return false; 
	}
    }

    string memberTypeString = typeToString(dataMember->type(), TypeModeIn);
    
    if(!open(index.name))
    {
        cerr << _prog << ": unable to open class " << index.name << endl;
        return false;
    }

    Output& out = output();

    out << sp << nl << "public class " << name << " extends Freeze.Index";
    out << sb;

    //
    // Constructors
    //
    out << sp << nl << "public" << nl << name << "(String __indexName, String __facet)";
    out << sb;
    out << nl << "super(__indexName, __facet);";
    out << eb;

    out << sp << nl << "public" << nl << name << "(String __indexName)";
    out << sb;
    out << nl << "super(__indexName, \"\");";
    out << eb;

    //
    // find and count
    //
    out << sp << nl << "public Ice.Identity[]" << nl 
	<< "findFirst(" << memberTypeString << " __index, int __firstN)";
    out << sb;
    out << nl << "return untypedFindFirst(marshalKey(__index), __firstN);";
    out << eb;

    out << sp << nl << "public Ice.Identity[]" << nl 
	<< "find(" << memberTypeString << " __index)";
    out << sb;
    out << nl << "return untypedFind(marshalKey(__index));";
    out << eb;
    
    out << sp << nl << "public int" << nl 
	<< "count(" << memberTypeString << " __index)";
    out << sb;
    out << nl << "return untypedCount(marshalKey(__index));";
    out << eb;

    //
    // Key marshalling
    //
    string typeString = typeToString(type, TypeModeIn);

    out << sp << nl << "protected byte[]" << nl 
	<< "marshalKey(Ice.Object __servant)";
    out << sb;
    out << nl << "if(__servant instanceof " << typeString << ")";
    out << sb;
    out << nl <<  memberTypeString << " __key = ((" << typeString << ")__servant)." << index.member << ";"; 
    out << nl << "return marshalKey(__key);";
    out << eb;
    out << nl << "else";
    out << sb;
    out << nl << "return null;";
    out << eb;
    out << eb;
    
    string valueS = index.caseSensitive ? "__key" : "__key.toLowerCase()";

    out << sp << nl << "private byte[]" << nl 
	<< "marshalKey(" << memberTypeString << " __key)";
    out << sb;
    out << nl << "IceInternal.BasicStream __os = "
	<< "new IceInternal.BasicStream(Ice.Util.getInstance(communicator()));";
    out << nl << "try";
    out << sb;
    int iter = 0;
    writeMarshalUnmarshalCode(out, "", dataMember->type(), valueS, true, iter, false);
    if(type->usesClasses())
    {
	out << nl << "__os.writePendingObjects();";
    }
    out << nl << "java.nio.ByteBuffer __buf = __os.prepareWrite();";
    out << nl << "byte[] __r = new byte[__buf.limit()];";
    out << nl << "__buf.get(__r);";
    out << nl << "return __r;";
    out << eb;
    out << nl << "finally";
    out << sb;
    out << nl << "__os.destroy();";
    out << eb;
    out << eb;

    out << eb;

    close();

    return true;
}


void
usage(const char* n)
{
    cerr << "Usage: " << n << " [options] [slice-files...]\n";
    cerr <<
        "Options:\n"
        "-h, --help                Show this message.\n"
        "-v, --version             Display the Ice version.\n"
        "-DNAME                    Define NAME as 1.\n"
        "-DNAME=DEF                Define NAME as DEF.\n"
        "-UNAME                    Remove any definition for NAME.\n"
        "-IDIR                     Put DIR in the include file search path.\n"
        "--include-dir DIR         Use DIR as the header include directory.\n"
        "--dict NAME,KEY,VALUE     Create a Freeze dictionary with the name NAME,\n"
        "                          using KEY as key, and VALUE as value. This\n"
        "                          option may be specified multiple times for\n"
        "                          different names. NAME may be a scoped name.\n"
	"--index NAME,TYPE,MEMBER[,{case-sensitive|case-insensitive}]\n" 
        "                          Create a Freeze evictor index with the name\n"
        "                          NAME for member MEMBER of class TYPE. This\n"
        "                          option may be specified multiple times for\n"
        "                          different names. NAME may be a scoped name.\n"
        "                          When member is a string, the case can be\n"
        "                          sensitive or insensitive (default is sensitive).\n"
        "--output-dir DIR          Create files in the directory DIR.\n"
	"--depend                  Generate Makefile dependencies.\n"
        "-d, --debug               Print debug messages.\n"
        "--ice                     Permit `Ice' prefix (for building Ice source code only)\n"
        ;
    // Note: --case-sensitive is intentionally not shown here!
}

int
main(int argc, char* argv[])
{
    string cppArgs;
    vector<string> includePaths;
    string include;
    string output;
    bool depend = false;
    bool debug = false;
    bool ice = false;
    bool caseSensitive = false;
    vector<Dict> dicts;
    vector<Index> indices;

    int idx = 1;
    while(idx < argc)
    {
        if(strncmp(argv[idx], "-I", 2) == 0)
        {
            cppArgs += ' ';
            cppArgs += argv[idx];

            string path = argv[idx] + 2;
            if(path.length())
            {
                includePaths.push_back(path);
            }

            for(int i = idx ; i + 1 < argc ; ++i)
            {
                argv[i] = argv[i + 1];
            }
            --argc;
        }
        else if(strncmp(argv[idx], "-D", 2) == 0 || strncmp(argv[idx], "-U", 2) == 0)
        {
            cppArgs += ' ';
            cppArgs += argv[idx];

            for(int i = idx ; i + 1 < argc ; ++i)
            {
                argv[i] = argv[i + 1];
            }
            --argc;
        }
        else if(strcmp(argv[idx], "--dict") == 0)
        {
            if(idx + 1 >= argc || argv[idx + 1][0] == '-')
            {
                cerr << argv[0] << ": argument expected for `" << argv[idx] << "'" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            string s = argv[idx + 1];
            s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
            
            Dict dict;

            string::size_type pos;
            pos = s.find(',');
            if(pos != string::npos)
            {
                dict.name = s.substr(0, pos);
                s.erase(0, pos + 1);
            }
            pos = s.find(',');
            if(pos != string::npos)
            {
                dict.key = s.substr(0, pos);
                s.erase(0, pos + 1);
            }
            dict.value = s;

            if(dict.name.empty())
            {
                cerr << argv[0] << ": " << argv[idx] << ": no name specified" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            if(dict.key.empty())
            {
                cerr << argv[0] << ": " << argv[idx] << ": no key specified" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            if(dict.value.empty())
            {
                cerr << argv[0] << ": " << argv[idx] << ": no value specified" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            dicts.push_back(dict);

            for(int i = idx ; i + 2 < argc ; ++i)
            {
                argv[i] = argv[i + 2];
            }
            argc -= 2;
        }
	else if(strcmp(argv[idx], "--index") == 0)
        {
            if(idx + 1 >= argc || argv[idx + 1][0] == '-')
            {
                cerr << argv[0] << ": argument expected for `" << argv[idx] << "'" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            string s = argv[idx + 1];
            s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
            
	    Index index;

            string::size_type pos;
            pos = s.find(',');
            if(pos != string::npos)
            {
                index.name = s.substr(0, pos);
                s.erase(0, pos + 1);
            }
            pos = s.find(',');
            if(pos != string::npos)
            {
                index.type = s.substr(0, pos);
                s.erase(0, pos + 1);
            }
	    pos = s.find(',');
	    string caseString;
	    if(pos != string::npos)
            {
                index.member = s.substr(0, pos);
                s.erase(0, pos + 1);
		caseString = s;
            }
	    else
	    {
		index.member = s;
		caseString = "case-sensitive";
	    }

            if(index.name.empty())
            {
                cerr << argv[0] << ": " << argv[idx] << ": no name specified" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            if(index.type.empty())
            {
                cerr << argv[0] << ": " << argv[idx] << ": no type specified" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            if(index.member.empty())
            {
                cerr << argv[0] << ": " << argv[idx] << ": no member specified" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }
	    
	    if(caseString != "case-sensitive" && caseString != "case-insensitive")
            {
                cerr << argv[0] << ": " << argv[idx]
		     << ": the case can be `case-sensitive' or `case-insensitive'" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }
	    index.caseSensitive = (caseString == "case-sensitive");

            indices.push_back(index);

            for(int i = idx ; i + 2 < argc ; ++i)
            {
                argv[i] = argv[i + 2];
            }
            argc -= 2;
        }
        else if(strcmp(argv[idx], "-h") == 0 || strcmp(argv[idx], "--help") == 0)
        {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else if(strcmp(argv[idx], "-v") == 0 || strcmp(argv[idx], "--version") == 0)
        {
            cout << ICE_STRING_VERSION << endl;
            return EXIT_SUCCESS;
        }
	else if(strcmp(argv[idx], "--depend") == 0)
	{
	    depend = true;
	    for(int i = idx ; i + 1 < argc ; ++i)
	    {
		argv[i] = argv[i + 1];
	    }
	    --argc;
	}
        else if(strcmp(argv[idx], "-d") == 0 || strcmp(argv[idx], "--debug") == 0)
        {
            debug = true;
            for(int i = idx ; i + 1 < argc ; ++i)
            {
                argv[i] = argv[i + 1];
            }
            --argc;
        }
	else if(strcmp(argv[idx], "--ice") == 0)
	{
	    ice = true;
	    for(int i = idx ; i + 1 < argc ; ++i)
	    {
		argv[i] = argv[i + 1];
	    }
	    --argc;
	}
	else if(strcmp(argv[idx], "--case-sensitive") == 0)
	{
	    caseSensitive = true;
	    for(int i = idx ; i + 1 < argc ; ++i)
	    {
		argv[i] = argv[i + 1];
	    }
	    --argc;
	}
        else if(strcmp(argv[idx], "--include-dir") == 0)
        {
            if(idx + 1 >= argc)
            {
                cerr << argv[0] << ": argument expected for `" << argv[idx] << "'" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            
            include = argv[idx + 1];
            for(int i = idx ; i + 2 < argc ; ++i)
            {
                argv[i] = argv[i + 2];
            }
            argc -= 2;
        }
        else if(strcmp(argv[idx], "--output-dir") == 0)
        {
            if(idx + 1 >= argc)
            {
                cerr << argv[0] << ": argument expected for `" << argv[idx] << "'" << endl;
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            
            output = argv[idx + 1];
            for(int i = idx ; i + 2 < argc ; ++i)
            {
                argv[i] = argv[i + 2];
            }
            argc -= 2;
        }
        else if(argv[idx][0] == '-')
        {
            cerr << argv[0] << ": unknown option `" << argv[idx] << "'" << endl;
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        else
        {
            ++idx;
        }
    }

    if(dicts.empty() && indices.empty())
    {
        cerr << argv[0] << ": no Freeze types specified" << endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    UnitPtr u = Unit::createUnit(true, false, ice, caseSensitive);

    int status = EXIT_SUCCESS;

    for(idx = 1 ; idx < argc ; ++idx)
    {
	if(depend)
	{
	    Preprocessor icecpp(argv[0], argv[idx], cppArgs);
	    icecpp.printMakefileDependencies(Preprocessor::Java);
	}
	else
	{
	    Preprocessor icecpp(argv[0], argv[idx], cppArgs);
	    FILE* cppHandle = icecpp.preprocess(false);

	    if(cppHandle == 0)
	    {
		u->destroy();
		return EXIT_FAILURE;
	    }
	    
	    status = u->parse(cppHandle, debug);

	    if(!icecpp.close())
	    {
		u->destroy();
		return EXIT_FAILURE;
	    }	    
	}
    }

    if(depend)
    {
	return EXIT_SUCCESS;
    }

    if(status == EXIT_SUCCESS)
    {
        u->mergeModules();
        u->sort();

        FreezeGenerator gen(argv[0], output);

        JavaGenerator::validateMetaData(u);

        for(vector<Dict>::const_iterator p = dicts.begin(); p != dicts.end(); ++p)
        {
            try
            {
                if(!gen.generate(u, *p))
                {
                    u->destroy();
                    return EXIT_FAILURE;
                }
            }
            catch(...)
            {
                cerr << argv[0] << ": unknown exception" << endl;
                u->destroy();
                return EXIT_FAILURE;
            }
        }

	for(vector<Index>::const_iterator q = indices.begin(); q != indices.end(); ++q)
        {
            try
            {
                if(!gen.generate(u, *q))
                {
                    u->destroy();
                    return EXIT_FAILURE;
                }
            }
            catch(...)
            {
                cerr << argv[0] << ": unknown exception" << endl;
                u->destroy();
                return EXIT_FAILURE;
            }
        }

    }
    
    u->destroy();

    return status;
}
