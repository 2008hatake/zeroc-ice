# **********************************************************************
#
# Copyright (c) 2003-2013 ZeroC, Inc. All rights reserved.
#
# This copy of Ice is licensed to you under the terms described in the
# ICE_LICENSE file included in this distribution.
#
# **********************************************************************

import Ice, Test, sys

def test(b):
    if not b:
        raise RuntimeError('test assertion failed')

def allTests():

    sys.stdout.write("testing default values... ")
    sys.stdout.flush()

    v = Test.Struct1()
    test(not v.boolFalse)
    test(v.boolTrue)
    test(v.b == 254)
    test(v.s == 16000)
    test(v.i == 3)
    test(v.l == 4)
    test(v.f == 5.1)
    test(v.d == 6.2)
    test(v.str == "foo \\ \"bar\n \r\n\t\013\f\007\b? \007 \007")
    test(v.c1 == Test.Color.red)
    test(v.c2 == Test.Color.green)
    test(v.c3 == Test.Color.blue)
    test(v.nc1 == Test.Nested.Color.red)
    test(v.nc2 == Test.Nested.Color.green)
    test(v.nc3 == Test.Nested.Color.blue)
    test(v.noDefault == '')
    test(v.zeroI == 0)
    test(v.zeroL == 0)
    test(v.zeroF == 0)
    test(v.zeroDotF == 0)
    test(v.zeroD == 0)
    test(v.zeroDotD == 0)
    
    v = Test.Struct2()
    test(v.boolTrue == Test.ConstBool)
    test(v.b == Test.ConstByte)
    test(v.s == Test.ConstShort)
    test(v.i == Test.ConstInt)
    test(v.l == Test.ConstLong)
    test(v.f == Test.ConstFloat)
    test(v.d == Test.ConstDouble)
    test(v.str == Test.ConstString)
    test(v.c1 == Test.ConstColor1)
    test(v.c2 == Test.ConstColor2)
    test(v.c3 == Test.ConstColor3)
    test(v.nc1 == Test.ConstNestedColor1)
    test(v.nc2 == Test.ConstNestedColor2)
    test(v.nc3 == Test.ConstNestedColor3)
    test(v.zeroI == Test.ConstZeroI)
    test(v.zeroL == Test.ConstZeroL)
    test(v.zeroF == Test.ConstZeroF)
    test(v.zeroDotF == Test.ConstZeroDotF)
    test(v.zeroD == Test.ConstZeroD)
    test(v.zeroDotD == Test.ConstZeroDotD)
    
    v = Test.Base()
    test(not v.boolFalse)
    test(v.boolTrue)
    test(v.b == 1)
    test(v.s == 2)
    test(v.i == 3)
    test(v.l == 4)
    test(v.f == 5.1)
    test(v.d == 6.2)
    test(v.str == "foo \\ \"bar\n \r\n\t\013\f\007\b? \007 \007")
    test(v.noDefault == '')
    test(v.zeroI == 0)
    test(v.zeroL == 0)
    test(v.zeroF == 0)
    test(v.zeroDotF == 0)
    test(v.zeroD == 0)
    test(v.zeroDotD == 0)
            
    v = Test.Derived()
    test(not v.boolFalse)
    test(v.boolTrue)
    test(v.b == 1)
    test(v.s == 2)
    test(v.i == 3)
    test(v.l == 4)
    test(v.f == 5.1)
    test(v.d == 6.2)
    test(v.str == "foo \\ \"bar\n \r\n\t\013\f\007\b? \007 \007")
    test(v.c1 == Test.Color.red)
    test(v.c2 == Test.Color.green)
    test(v.c3 == Test.Color.blue)
    test(v.nc1 == Test.Nested.Color.red)
    test(v.nc2 == Test.Nested.Color.green)
    test(v.nc3 == Test.Nested.Color.blue)
    test(v.noDefault == '')
    test(v.zeroI == 0)
    test(v.zeroL == 0)
    test(v.zeroF == 0)
    test(v.zeroDotF == 0)
    test(v.zeroD == 0)
    test(v.zeroDotD == 0)

    v = Test.BaseEx()
    test(not v.boolFalse)
    test(v.boolTrue)
    test(v.b == 1)
    test(v.s == 2)
    test(v.i == 3)
    test(v.l == 4)
    test(v.f == 5.1)
    test(v.d == 6.2)
    test(v.str == "foo \\ \"bar\n \r\n\t\013\f\007\b? \007 \007")
    test(v.noDefault == '')
    test(v.zeroI == 0)
    test(v.zeroL == 0)
    test(v.zeroF == 0)
    test(v.zeroDotF == 0)
    test(v.zeroD == 0)
    test(v.zeroDotD == 0)

    v = Test.DerivedEx()
    test(not v.boolFalse)
    test(v.boolTrue)
    test(v.b == 1)
    test(v.s == 2)
    test(v.i == 3)
    test(v.l == 4)
    test(v.f == 5.1)
    test(v.d == 6.2)
    test(v.str == "foo \\ \"bar\n \r\n\t\013\f\007\b? \007 \007")
    test(v.noDefault == '')
    test(v.c1 == Test.Color.red)
    test(v.c2 == Test.Color.green)
    test(v.c3 == Test.Color.blue)
    test(v.nc1 == Test.Nested.Color.red)
    test(v.nc2 == Test.Nested.Color.green)
    test(v.nc3 == Test.Nested.Color.blue)
    test(v.zeroI == 0)
    test(v.zeroL == 0)
    test(v.zeroF == 0)
    test(v.zeroDotF == 0)
    test(v.zeroD == 0)
    test(v.zeroDotD == 0)

    print("ok")
