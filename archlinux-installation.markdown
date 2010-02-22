---
layout: default
title: Installing on Archlinux
---

Originally published at [http://spectrum.repo.barfooze.de/](http://spectrum.repo.barfooze.de/).

first of all, insert the following snippet into your /etc/pacman.conf if you are on i686:

	[spectrum]
	Server = http://spectrum.repo.barfooze.de/i686

and if you are on x86_64:

	[spectrum]
	Server = http://spectrum.repo.barfooze.de/x86_64

after that, if you don't already have libpurple installed, you either install the minimal libpurple (which does not depend on gstreamer, dbus and stuff) and is so ideal for use on servers (in my opinion, but if you like to have the whole X11 environment on your server anyway, just skip this step, my spectrum package depends on libpurple) by executing

	pacman -Sy libpurple-minimal --asdeps

The only thing left to do now is installing spectrum itself, which is done by

	pacman -Sy spectrum-git

After that, you have (hopefully successfully) installed spectrum, and configure it by editing files in /etc/spectrum/.
If you have any questions, send me an E-Mail to wzff.de with crap@ prepended.

I do not take any liability for harmful effects which might be caused by the installation of the software in this repository.

Enjoy!