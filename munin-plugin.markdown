---
layout: default
title: Munin plugin
---

Spectrum has a plugin for [Munin](http://munin.projects.linpro.no/) that allows
you to monitor transports usage. If you 
[installed from source](building-from-source-code.html), you can find it in 
munin/spectrum_, the [Debian/Ubuntu package](debian-ubuntu-installation.html)
installs it into /usr/share/munin/plugins/spectrum_.

The plugin requires [xmpppy](http://xmpppy.sourceforge.net/), on Debian/Ubuntu
you can simply install python-xmpp. It also requires that config_interface is
set in all instances that you want to monitor. 

###Installation
This plugin is a [wildcard
plugin](http://munin.projects.linpro.no/wiki/WildcardPlugins), so simply symlink
it to spectrum_&lt;suffix&gt;. The suggest parameter gives you an up-to-date
list of suffixes. In the following example a bash-prompt is shown for clarity:

	root@host:~ $ /usr/share/munin/plugins/spectrum_ suggest
	uptime
	registered
	online
	legacy_registered
	legacy_online
	memory
	messages
	messages_sec
	root@host:~ $ cd /etc/munin/plugins
	root@host:/etc/munin/plugins $ ln -s /usr/share/munin/plugins/spectrum_ spectrum_uptime
	root@host:/etc/munin/plugins $ ln -s /usr/share/munin/plugins/spectrum_ spectrum_registered

... and so forth. 


###Configuration
The plugin does need to run as a user/group that has access to the local socket
configured by the config_interface configuration variable. Here is a minimal
configuration (also see the [official
documentation](http://munin-monitoring.org/wiki/plugin-conf.d) on how do
configure munin plugins):

	[spectrum_*]
	user spectrum
	group spectrum

By default, the plugin will monitor all instances configured in /etc/spectrum.
If you want to use a different directory, you can use the environment variable
"base" to specify a different one. If you do not want to monitor all instances,
you can use the environment variable "cfgs" to monitor only those named
explicitly by a comma-seperated list. If you give an absolute path "base" will
not be used. This more thorough example configuration would only monitor
xmpp.cfg and irc.cfg in /usr/local/etc/spectrum as well as /root/etc/icq.cfg.

	[spectrum_*]
	user spectrum
	group spectrum
	env.base /usr/local/etc/spectrum
	env.cfgs xmpp.cfg,irc.cfg,/root/etc/icq.cfg

You can also set the environment variable "verbose" to any non-empty string,
which will cause the munin-plugin to show each individual value on graphs that
show aggregated values (example: By default, spectrum_messages only displays the
total number of messages send, with the verbose setting, it will also show the
number of incoming and outgoing messages).

###Showcase
There is a showcase of these munin-plugins [here](http://jabber.fsinf.at/stats/fsinf.at/jabber.fsinf.at.html#Transports).
