---
layout: default
title: Building from source code on Windows
---

Spectrum can be built on Windows, but only SQLite is supported as storage backend at the moment. This page describes how to do it.
Current way of compilling Spectrum on Windows is very hacky :). If you know how to make it easier, feel free to tell us.

### Installing Cygwin
Download Cygwin setup file from [Cygwin homepage](http://cygwin.com/). In installation process, choose also these packages:

	make, patch, unzip, zip, wget, git, subversion, autoconf, automake, gettext-devel, libtool, sqlite3

### Checkout Spectrum
Open Cygwin Bash Shell and checkout spectrum from github repository:

	git clone git://github.com/hanzz/spectrum.git
	cd spectrum

### Compile Spectrum
At first you have to install libpurple dependencies and download Pidgin. There is Bash script which can do it automatically:

	cd win-deps
	wget http://gaim-extprefs.sf.net/winpidgin-build-fetcher.sh
	sh winpidgin-build-fetcher.sh

You have to update MinGW Win32 API, because this script downloads old version:

	cd win32-dev
	cd mingw
	wget http://downloads.sourceforge.net/project/mingw/MinGW%20API%20for%20MS-Windows/Current%20Release_%20w32api-3.13/w32api-3.13-mingw32-dev.tar.gz?use_mirror=dfn
	tar -xzf w32api-3.13-mingw32-dev.tar.gz
	cd ..
	cd ..

Now you can compile libpurple:

	cd pidgin-2.6.2
	cd libpurple
	make -f Makefile.mingw
	make -f Makefile.mingw install
	cd ..
	cd ..

*Note: If you get some error that perl.exe can’t be fount, you have to install ActivePerl 
1003 by yourself or just remove lines that contains $(PERL_PLUGIN) from libpurple/plugins/Makefile.mingw.
	
### Install G++

	cd win32-dev
	cd mingw
	wget http://downloads.sourceforge.net/project/mingw/GCC%20Version%203/Current%20Release_%20gcc-3.4.5-20060117-3/gcc-g%2B%2B-3.4.5-20060117-3.tar.gz?use_mirror=surfnet
	tar -xzf gcc-g++-3.4.5-20060117-3.tar.gz
	cd ..
	cd ..

### Compile Gloox
At first you have to checkout Gloox from SVN:

	svn co svn://svn.camaya.net/gloox/1.0 gloox
	cd gloox

Then you can compile it:

	sh autogen.sh
	./configure
	make
	cd ..

*Note: If you get something like following error, please follow this [blogpost](http://www.heikkitoivonen.net/blog/2008/11/26/cygwin-upgrades-and-rebaseall/):

	Automatically preparing build ...       4 [main] perl 5168 C:\cygwin\bin\perl.ex
	e: *** fatal error - unable to remap C:\cygwin\lib\perl5\5.10\i686-cygwin\auto\F
	ile\Glob\Glob.dll to same address as parent(0x1550000) != 0x1560000

### Compile libPoco
Libpoco is used as storage backend. Just now there is only SQLite supported on Windows.

	wget http://downloads.sourceforge.net/project/poco/sources/poco-1.3.5/poco-1.3.5-all.tar.gz?use_mirror=dfn
	tar -xzf poco-1.3.5-all.tar.gz
	cd poco-1.3.5-all
	./configure --config=MinGW --no-tests --no-samples

Official libpoco can’t be compiled in Cygwin, so we have to change its build system little bit. You will have to change few files to get it work:

* File: build/rules/global

		find:
		LIBPATH  = $(POCO_BUILD)/$(LIBDIR)
		replace it with:
		LIBPATH  = ../$(LIBDIR)
		
		find:
		OBJPATH  = $(POCO_BUILD)/$(COMPONENT)/$(OBJDIR)
		replace it with:
		OBJPATH  = $(OBJDIR)
		
		find:
		INCLUDE = $(foreach p,$(POCO_ADD_INCLUDE),-I$(p)) -Iinclude $(foreach p,$(COMPONENTS),-I$(POCO_BASE)/$(p)/$(INCDIR))
		replace it with:
		INCLUDE = $(foreach p,$(POCO_ADD_INCLUDE),-I$(p)) -Iinclude $(foreach p,$(COMPONENTS),-I../$(p)/$(INCDIR)) $(foreach p,$(COMPONENTS),-I../../$(p)/$(INCDIR))

* File: Makefile

		find:
		all: libexecs tests sample
		replace it with:
		all: libexecs
		
		find:
		libexecs: $(filter-out $(foreach f,$(OMIT),$f%),$(libexecs))
		replace it with:
		libexecs: Foundation-libexec Data-libexec Data/SQLite-libexec

Now you can compile it:

	make
	cd ..

### Compile Spectrum

	cd ..
	cp win-deps/gloox/src/*.h win-deps/gloox
	aclocal
	automake --add-missing
	autoconf
	./configure
	make

Now you should have spectrum.exe in src/ directory. We can create bin/ directory and move all libraries which spectrum needs into it (TODO: this could be added into Makefile, but I don’t have experiences to do it):

	mkdir bin
	cp src/spectrum.exe bin/spectrum.exe
	cp spectrum.cfg bin/spectrum.cfg bin/spectrum.cfg
	cp win-deps/pidgin-2.6.2/libpurple/libpurple.dll bin/libpurple.dll
	cp win-deps/gtk_installer/gtk_install_files/bin/libgmodule-2.0-0.dll bin/libgmodule-2.0-0.dll
	cp win-deps/gtk_installer/gtk_install_files/bin/libgthread-2.0-0.dll bin/libthread-2.0-0.dll
	cp win-deps/gtk_installer/gtk_install_files/bin/libgobject-2.0-0.dll bin/libobject-2.0-0.dll
	cp win-deps/gtk_installer/gtk_install_files/bin/intl.dll bin/intl.dll
	cp win-deps/win32-dev/libxml2-2.6.30/bin/libxml2.dll bin/libxml2.dll
	cp win-deps/gtk_installer/gtk_install_files/bin/iconv.dll bin/iconv.dll

You have to also copy there libpurple plugins for protocols you want to use. They are located here:

	win-deps/pidgin-2.6.2/win32-install-dir/
