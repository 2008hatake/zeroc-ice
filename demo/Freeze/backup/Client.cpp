// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <IntLongMap.h>
#include <IceUtil/Time.h>
#include <Freeze/Freeze.h>

using namespace Freeze;
using namespace Ice;
using namespace IceUtil;
using namespace std;


static void
testFailed(const char* expr, const char* file, unsigned int line)
{
    std::cout << "failed!" << std::endl;
    std::cout << file << ':' << line << ": assertion `" << expr << "' failed" << std::endl;
    abort();
}

#define test(ex) ((ex) ? ((void)0) : testFailed(#ex, __FILE__, __LINE__))



int
main(int argc, char* argv[])
{
    PropertiesPtr properties = Ice::createProperties(argc, argv);
    properties->load("config");

    CommunicatorPtr communicator = initializeWithProperties(argc, argv, properties);

    ConnectionPtr connection = createConnection(communicator, "db");
    IntLongMap m(connection, "IntLongMap", true);
    
    const size_t size = 10000;

    if(m.size() == 0)
    {	
	cout << "********* Creating new map ***********" << endl;
	TransactionHolder txHolder(connection);

	Time time = Time::now();
	Int64 ms = time.toMilliSeconds();

	for(size_t i = 0; i < size; ++i)
	{
	    m.insert(IntLongMap::value_type(i, ms));
	}
	txHolder.commit();
    }
   
    cout << "Updating map" << endl;

    for(;;)
    {
	size_t count = 0;

	TransactionHolder txHolder(connection);
	Time time = Time::now();
	Int64 ms = time.toMilliSeconds();

	IntLongMap::iterator p = m.begin();
	Int64 oldMs = p->second;
	do
	{
	    if(p->second != oldMs)
	    {
		cerr << "old time (ms) == " << oldMs << endl;
		cerr << "current current (ms) == " << p->second << endl;
	    }
	    
	    test(p->second == oldMs);
	    p.set(ms);
	    count++;
	} while(++p != m.end());

	cout << "Switching from " << Time::milliSeconds(oldMs).toString() 
	     << " to " << time.toString() << " ... " << flush;
	  
	txHolder.commit();
	cout << "done" << endl;
	test(count == size);
    }

    connection->close();
    communicator->destroy();

    return EXIT_SUCCESS;
}
