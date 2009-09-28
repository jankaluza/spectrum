-- 
-- Created by SQL::Translator::Producer::SQLite
-- Created on Mon Sep 28 17:10:15 2009
-- 


BEGIN TRANSACTION;

--
-- Table: buddies
--
CREATE TABLE buddies (
  id INTEGER PRIMARY KEY NOT NULL,
  user_id int(10) NOT NULL,
  uin varchar(255) NOT NULL,
  subscription enum(4) NOT NULL,
  nickname varchar(255) NOT NULL,
  groups varchar(255) NOT NULL
);

CREATE UNIQUE INDEX user_id ON buddies (user_id, uin);

--
-- Table: buddies_settings
--
CREATE TABLE buddies_settings (
  user_id int(10) NOT NULL,
  buddy_id int(10) NOT NULL,
  var varchar(50) NOT NULL,
  type smallint(4) NOT NULL,
  value varchar(255) NOT NULL,
  PRIMARY KEY (buddy_id, var)
);

CREATE INDEX user_id02 ON buddies_settings (user_id);

--
-- Table: users
--
CREATE TABLE users (
  id INTEGER PRIMARY KEY NOT NULL,
  jid varchar(255) NOT NULL,
  uin varchar(4095) NOT NULL,
  password varchar(255) NOT NULL,
  language varchar(25) NOT NULL,
  encoding varchar(50) NOT NULL DEFAULT 'utf8',
  last_login datetime,
  vip tinyint(1) NOT NULL DEFAULT '0'
);

CREATE UNIQUE INDEX jid ON users (jid);

--
-- Table: users_settings
--
CREATE TABLE users_settings (
  user_id int(10) NOT NULL,
  var varchar(50) NOT NULL,
  type smallint(4) NOT NULL,
  value varchar(255) NOT NULL,
  PRIMARY KEY (user_id, var)
);

CREATE INDEX user_id03 ON users_settings (user_id);

COMMIT;
