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

def start():
	if len(sys.argv) != 6 and len(sys.argv) != 7:
		print sys.argv[0] + " pyicqt_directory database username password host [mysql_prefix]"
		reactor.stop()
		return
	maindir = sys.argv[1]
	
	prefix = ""
	if len(sys.argv) == 7:
		prefix = sys.argv[6]
	dict = {}
	db = adbapi.ConnectionPool('MySQLdb', db = sys.argv[2], user = sys.argv[3], passwd = sys.argv[4], host = sys.argv[5], cp_min=1, cp_max=1)
	c = 0
	for dr in os.listdir(maindir):
		if os.path.isdir(maindir+dr):
			for dr2 in os.listdir(maindir+dr):
				for f in os.listdir(maindir+dr+'/'+dr2):
					if os.path.isfile(maindir+dr+'/'+dr2+'/'+f):
						fp = open(maindir+dr+'/'+dr2+'/'+f, 'r')
						c += 1
						print c
						buff = fp.read()
						fp.close()
						try:
							x = dom.parseXMLString(buff)
						except:
							continue
						jid = f.replace('%','@')[:-4]
						p = x.getElementsByTagName('password')
						if len(p) != 1:
							continue
						password =  gatherTextNodes(p[0])
						u = x.getElementsByTagName('username')
						uin = gatherTextNodes(u[0])
						print jid
						
						def done2(res):
							user_id = int(res[0][0])
							items = x.getElementsByTagName('item')
							for j in items:
								db.runQuery('insert ignore into ' + prefix + 'buddies (uin, user_id, nickname, groups, subscription) values ("%s", "%s", "%s","Buddies", "both")'%(s(j.getAttribute('jid')), s(str(user_id)), s(j.getAttribute('jid')))).addCallback(done)
						
						def done(res):
							print '..done'
							db.runQuery('select @@IDENTITY from'+ prefix + 'users ;' ).addCallback(done2)
						
						db.runQuery('insert ignore into ' + prefix + 'users (jid, uin, password, lang) values ("%s", "%s", "%s", "en")'%(s(jid), s(uin),s(password))).addCallback(done)

reactor.callWhenRunning(start)
reactor.run()