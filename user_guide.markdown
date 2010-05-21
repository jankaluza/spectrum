---
layout: default
title: Spectrum User Guide
---
###Contents

<div style="padding-left:20px;">
<a href="#1">Chapter 1 - Introduction</a><br/>

<a href="#2">Chapter 2 - Installing Spectrum</a><br/>

<div style="padding-left:20px;">
<a href="#2.1">2.1 - Installing on Debian/Ubuntu</a><br/>
<a href="#2.2">2.2 - Installing on Windows</a><br/>
<a href="#2.3">2.3 - Building from source code on Linux</a><br/>
<a href="#2.4">2.4 - Building from source code on Windows</a><br/>
</div>

<a href="#3">Chapter 3 - Configuring Spectrum</a><br/>

<div style="padding-left:20px;">
<a href="#3.1">3.1 - Basic Configuration</a><br/>
<div style="padding-left:20px;">
<a href="#3.1.1">3.1.1 - Configuring ejabberd to work with Spectrum</a><br/>
<a href="#3.1.2">3.1.2 - Configuring Spectrum to connect Jabber Server</a><br/>
</div>
</div>

<a href="#4">Chapter 4 - Managing Spectrum instances</a><br/>
<div style="padding-left:20px;">
<a href="#4.1">4.1 - spectrumctl</a><br/>
<div style="padding-left:20px;">
<a href="#4.1.1">4.1.1 - spectrumctl Commands</a><br/>
</div>
</div>

</div>




<span id="1"></span>
### Chapter 1 - Introduction
Spectrum is an XMPP transport/gateway. It allows XMPP users to communicate with their friends who are using one of the supported networks. Spectrum is written in C++ and uses the Gloox XMPP library and libpurple for “legacy networks”.

Spectrum is open source and released under the GNU GPL.

Spectrum supports a wide range of different networks such as ICQ, XMPP (Jabber, GTalk, Facebook), AIM, MSN, Yahoo, Twitter, Gadu-Gadu, IRC or SIMPLE.

In comparison with other gateways, Spectrum can handle more users while keeping usage of resources lower. We are aware of installations where Spectrum handles more than a thousand concurrent connections flawlessly.




<span id="2"></span>
### Chapter 2 - Installing Spectrum




<span id="2.1"></span>
### 2.1 - Installing on Debian/Ubuntu
We have APT repositories for [Debian](http://www.debian.org) and 
[Ubuntu](http://www.ubuntu.com) that make it very easy to install Spectrum.
Currently there are packages for Debian Lenny (stable), Debian Squeeze
(testing), and every Ubuntu distribution from Hardy Heron (8.04) to Lucid Lynx
(10.04).

To use the repositories, just add the following lines to /etc/apt/sources.list:

	deb http://packages.spectrum.im <dist> spectrum

where &lt;dist&gt; is either lenny, squeeze, hardy, intrepid, jaunty, karmic or lucid.
You can find your distribution in the file /etc/lsb-release. We also have a
source repository at the same location if you want to build the package
yourself.

####Add GPG key
After you have added the repository, you still have to import the GPG key that
is used to sign the packages in the repository. You can do this in two ways:

1. You can download and add the key manually (apt-key requires root privileges):
	
        wget -O - http://packages.spectrum.im/keys/apt-repository@fsinf.at | apt-key add -

2. You can simply update the repositories and install the fsinf-keyring packages:

        apt-get update
        apt-get install fsinf-keyring
        apt-get update

Don't worry about the warnings that the packages can't be identified, they will
be gone after you installed the fsinf-keyring package.

####Install minimal libpurple

If you don't want the spectrum package to pull in too many dependencies, you can
install the libpurple0-minimal package manually. It pulls in way less
dependencies, but currently thats still quite a few:

	apt-get install libpurple0-minimal

####Install spectrum - stable version

After you have done that, simply do:

	apt-get install spectrum

Note that these repositories pull in quite a few dependencies, depending on the distribution you use. 

#### Install spectrum - development version
If you want to try newest features or help us with testing, you can use development packages. 
The packages are rebuild daily at 6:00 AM CEST.

	apt-get install spectrum-dev

After you have successfully installed spectrum, you can start [configuring spectrum instances](new-spectrum-instances.html).




<span id="2.2"></span>
### 2.2 - Installing on Windows
We provide Windows installer which contains all Spectrum dependencies. You can download it in [download section](http://spectrum.im/download/).




<span id="2.3"></span>
### 2.3 - Building from source code on Linux
Before building Spectrum from source code, you need to install the following dependencies:

* [libpoco](http://pocoproject.org/) – at least version 1.3.3
* [libpurple](http://developer.pidgin.im/wiki/WhatIsLibpurple) (part of the Pidgin project) version 2.6.0 or later.
* [Gloox](http://camaya.net/gloox/) – version 1.0
* [gettext](http://www.gnu.org/software/gettext/)
* [CMake](http://www.cmake.org/)

If you want to build also unit tests, you have to install [CppUnit](http://sourceforge.net/apps/mediawiki/cppunit/index.php?title=Main_Page).

For SpectrumCtl (script for easier starting/stopping Spectrum instances), you have to have [Python](http://python.org/) (2.5 or 2.6) installed.

Once you have these dependencies installed, you can get Spectrum’s source code:

1. Development version

		git clone git://github.com/hanzz/spectrum.git

2. Stable version

	You can get stable version from [Download page](http://spectrum.im/download/source/).

Once you have downloaded the source code, you can build Spectrum:

	cd spectrum
	cmake .
	make
	sudo make install




<span id="2.4"></span>
### 2.4 - Building from source code on Windows
Spectrum can be built on Windows, but only SQLite is supported as storage backend at the moment.
Download Cygwin setup file from [Cygwin homepage](http://cygwin.com/). In installation process, choose also these packages:

	make, patch, unzip, zip, wget, git, subversion, autoconf, automake, gettext-devel, libtool, sqlite3

#### Checkout Spectrum
Open Cygwin Bash Shell and checkout Spectrum from github repository:

	git clone git://github.com/hanzz/spectrum.git
	cd spectrum

#### Compile Spectrum

	sh win-build.sh

On Windows 2000 you have to use:

	sh win-build.sh win2k




<span id="3"></span>
### Chapter 3 - Configuring Spectrum




<span id="3.1"></span>
### 3.1 - Basic Configuration
With spectrum, each protocol implementation runs as its own instance. So if you have an ICQ, an MSN and a QQ transport, spectrum will run three times, each time with a different config file.

Configuration files are located by default in /etc/spectrum directory. You can find an example spectrum.cfg file there. The file itself is well documented, so you simply need to fill out the details. The initscript will only recognize configuration files if the suffix is .cfg.

Configuration file is divided into named sections. The section name appears on a line by itself, in square brackets ([ and ]). All parameters after the section declaration are associated with that section. There is no explicit "end of section" delimiter; sections end at the next section declaration, or the end of the file.




<span id="3.1.1"></span>
### 3.1.1 - Configuring ejabberd to work with Spectrum
1. Edit ejabberd.cfg (by default in /etc/ejabberd/ejabberd.cfg).

2. In the section that says: '{listen,' add those two lines:

		{5437, ejabberd_service, [
			{ip, {127, 0, 0, 1}},
			{access, all},
			{shaper_rule, fast},
			{hosts, ["icq.localhost"],
				[{password, "secret"}]}
			]},
	* 5437 is port which will be used by Spectrum to connect your ejabberd server. Each Spectrum instance has to be connected to ejabberd server with different port.
	* "icq.localhost" is hostname of your Spectrum transport.
	* "secret" is password which will be used by Spectrum authenticate itself to ejabberd server.
	

3. Restart ejabberd and you are done.




<span id="3.1.2"></span>
### 3.1.2 - Configuring Spectrum to connect Jabber Server
To connect Spectrum to Jabber server, you have to change only few variables in config file. Each Spectrum instance has to have it's own configuration gile ([Read about configuration file syntax](#3.1)). At first you have to create copy of default spectrum.cfg for Spectrum instance you're configuring (ICQ is used as example here):

	cd /etc/spectrum
	cp spectrum.cfg icq.cfg

Then you can edit icq.cfg and start configuring new Spectrum instance. Spectrum is by default configured to use SQLite database, and also all directories are preconfigured, so to successfully connect Spectrum to Jabber server, you have to change just these variables:

	[service]
	# enable this spectrum instance.
	enable=1
	
	# one of: aim, facebook, gg, icq, irc, msn, myspace, qq, simple, xmpp, yahoo.
	protocol=icq
	
	# IP address of Jabber server Spectrum should connect to.
	server=127.0.0.1
	
	# Spectrum transport Jabber ID (has to be the same as configured in Jabber server config file).
	jid=$protocol.jaim.im
	
	# Password which will be used by Spectrum to connect Jabber server (Password is configured in Jabber server config file)
	password=secret
	
	# Jabber server port to which Spectrum should connect (port is configured in Jabber server config file)
	port=5347




<span id="4"></span>
### Chapter 4 - Managing Spectrum instances




<span id="4.1"></span>
### 4.1 - spectrumctl
spectrumctl is a small tool written in python which can be used to control instances of the spectrum XMPP transport. Spectrum only handles one legacy network per instance, so you need more than one Spectrum instance if you want support for more than one legacy network. By default, spectrumctl acts on all transports defined in /etc/spectrum/.




<span id="4.1.1"></span>
### 4.1.1 - spectrumctl Commands
* **start, stop, restart**  
	Start, stop or restart given spectrum instances (all of them per default).  

* **list**  
	Show a list of Spectrum configurations (and their corresponding instances with PID if running)

* **reload**  
	Reload given Spectrum configurations. This just causes Spectrum to reopen its log-files, and reload configuration file.

* **stats**  
	Print current usage statistics. Note: This option requires the xmpppy library for python.

* **help**  
	Show the help message, alias for --help.

#### Examples:
* Start all Spectrum instances (defined by config files in /etc/spectrum directory)

		spectrumctl start
* Stop all Spectrum instances (defined by config files in /etc/spectrum directory)

		spectrumctl stop

