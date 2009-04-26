CREATE TABLE `users` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `jid` varchar(255) collate utf8_bin NOT NULL,
  `uin` varchar(255) collate utf8_bin NOT NULL,
  `password` varchar(255) collate utf8_bin NOT NULL,
  `group` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `jid` (`jid`),
  KEY `uin` (`uin`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

CREATE TABLE `rosters` (
  `id` bigint(20) unsigned NOT NULL auto_increment,
  `jid` varchar(255) collate utf8_bin NOT NULL,
  `uin` varchar(255) collate utf8_bin NOT NULL,
  `subscription` varchar(10) collate utf8_bin NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `jid` (`jid`),
  KEY `uin` (`uin`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

CREATE TABLE `settings` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `jid` varchar(255) NOT NULL,
  `var` varchar(255) NOT NULL,
  `value` varchar(255) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_bin;
