---
layout: default
title: Munin plugin
---

Spectrum has a plugin for [Munin](http://munin.projects.linpro.no/) that allows
you to monitor transports usage. If you 
[installed from source](building-from-source-code.html), you can find it in 
munin/spectrum_, the [Debian/Ubuntu package](debian-ubuntu-installation.html)
installs it into /usr/share/munin/plugins/spectrum_. Note that the plugin
actually opens an XMPP connection because there is no other way to get the
current usage from spectrum.

The plugin requires [xmpppy](http://xmpppy.sourceforge.net/), on Debian/Ubuntu
you can simply install python-xmpp.

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
	messages
	memory
	root@host:~ $ cd /etc/munin/plugins
	root@host:/etc/munin/plugins $ ln -s /usr/share/munin/plugins/spectrum_ spectrum_uptime
	root@host:/etc/munin/plugins $ ln -s /usr/share/munin/plugins/spectrum_ spectrum_registered

... and so forth. Note that spectrum currently does not correctly report all
values, so don't worry if some graphs always show 0.


###Configuration
The plugins require some configuration using environment variables, because of
the xmpp-connection you will also have to specify an xmpp-user and password.
Here is an example snippet for your munin-node.conf:

	[spectrum_*]
	env.jids one.example.com two.example.com
	env.bot &lt;jid&gt;
	env.password &lt;password&gt;
