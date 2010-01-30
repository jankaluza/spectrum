---
layout: default
title: Installing on Debian/Ubuntu
---

Spectrum has APT repositories for [Debian](http://www.debian.org) and [Ubuntu](http://www.ubuntu.com)
that make it very easy to install spectrum. Currently there are packages for Debian Lenny and every
Ubuntu distribution from Hardy Heron to Karmic Koala.

To use the repositories, just add the following lines to /etc/apt/sources.list:

	deb http://packages.spectrum.im <dist> spectrum
	deb-src http://packages.spectrum.im <dist> spectrum

where <dist> is either lenny, hardy, intrepid, jaunty or karmic. You can find your distribution in
the file /etc/lsb-release.

After you have done that, simply do:
	apt-get update
	apt-get install spectrum

Note that these repositories pull in quite a few dependencies, depending on the distribution you use. 

TODO: How to add the GPG-key

After you have successfully installed spectrum, you can start [configuring spectrum instances](new-spectrum-instances.html).
