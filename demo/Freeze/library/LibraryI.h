// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef LIBRARY_I_H
#define LIBRARY_I_H

#include <IceUtil/RWRecMutex.h>
#include <Freeze/Evictor.h>
#include <Library.h>
#include <LibraryTypes.h>
#include <IceUtil/AbstractMutex.h>

class LibraryI : public Library, public IceUtil::RWRecMutex
{
public:

    LibraryI(const Ice::CommunicatorPtr& communicator,
	     const std::string& envName, const std::string& dbName,
	     const Freeze::EvictorPtr& evictor);
    virtual ~LibraryI();

    virtual ::BookPrx createBook(const ::BookDescription&, const Ice::Current&);
    virtual ::BookPrx findByIsbn(const ::std::string&, const Ice::Current&) const;
    virtual ::BookPrxSeq findByAuthors(const ::std::string&, const Ice::Current&) const;
    virtual void setEvictorSize(::Ice::Int, const Ice::Current&);
    virtual void shutdown(const Ice::Current&);

    void remove(const BookDescription&);

private:

    Freeze::EvictorPtr _evictor;

    Freeze::ConnectionPtr _connection;
    //
    // This is a dictionary of authors to a sequence of isbn numbers
    // for efficient lookup of books by authors.
    //
    StringIsbnSeqDict _authors;
};

typedef IceUtil::Handle<LibraryI> LibraryIPtr;

class BookI : public Book, public IceUtil::AbstractMutexReadI<IceUtil::RWRecMutex>
{
public:

    BookI(const LibraryIPtr&);
    virtual ~BookI();

    virtual ::BookDescription getBookDescription(const Ice::Current&) const;
    virtual void destroy(const Ice::Current&);
    virtual void rentBook(const ::std::string&, const Ice::Current&);
    virtual ::std::string getRenterName(const Ice::Current&) const;
    virtual void returnBook(const Ice::Current&);

private:

    LibraryIPtr _library;
    bool _destroyed;
};

#endif
