// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <TestCommon.h>
#include <IceUtil/CtrlCHandler.h>
#include <IceUtil/Thread.h>
#include <IceUtil/Time.h>

using namespace IceUtil;
using namespace std;

void callback(int signal)
{
    cout << "Handling signal " << signal << endl;
}

int main()
{
    {
	cout << "First ignore CTRL+C and the like for 20 seconds (try it!)" << endl;
	CtrlCHandler handler;
	ThreadControl::sleep(Time::seconds(20));
	
	cout << "Then handling them for another 20 seconds (try it)" << endl;
	handler.setCallback(callback);
	ThreadControl::sleep(Time::seconds(20));		 
    }
    cout << "And another 20 seconds after ~CtrlCHandler" << endl;
    ThreadControl::sleep(Time::seconds(20));

    cout << "ok" << endl;
    return EXIT_SUCCESS;
}
