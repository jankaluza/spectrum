CREATE TABLE IF NOT EXISTS `buddies` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `user_id` int(10) unsigned NOT NULL,
  `uin` varchar(255) collate utf8_bin NOT NULL,
  `subscription` enum('to','from','both','ask','none') collate utf8_bin NOT NULL,
  `nickname` varchar(255) collate utf8_bin NOT NULL,
  `groups` varchar(255) collate utf8_bin NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_id` (`user_id`,`uin`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;
 
CREATE TABLE IF NOT EXISTS `buddies_settings` (
  `user_id` int(10) unsigned NOT NULL,
  `buddy_id` int(10) unsigned NOT NULL,
  `var` varchar(50) collate utf8_bin NOT NULL,
  `type` smallint(4) unsigned NOT NULL,
  `value` varchar(255) collate utf8_bin NOT NULL,
  PRIMARY KEY (`buddy_id`,`var`),
  KEY `user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;
 
CREATE TABLE IF NOT EXISTS `users` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `jid` varchar(255) collate utf8_bin NOT NULL,
  `uin` varchar(4095) collate utf8_bin NOT NULL,
  `password` varchar(255) collate utf8_bin NOT NULL,
  `language` varchar(25) collate utf8_bin NOT NULL,
  `encoding` varchar(50) collate utf8_bin NOT NULL default 'utf8',
  `last_login` datetime,
  `vip` tinyint(1) NOT NULL  default '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `jid` (`jid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

CREATE TABLE IF NOT EXISTS `users_settings` (
  `user_id` int(10) unsigned NOT NULL,
  `var` varchar(50) collate utf8_bin NOT NULL,
  `type` smallint(4) unsigned NOT NULL,
  `value` varchar(255) collate utf8_bin NOT NULL,
  PRIMARY KEY (`user_id`,`var`),
  KEY `user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;
