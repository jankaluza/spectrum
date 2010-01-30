---
layout: default
title: Building from source code
---

# Building from source code
  
Before building Spectrum from source code, you need to install the following dependencies:

* [libpoco](http://pocoproject.org/) – at least version 1.3.3
* [libpurple](http://developer.pidgin.im/wiki/WhatIsLibpurple) (part of the Pidgin project) version 2.6.0 or later.
* [Gloox](http://camaya.net/gloox/) – version 1.0
* [gettext](http://www.gnu.org/software/gettext/)
* [CMake](http://www.cmake.org/)

If you want to build also unit tests, you have to install [CppUnit](http://sourceforge.net/apps/mediawiki/cppunit/index.php?title=Main_Page).

For SpectrumCtl (script for easier starting/stopping Spectrum instances), you have to have installed:

* [Python](http://python.org/)

Once you have these dependencies installed, you can get Spectrum’s source code:

> git clone git://github.com/hanzz/spectrum.git

After you have downloaded the source code, you can build Spectrum:

> cd spectrum
> cmake .
> make
> sudo make install
