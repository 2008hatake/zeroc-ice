#!/usr/bin/env python
# **********************************************************************
#
# Copyright (c) 2003
# ZeroC, Inc.
# Billerica, MA, USA
#
# All Rights Reserved.
#
# Ice is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License version 2 as published by
# the Free Software Foundation.
#
# **********************************************************************

import os, sys

for toplevel in [".", "..", "../..", "../../..", "../../../.."]:
    toplevel = os.path.normpath(toplevel)
    if os.path.exists(os.path.join(toplevel, "config", "TestUtil.py")):
        break
else:
    raise "can't find toplevel directory!"

sys.path.append(os.path.join(toplevel, "config"))
import TestUtil
import IcePackAdmin

name = os.path.join("IcePack", "deployer")
testdir = os.path.join(toplevel, "test", name)

if TestUtil.isWin32():
    os.environ["PATH"] = testdir + ";" + os.getenv("PATH", "")
else:
    os.environ["LD_LIBRARY_PATH"] = testdir + ":" + os.getenv("LD_LIBRARY_PATH", "")

#
# Start the client.
#
def startClient(options):

    fullClientOptions = TestUtil.clientOptions + \
                        " --Ice.Default.Locator=\"IcePack/Locator:default -p 12346\" " + \
                        options

    print "starting client...",
    clientPipe = os.popen(os.path.join(testdir, "client") + fullClientOptions)
    print "ok"

    for output in clientPipe.xreadlines():
        print output,
    
    clientStatus = clientPipe.close()
    if clientStatus:
        print "failed"

#
# Start IcePack.
#
IcePackAdmin.cleanDbDir(os.path.join(testdir, "db"))
icePackRegistryPipe = IcePackAdmin.startIcePackRegistry("12346", testdir)
icePackNodePipe = IcePackAdmin.startIcePackNode(testdir)

#
# Deploy the application, run the client and remove the application.
#
print "deploying application...",
IcePackAdmin.addApplication(os.path.join(testdir, "application.xml"), "");
print "ok"

startClient("")

print "removing application...",
IcePackAdmin.removeApplication(os.path.join(testdir, "application.xml"));
print "ok"

#
# Deploy the application with some targets to test targets, run the
# client to test targets (-t options) and remove the application.
#
print "deploying application with target...",
IcePackAdmin.addApplication(os.path.join(testdir, "application.xml"), "debug localnode.Server1.manual")
print "ok"

startClient("-t")

print "removing application...",
IcePackAdmin.removeApplication(os.path.join(testdir, "application.xml"));
print "ok"

#
# Shutdown IcePack.
#
IcePackAdmin.shutdownIcePackNode(icePackNodePipe)
IcePackAdmin.shutdownIcePackRegistry(icePackRegistryPipe)

sys.exit(0)
