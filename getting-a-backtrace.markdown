---
layout: default
title: Getting a backtrace
---

If Spectrum is crashing, it's useful to get backtrace to help us to find the reason. To get
a backtrace you have to have debugging symbols installed or compiled Spectrum with them.

###Installing debugging symbols
1. If you are installing from our [Debian/Ubuntu repository](debian-ubuntu-installation.html), you can just install debugging symbols
with this command:

		sudo apt-get install spectrum-dbg

2. If you build Spectrum by yourself, you have to build it in Debug mode.

		cmake . -DCMAKE_BUILD_TYPE=Debug
		make
		sudo make install

###Installing GDB

	sudo apt-get install gdb

### Getting a backtrace from coredump
This is preferred method how to get the backtrace. At first you have to run Spectrum in debug mode to let it generate coredumps:

	spectrumctl --debug -c /etc/spectrum/your_config_file.cfg start

Now reproduce the crash and Spectrum will generate coredump (file named like "core.12345" when numbers are Spectrum process ID) in userdir (that directory is configurable in config file, default
value is /var/lib/spectrum/$jid/userdir). Now you just have to get the backtrace from coredump:

	cd /var/lib/spectrum/$jid/userdir
	gdb spectrum core.12345
	bt full

### Getting a backtrace by running Spectrum in GDB
Now you have all dependencies installed and you can obtain a backtrace. Run Spectrum in GDB:

	gdb --args spectrum -n config_name

where "config_name" is name of config you have in /etc/spectrum (You can also specify full path to config instead of its name).

You will see something like this:

	GNU gdb (GDB) 7.0-ubuntu
	Copyright (C) 2009 Free Software Foundation, Inc.
	License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
	This is free software: you are free to change and redistribute it.
	There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
	and "show warranty" for details.
	This GDB was configured as "i486-linux-gnu".
	For bug reporting instructions, please see:
	<http://www.gnu.org/software/gdb/bugs/>...
	Reading symbols from /home/hanzz/code/test/transport/spectrum...done.
	(gdb)

Now you have to start Spectrum with following GDB command:

	run

Since now Spectrum is running and you have to reproduce the crash or just wait for crash. Then get a backtrace with this GDB command:

	bt full
