---
layout: default
title: Installing on Salix OS/Slackware
---

Spectrum and all of it's dependencies are in the official 
[Salix](http://www.salixos.org/) repositories. The packages should be useable
in [Slackware](http://www.slackware.com), too.

### Salix package installation instructions
At first update your slapt-get cache:

	slapt-get -u

You'll probably not need Pidgin with all of it's dependencies, but libpurple is
part of the Pidgin package. So first install Pidgin without deps:

	slapt-get --ignore-dep -i pidgin

After that get Spectrum with all needed deps:

	slapt-get -i spectrum

### Slackware package installation instructions
You can manually download Spectrum and it's dependencies from the Salix repo: 
	http://salix.enialis.net/ 
	
You'll find Spectrum in the [n] category and most of it's deps in the [l]
category. Additional deps are as follows: 
	
	gloox,poco
	
Install the packages with installpkg as usual.

### Configuration
Follow the instructions, that are provided in the README.Slackware file in
/usr/doc/spectrum-$VERSION/:

1. Create a user

		adduser --system --disabled-login --no-create-home \
		  --home /var/lib/spectrum --group spectrum

2. You have to create these 3 directories:

		mkdir -p -m750 /var/lib/spectrum
		mkdir -p -m750 /var/log/spectrum/
		mkdir -p -m750 /var/run/spectrum

3. Owner and group of both dirs have to be set to the previously created ones:

		chown spectrum:spectrum /var/lib/spectrum
		chown spectrum:spectrum /var/run/spectrum
		chown spectrum:spectrum /var/log/spectrum

4. Configure spectrum config files in /etc/spectrum/
   There is already an example cfg file included in current dir. The config
   files need to have ownership root:spectrum and permissions 640!

NOTE: Step 1-3 can easily be done by 'sh config-helper.sh' in current directory.
