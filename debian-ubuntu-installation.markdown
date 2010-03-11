---
layout: default
title: Installing on Debian/Ubuntu
---

We have APT repositories for [Debian](http://www.debian.org) and 
[Ubuntu](http://www.ubuntu.com) that make it very easy to install spectrum.
Currently there are packages for Debian Lenny (stable), Debian Squeeze
(testing), and every Ubuntu distribution from Hardy Heron (8.04) to Karmic Koala
(9.10).

To use the repositories, just add the following lines to /etc/apt/sources.list:

	deb http://packages.spectrum.im <dist> spectrum

where &lt;dist&gt; is either lenny, squeeze, hardy, intrepid, jaunty or karmic.
You can find your distribution in the file /etc/lsb-release. We also have a
source repository at the same location if you want to build the package
yourself.

###Add GPG key
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

###Install minimal libpurple

If you don't want the spectrum package to pull in too many dependencies, you can
install the libpurple0-minimal package manually. It pulls in way less
dependencies, but currently thats still quite a few:

	apt-get install libpurple0-minimal

###Install spectrum - stable version

After you have done that, simply do:

	apt-get install spectrum

Note that these repositories pull in quite a few dependencies, depending on the distribution you use. 

### Install spectrum - development version
If you want to try newest features or help us with testing, you can use development packages. 
The packages are rebuild daily at 6:00 AM CEST.

	apt-get install spectrum-dev

After you have successfully installed spectrum, you can start [configuring spectrum instances](new-spectrum-instances.html).
