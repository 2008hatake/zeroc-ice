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
#include <Parser.h>

using namespace std;
using namespace Ice;
using namespace Freeze;

class PhoneBookCollocated : public Ice::Application
{
public:
    
    PhoneBookCollocated(const string& envName) :
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
    PhoneBookCollocated app("db");
    return app.main(argc, argv, "config");
}

int
PhoneBookCollocated::run(int argc, char* argv[])
{
    PropertiesPtr properties = communicator()->getProperties();

    //
    // Create the name index.
    //
    NameIndexPtr index = new NameIndex("name");
    vector<Freeze::IndexPtr> indices;
    indices.push_back(index);

    //
    // Create an Evictor for contacts.
    //
    Freeze::EvictorPtr evictor = Freeze::createEvictor(communicator(), _envName, "contacts", indices);

    Int evictorSize = properties->getPropertyAsInt("PhoneBook.EvictorSize");
    if(evictorSize > 0)
    {
	evictor->setSize(evictorSize);
    }
    
    //
    // Create an Object Adapter, use the Evictor as Servant Locator.
    //
    ObjectAdapterPtr adapter = communicator()->createObjectAdapter("PhoneBook");
    adapter->addServantLocator(evictor, "contact");
    
    //
    // Create the phonebook, and add it to the Object Adapter.
    //
    PhoneBookIPtr phoneBook = new PhoneBookI(evictor, index);
    adapter->add(phoneBook, stringToIdentity("phonebook"));
    
    //
    // Create and install a factory for contacts.
    //
    ObjectFactoryPtr contactFactory = new ContactFactory(evictor);
    communicator()->addObjectFactory(contactFactory, "::Contact");
    
    //
    // Everything ok, let's go.
    //
    int runParser(int, char*[], const CommunicatorPtr&);
    int status = runParser(argc, argv, communicator());
    adapter->deactivate();
    adapter->waitForDeactivate();

    return status;
}
