---
layout: default
title: Configuring new Spectrum instances
---

With spectrum, each protocol implementation runs as its own instance. So if you have
an ICQ, an MSN and a QQ transport, spectrum will run three times, each time with a different
config file

###Configuration file
The configuration files are located in /etc/spectrum. You can find an example there.
The file itself is well documented, so you simply need to fill out the details.

###Setting up the database
If you use SQLite as a database backend, you do not need to do anything to set
up the database other than making sure the path of the db-file you specified in
the configuration file exists.

If you use MySQL, there are some additional steps needed:

	mysql -uroot -p -e "create database my_database;
	 GRANT SELECT, UPDATE, INSERT, DELETE FROM ON my_database.* TO 'my_user'@'my_host' IDENTIFIED BY 'db_password';
	 flush privileges;"
	mysql -uroot -p my_database < schema_file

If you [built from source](building-from-source-code.html), you can find the
schema file in schemas/mysql_schema, if you use the [Debian/Ubuntu
packages](debian-ubuntu-installation.html), you can find it in
/usr/share/spectrum/schemas/mysql_schema.sql.

###Starting new instance
If this is the first instance of spectrum, you can simply use the initscript to
start it:

	/etc/init.d/spectrum start

If you already have some instances running, you might want to use spectrumctl
instead:

	spectrumctl -c /etc/spectrum/new_config.cfg start

In any case, you will also want to try "spectrumctl list".
