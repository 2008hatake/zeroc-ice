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

#include <Ice/Application.h>
#include <ContactFactory.h>
#include <NameIndex.h>
#include <PhoneBookI.h>

using namespace std;
using namespace Ice;
using namespace Freeze;

class PhoneBookServer : public Ice::Application
{
public:
    
    PhoneBookServer(const string& envName) :
	_envName(envName)
    {
    }

    virtual int run(int argc, char* argv[]);

private:
    const string _envName;
};

int
main(int argc, char* argv[])
{
    PhoneBookServer app("db");
    return app.main(argc, argv, "config");
}

int
PhoneBookServer::run(int argc, char* argv[])
{
    PropertiesPtr properties = communicator()->getProperties();
    
    //
    // Create and install a factory for contacts.
    //
    ContactFactoryPtr contactFactory = new ContactFactory();
    communicator()->addObjectFactory(contactFactory, "::Contact");
    
    //
    // Create the name index.
    //
    NameIndexPtr index = new NameIndex("name");
    vector<Freeze::IndexPtr> indices;
    indices.push_back(index);

    //
    // Create an evictor for contacts.
    //
    Freeze::EvictorPtr evictor = Freeze::createEvictor(communicator(), _envName, "contacts", indices);

    Int evictorSize = properties->getPropertyAsInt("PhoneBook.EvictorSize");
    if(evictorSize > 0)
    {
	evictor->setSize(evictorSize);
    }

    contactFactory->setEvictor(evictor);
    
    //
    // Create an object adapter, use the evictor as servant locator.
    //
    ObjectAdapterPtr adapter = communicator()->createObjectAdapter("PhoneBook");
    adapter->addServantLocator(evictor, "contact");
    
    //
    // Create the phonebook, and add it to the object adapter.
    //
    PhoneBookIPtr phoneBook = new PhoneBookI(evictor, contactFactory, index);
    adapter->add(phoneBook, stringToIdentity("phonebook"));
    
    //
    // Everything ok, let's go.
    //
    shutdownOnInterrupt();
    adapter->activate();
    communicator()->waitForShutdown();
    ignoreInterrupt();

    return EXIT_SUCCESS;
}
