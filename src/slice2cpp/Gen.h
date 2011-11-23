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

#ifndef GEN_H
#define GEN_H

#include <Slice/Parser.h>
#include <IceUtil/OutputUtil.h>

namespace Slice
{

class Gen : public ::IceUtil::noncopyable
{
public:

    Gen(const std::string&,
	const std::string&,
	const std::string&,
	const std::vector<std::string>&,
	const std::string&,
	const std::string&,
        bool);
    ~Gen();

    bool operator!() const; // Returns true if there was a constructor error

    void generate(const UnitPtr&);

private:

    ::IceUtil::Output H;
    ::IceUtil::Output C;

    ::IceUtil::Output implH;
    ::IceUtil::Output implC;

    std::string _base;
    std::string _include;
    std::vector<std::string> _includePaths;
    std::string _dllExport;
    bool _impl;

    class TypesVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	TypesVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);
	virtual bool visitExceptionStart(const ExceptionPtr&);
	virtual void visitExceptionEnd(const ExceptionPtr&);
	virtual bool visitStructStart(const StructPtr&);
	virtual void visitStructEnd(const StructPtr&);
	virtual void visitSequence(const SequencePtr&);
	virtual void visitDictionary(const DictionaryPtr&);
	virtual void visitEnum(const EnumPtr&);
	virtual void visitConst(const ConstPtr&);
	virtual void visitDataMember(const DataMemberPtr&);

    private:

	void emitExceptionBase(const ExceptionPtr&, const std::string&);

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class ProxyDeclVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	ProxyDeclVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitUnitStart(const UnitPtr&);
	virtual void visitUnitEnd(const UnitPtr&);
	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual void visitClassDecl(const ClassDeclPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class ProxyVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	ProxyVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitUnitStart(const UnitPtr&);
	virtual void visitUnitEnd(const UnitPtr&);
	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);
	virtual void visitClassDefEnd(const ClassDefPtr&);
	virtual void visitOperation(const OperationPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class DelegateVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	DelegateVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitUnitStart(const UnitPtr&);
	virtual void visitUnitEnd(const UnitPtr&);
	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);
	virtual void visitClassDefEnd(const ClassDefPtr&);
	virtual void visitOperation(const OperationPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class DelegateMVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	DelegateMVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitUnitStart(const UnitPtr&);
	virtual void visitUnitEnd(const UnitPtr&);
	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);
	virtual void visitClassDefEnd(const ClassDefPtr&);
	virtual void visitOperation(const OperationPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class DelegateDVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	DelegateDVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitUnitStart(const UnitPtr&);
	virtual void visitUnitEnd(const UnitPtr&);
	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);
	virtual void visitClassDefEnd(const ClassDefPtr&);
	virtual void visitOperation(const OperationPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class ObjectDeclVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	ObjectDeclVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual void visitClassDecl(const ClassDeclPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class ObjectVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	ObjectVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);
	virtual void visitClassDefEnd(const ClassDefPtr&);
	virtual bool visitExceptionStart(const ExceptionPtr&);
	virtual bool visitStructStart(const StructPtr&);
	virtual void visitOperation(const OperationPtr&);
	virtual void visitDataMember(const DataMemberPtr&);

    private:

	void emitClassBase(const ClassDefPtr&, const std::string&);

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class IceInternalVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	IceInternalVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitUnitStart(const UnitPtr&);
	virtual void visitUnitEnd(const UnitPtr&);
	virtual void visitClassDecl(const ClassDeclPtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class HandleVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	HandleVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual void visitClassDecl(const ClassDeclPtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class ImplVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	ImplVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual bool visitClassDefStart(const ClassDefPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;

        //
        // Generate code to emit a local variable declaration and initialize it
        // if necessary.
        //
        void writeDecl(::IceUtil::Output&, const std::string&, const TypePtr&);

        //
        // Generate code to return a dummy value
        //
        void writeReturn(::IceUtil::Output&, const TypePtr&);
    };

    class AsyncVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	AsyncVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual void visitOperation(const OperationPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };

    class AsyncImplVisitor : public ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

	AsyncImplVisitor(::IceUtil::Output&, ::IceUtil::Output&, const std::string&);

	virtual bool visitUnitStart(const UnitPtr&);
	virtual void visitUnitEnd(const UnitPtr&);
	virtual bool visitModuleStart(const ModulePtr&);
	virtual void visitModuleEnd(const ModulePtr&);
	virtual void visitOperation(const OperationPtr&);

    private:

	::IceUtil::Output& H;
	::IceUtil::Output& C;

	std::string _dllExport;
    };
};

}

#endif
