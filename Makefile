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

top_srcdir	= .

include $(top_srcdir)/config/Make.rules

SUBDIRS		= src include schema test demo slice

install::
	@if test ! -d $(prefix) ; \
	then \
	    echo "Creating $(prefix)..." ; \
	    mkdir $(prefix) ; \
	    chmod a+rx $(prefix) ; \
	fi

	@if test ! -d $(install_bindir) ; \
	then \
	    echo "Creating $(install_bindir)..." ; \
	    mkdir -p $(install_bindir) ; \
	    chmod a+rx $(install_bindir) ; \
	fi

	@if test ! -d $(install_libdir) ; \
	then \
	    echo "Creating $(install_libdir)..." ; \
	    mkdir -p $(install_libdir) ; \
	    chmod a+rx $(install_libdir) ; \
	fi

	@if test ! -d $(install_includedir) ; \
	then \
	    echo "Creating $(install_includedir)..." ; \
	    mkdir $(install_includedir) ; \
	    chmod a+rx $(install_includedir) ; \
	fi

	@if test ! -d $(install_slicedir) ; \
	then \
	    echo "Creating $(install_slicedir)..." ; \
	    mkdir $(install_slicedir) ; \
	    chmod a+rx $(install_slicedir) ; \
	fi

	@if test ! -d $(install_schemadir) ; \
	then \
	    echo "Creating $(install_schemadir)..." ; \
	    mkdir $(install_schemadir) ; \
	    chmod a+rx $(install_schemadir) ; \
	fi

	@if test ! -d $(install_docdir) ; \
	then \
	    echo "Creating $(install_docdir)..." ; \
	    mkdir $(install_docdir) ; \
	    chmod a+rx $(install_docdir) ; \
	fi

$(EVERYTHING)::
	@for subdir in $(SUBDIRS); \
	do \
	    echo "making $@ in $$subdir"; \
	    ( cd $$subdir && $(MAKE) $@ ) || exit 1; \
	done

doc::
	@( cd doc && $(MAKE) ) || exit 1

install::
	@( cd doc && $(MAKE) install ) || exit 1

clean::
	@( cd doc && $(MAKE) clean ) || exit 1

test::
	@python $(top_srcdir)/allTests.py
