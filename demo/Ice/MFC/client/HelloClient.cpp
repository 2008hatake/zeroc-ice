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

#include "stdafx.h"
#include "HelloClient.h"
#include "HelloClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CHelloClientApp, CWinApp)
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

CHelloClientApp::CHelloClientApp()
{
    // Place all significant initialization in InitInstance
}

// The one and only CHelloClientApp object

CHelloClientApp theApp;

BOOL
CHelloClientApp::InitInstance()
{
    // InitCommonControls() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    InitCommonControls();

    CWinApp::InitInstance();

    //
    // Create a communicator.
    //
    Ice::CommunicatorPtr communicator;
    try
    {
        int argc = 0;
        communicator = Ice::initialize(argc, 0);
    }
    catch(const IceUtil::Exception& ex)
    {
        std::ostringstream ostr;
        ostr << ex;
        std::string s = ostr.str();
        AfxMessageBox(CString(s.c_str()), MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }

    CHelloClientDlg dlg(communicator);
    m_pMainWnd = &dlg;
    dlg.DoModal();

    //
    // Clean up.
    //
    try
    {
        communicator->destroy();
    }
    catch(const IceUtil::Exception&)
    {
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    // application, rather than start the application's message pump.
    return FALSE;
}
