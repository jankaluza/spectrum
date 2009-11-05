# -*- coding: utf-8 -*-
import os,  os.path, sys
try:
	from twisted.web import microdom as dom
	from twisted.web.domhelpers import *
	from twisted.internet import defer, utils
	from twisted.enterprise import adbapi, util as dbutil
	from twisted.words.protocols.jabber import jid
	from twisted.enterprise import util
except:
	print "python-twisted is not installed"

s = util.safe
from twisted.internet import reactor

def error(result):
	print 'SQL ERROR ' + unicode(result)

def start():
	if len(sys.argv) != 6 and len(sys.argv) != 7 and len(sys.argv) != 3 and len(sys.argv) != 4:
		print "Usage for MySQL: " + sys.argv[0] + " transport_path database username password host [mysql_prefix]"
		print "Usage for SQLite: " + sys.argv[0] + " transport_path database [prefix]"
		reactor.stop()
		return
	maindir = sys.argv[1]
	
	prefix = ""
	if len(sys.argv) == 7:
		prefix = sys.argv[6]
	elif len(sys.argv) == 4:
		prefix = sys.argv[3]
	dict = {}
	if len(sys.argv) == 3 or len(sys.argv) == 4:
		if not os.path.exists(sys.argv[2]):
			print "Run Spectrum to create this DB file first and then run this script again with the DB file created by Spectrum."
			reactor.stop()
			return
		db = adbapi.ConnectionPool('sqlite3', sys.argv[2])
	else:
		db = adbapi.ConnectionPool('MySQLdb', db = sys.argv[2], user = sys.argv[3], passwd = sys.argv[4], host = sys.argv[5], cp_min=1, cp_max=1)
	c = 0
	for dr in os.listdir(maindir):
		if os.path.isdir(maindir+dr):
			for f in os.listdir(maindir+dr):
				if os.path.isfile(maindir+dr+'/'+f) and not f in ['pubsub', 'avatars']:
					fp = open(maindir+dr+'/'+f, 'r')
					c += 1
					buff = fp.read()
					fp.close()
					try:
						x = dom.parseXMLString(buff)
					except:
						print "Can't parse", maindir+dr+'/'+dr2+'/'+f, ". Skipping..."
						continue
					jid = f.replace('%','@')[:-4]
					p = x.getElementsByTagName('password')
					if len(p) != 1:
						print "No password for jid", jid, ". Skipping..."
						continue
					password =  gatherTextNodes(p[0])
					u = x.getElementsByTagName('username')
					uin = gatherTextNodes(u[0])
					print "Adding", jid
					
					def done2(res, x2):
						user_id = int(res[0][0])
						print "user_id =", user_id
						print "Adding buddies of", jid
						items = x2.getElementsByTagName('item')
						for j in items:
							if len(sys.argv) == 3 or len(sys.argv) == 4:
								db.runQuery('insert or ignore into ' + prefix + 'buddies (uin, user_id, nickname, groups, subscription) values ("%s", "%s", "%s","Buddies", "both")'%(s(j.getAttribute('jid')), s(str(user_id)), s(j.getAttribute('jid')))).addErrback(error)
							else:
								db.runQuery('insert ignore into ' + prefix + 'buddies (uin, user_id, nickname, groups, subscription) values ("%s", "%s", "%s","Buddies", "both")'%(s(j.getAttribute('jid')), s(str(user_id)), s(j.getAttribute('jid')))).addErrback(error)
						print "done!"

					if len(sys.argv) == 3 or len(sys.argv) == 4:
						db.runQuery('insert or ignore into ' + prefix + 'users (jid, uin, password, language) values ("%s", "%s", "%s", "en")'%(s(jid), s(uin),s(password))).addErrback(error)
					else:
						db.runQuery('insert ignore into ' + prefix + 'users (jid, uin, password, language) values ("%s", "%s", "%s", "en")'%(s(jid), s(uin),s(password))).addErrback(error)
					db.runQuery('select id from users WHERE jid = "'+jid+'";').addCallback(done2, x).addErrback(error)
	#print "Everything done!"
	#reactor.stop()

reactor.callWhenRunning(start)
reactor.run()
