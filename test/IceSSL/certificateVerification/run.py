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

if TestUtil.protocol != "ssl":
    print "This test may only be run with SSL enabled."
    sys.exit(0)

oldClientOptions = TestUtil.clientOptions
oldServerOptions = TestUtil.serverOptions
oldClientServerOptions = TestUtil.clientServerOptions

TestUtil.clientOptions += " --TestSSL.Client.CertPath=" + os.path.join(toplevel, "test", "IceSSL", "certs")
TestUtil.serverOptions += " --TestSSL.Server.CertPath=" + os.path.join(toplevel, "test", "IceSSL", "certs")
TestUtil.clientServerOptions += " --TestSSL.Client.CertPath=" + os.path.join(toplevel, "test", "IceSSL", "certs") + \
                                " --TestSSL.Server.CertPath=" + os.path.join(toplevel, "test", "IceSSL", "certs")

name = os.path.join("IceSSL", "certificateVerification")
testdir = os.path.join(toplevel, "test", name)

print "testing default certificate verifier."
TestUtil.clientServerTest(name)

print "testing single-certificate certificate verifier."
TestUtil.clientOptions += " --TestSSL.Client.CertificateVerifier=singleCert"
TestUtil.serverOptions += " --TestSSL.Server.CertificateVerifier=singleCert"
TestUtil.clientServerTest(name)

TestUtil.clientOptions = oldClientOptions
TestUtil.serverOptions = oldServerOptions
TestUtil.clientServerOptions = oldClientServerOptions

sys.exit(0)
