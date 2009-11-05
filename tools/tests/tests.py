#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, os
from twisted.internet import reactor
from twisted.names.srvconnect import SRVConnector
from twisted.words.xish import domish
from twisted.words.protocols.jabber import xmlstream, client, jid
from twisted.words.protocols.jabber.client import IQ
from subprocess import *
import time, signal

if len(sys.argv) != 5:
	print "usage: " + sys.argv[0] + " <transport JID> <test1@localhost/test> <test2@localhost/test> <test3@localhost/test>"
	exit(0)
transport = sys.argv[1]

client_jid1 = jid.JID(sys.argv[2])
secret1 = "test"

client_jid2 = jid.JID(sys.argv[3])
secret2 = "test"

client_jid3 = jid.JID(sys.argv[4])
secret3 = "test"

print "Initializing"

os.system("rm -rf tests")
os.system("rm test.db")
os.system("rm spectrum.log")
pid = Popen(["../../spectrum", "-n", "spectrum.cfg", "-l", "spectrum.log"]).pid
time.sleep(5)

class XMPPClientConnector(SRVConnector):
	def __init__(self, reactor, domain, factory):
		SRVConnector.__init__(self, reactor, 'xmpp-client', domain, factory)


	def pickServer(self):
		#host, port = SRVConnector.pickServer(self)

		if not self.servers and not self.orderedServers:
			# no SRV record, fall back..
			port = 5222

		return self.servers[0], self.servers[1]

class AbstractTest(object):
	def __init__(self, client_jid, secret, cb, i, timer = 2):
		self.cb = cb
		self.i = i + 1
		self.jid = client_jid
		self.password = secret
		self.logs = ""
		self.f = client.basicClientFactory(client_jid, secret)
		self.f.addBootstrap(xmlstream.STREAM_CONNECTED_EVENT, self.connected)
		self.f.addBootstrap(xmlstream.STREAM_END_EVENT, self.disconnected)
		self.f.addBootstrap(xmlstream.STREAM_AUTHD_EVENT, self.authenticated)
		connector = XMPPClientConnector(reactor, client_jid.host, self.f)
		connector.servers = ["localhost", 5222]
		connector.orderedServers = ["localhost", 5222]
		connector.connect()
		self.t = reactor.callLater(timer, self.failed, True)
		self._f = False
		self.running = True

	def rawDataIn(self, buf):
		self.logs += "RECV: %s\n" % unicode(buf, 'utf-8').encode('ascii', 'replace')
		#print "RECV: %s" % unicode(buf, 'utf-8').encode('ascii', 'replace')

	def rawDataOut(self, buf):
		self.logs += "SEND: %s\n" % unicode(buf, 'utf-8').encode('ascii', 'replace')
		#print "SEND: %s" % unicode(buf, 'utf-8').encode('ascii', 'replace')

	def connected(self, xs):
		self.xmlstream = xs

		# Log all traffic
		xs.rawDataInFn = self.rawDataIn
		xs.rawDataOutFn = self.rawDataOut

	def ok(self):
		if self.running:
			self.running = False
			print "OK"
			self.t.cancel()
			self.xmlstream.sendFooter()
			self.f.stopTrying()
			if len(self.cb) != 0:
				self.cb[0](self.jid, self.password, self.cb[1:], self.i)
			#else:
				#os.system("kill " + str(pid))

	def failed(self, timer = False):
		if self._f == False:
			print "FAILED"
			print self.logs
			self._f = True
			self.xmlstream.sendFooter()
			self.f.stopTrying()
			os.kill(pid, signal.SIGTERM)
			reactor.stop()

	def disconnected(self, xs):
		pass

	def authenticated(self, xs):
		pass

class RegisterAccount(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		AbstractTest.__init__(self, client_jid, secret, cb, i)
		self.f.addBootstrap(client.BasicAuthenticator.INVALID_USER_EVENT, self.invalid_user)
		self.f.addBootstrap(client.BasicAuthenticator.AUTH_FAILED_EVENT, self.invalid_user)
		print str(i) + ". Registering account " + client_jid.userhost() + ": ",

	def invalid_user(self, xs):
		iq = IQ(self.xmlstream, "set")
		iq.addElement(("jabber:iq:register", "query"))
		iq.query.addElement("username", content = self.jid.user)
		iq.query.addElement("password", content = self.password)

		iq.addCallback(self._registerResultEvent)

		iq.send()

	def _registerResultEvent(self, iq):
		if iq["type"] == "result":
			# Registration succeeded -- go ahead and auth
			self.ok()
		else:
			# Registration failed
			self.failed()

	def authenticated(self, xs):
		presence = domish.Element((None, 'presence'))
		xs.send(presence)

		self.ok()

class UnregisterAccount(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		AbstractTest.__init__(self, client_jid, secret, cb, i)
		self.f.addBootstrap(client.BasicAuthenticator.INVALID_USER_EVENT, self.invalid_user)
		self.f.addBootstrap(client.BasicAuthenticator.AUTH_FAILED_EVENT, self.invalid_user)
		if self.i>0:
			print str(i) + ". Unregistering account " + client_jid.userhost() + ": ",

	def invalid_user(self, xs):
		self.ok()

	def authenticated(self, xs):

		presence = domish.Element((None, 'presence'))
		xs.send(presence)

		iq = IQ(self.xmlstream, "set")
		iq.addElement(("jabber:iq:register", "query"))
		iq.query.addElement("remove")

		iq.addCallback(self._result)

		iq.send()
	
	def _result(self, xs):
		self.ok()

class RegisterTransport(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		AbstractTest.__init__(self, client_jid, secret, cb, i, 2)
		self.stages=[]
		print str(i) + ". Registering transport account " + client_jid.userhost() + ": ",

	def authenticated(self, xs):
		self.xmlstream.addObserver("/presence[@type='subscribe']", self.onSubscribe, 1)
		self.xmlstream.addObserver("/presence[@type='subscribed']", self.onSubscribed, 1)

		presence = domish.Element((None, 'presence'))
		xs.send(presence)

		iq = IQ(self.xmlstream, "get")
		iq['to'] = transport
		iq.addElement(("jabber:iq:register", "query"))

		iq.addCallback(self._result)

		iq.send()
	
	def onSubscribe(self, el):
		presence = domish.Element((None, 'presence'))
		presence['to'] = el['from']
		presence['type'] = "subscribed"
		self.xmlstream.send(presence)
		self.stages.append("subscribed")
		if len(self.stages) == 2:
			self.ok()
		
	def onSubscribed(self, el):
		self.stages.append("subscribed")
		if len(self.stages) == 2:
			self.ok()
	
	def _result(self, iq):
		#<iq from='icq.localhost' to='t3@localhost/test' id='H_24' type='result'><query xmlns='jabber:iq:register'><instructions>Enter your Jabber ID and password:</instructions><username/><password/><x xmlns='jabber:x:data' type='form'><title>Registration</title><instructions>Enter your Jabber ID and password:</instructions><field type='hidden' var='FORM_TYPE'><value>jabber:iq:register</value></field><field type='text-single' var='username' label='Network username'><required/></field><field type='text-private' var='password' label='Password'><required/></field><field type='list-single' var='language' label='Language'><value>en</value><option label='Cesky'><value>cs</value></option><option label='English'><value>en</value></option></field></x></query></iq>
		# TODO: more tests
		if iq['type'] != 'result':
			self.failed()
		query = iq.firstChildElement()
		if query.name != "query":
			self.failed()
		
		iq = IQ(self.xmlstream, "set")
		iq['to'] = transport
		iq.addElement(("jabber:iq:register", "query"))
		iq.query.addElement("username", content = client_jid1.userhost())
		iq.query.addElement("password", content = secret1)

		iq.addCallback(self._result2)

		iq.send()
	
	def _result2(self, iq):
		if iq['type'] != 'result':
			self.failed()
		query = iq.firstChildElement()
		if query.name != "query":
			self.failed()

		iq = IQ(self.xmlstream, "get")
		iq['to'] = transport
		iq.addElement(("jabber:iq:register", "query"))

		iq.addCallback(self._result3)

		iq.send()
	
	def _result3(self, iq):
		if iq['type'] != 'result':
			self.failed()
		query = iq.firstChildElement()
		if query.name != "query":
			self.failed()
		names = []
		for el in query.children:
			if el.name == "username" and unicode(el) != client_jid1.userhost():
				self.failed()
				break
			names.append(el.name)
		if not "registered" in names:
			self.failed()
		self.stages.append("registered")
		if len(self.stages) == 2:
			self.ok()

class LoginTransport(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		AbstractTest.__init__(self, client_jid, secret, cb, i, timer = 5)
		print str(i) + ". Logging to transport " + client_jid.userhost() + ": ",

	def authenticated(self, xs):
		self.xmlstream.addObserver("/iq[@type='get'][@id]/query[@xmlns='http://jabber.org/protocol/disco#info']", self._discoInfo, 1)
		self.xmlstream.addObserver("/presence", self._presence, 1)

		presence = domish.Element((None, 'presence'))
		xs.send(presence)
	
	def _presence(self, el):
		if el['from'] == client_jid2.user + "%" + client_jid2.host + "@" + transport + "/bot":
			self.ok()
	
	def _discoInfo(self, el):
		iq = domish.Element((None,'iq'))
		iq['to'] = el['from']
		iq['type'] = 'result'
		iq['id'] = el['id']
		q = iq.addElement('query', 'http://jabber.org/protocol/disco#info')

		id = q.addElement('identity')
		id['category'] = "client"
		id['name'] = "test_client"
		id['type'] = "client"
		
		features = ["http://jabber.org/protocol/rosterx","http://jabber.org/protocol/xhtml-im","http://jabber.org/protocol/si/profile/file-transfer","http://jabber.org/protocol/chatstates"]
		
		for feature in features:
			f = q.addElement('feature')
			f['var'] = feature

		self.xmlstream.send(iq)

class ChangeStatusTransport(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		AbstractTest.__init__(self, client_jid, secret, cb, i, timer = 6)
		print str(i) + ". Change status " + client_jid.userhost() + ": ",
		self.stage = -1

	def authenticated(self, xs):
		self.xmlstream.addObserver("/iq[@type='get'][@id]/query[@xmlns='http://jabber.org/protocol/disco#info']", self._discoInfo, 1)
		self.xmlstream.addObserver("/presence", self._presence, 1)

		presence = domish.Element((None, 'presence'))
		xs.send(presence)
	
	def _presence(self, el, called = False):
		if el['from'] == client_jid2.user + "%" + client_jid2.host + "@" + transport + "/bot":
			couple = None
			if self.stage == -1:
				reactor.callLater(2, self._presence, el, True)
				self.stage = 0
				return
			elif self.stage == 0:
				if called:
					couple = ("stage1", "xa")
				else:
					return
			elif self.stage == 1:
				for child in el.children:
					if child.name == "status" and unicode(child) != "stage1":
						self.failed()
						return
					elif child.name == "show" and unicode(child) != "xa":
						self.failed()
						return
				couple = ("stage2", "away")
			elif self.stage == 2:
				for child in el.children:
					if child.name == "status" and unicode(child) != "stage2":
						self.failed()
						return
					elif child.name == "show" and unicode(child) != "away":
						self.failed()
						return
			if couple:
				presence = domish.Element((None, 'presence'))
				presence.addElement('status', content = couple[0])
				presence.addElement('show', content = couple[1])
				self.xmlstream.send(presence)
			else:
				self.ok()
			self.stage += 1
	
	def _discoInfo(self, el):
		iq = domish.Element((None,'iq'))
		iq['to'] = el['from']
		iq['type'] = 'result'
		iq['id'] = el['id']
		q = iq.addElement('query', 'http://jabber.org/protocol/disco#info')

		id = q.addElement('identity')
		id['category'] = "client"
		id['name'] = "test_client"
		id['type'] = "client"
		
		features = ["http://jabber.org/protocol/rosterx","http://jabber.org/protocol/xhtml-im","http://jabber.org/protocol/si/profile/file-transfer","http://jabber.org/protocol/chatstates"]
		
		for feature in features:
			f = q.addElement('feature')
			f['var'] = feature

		self.xmlstream.send(iq)

class SendMessageTransport(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		AbstractTest.__init__(self, client_jid, secret, cb, i, timer = 5)
		print str(i) + ". Send message through transport " + client_jid.userhost() + ": ",
		
	def authenticated(self, xs):
		self.xmlstream.addObserver("/iq[@type='get'][@id]/query[@xmlns='http://jabber.org/protocol/disco#info']", self._discoInfo, 1)
		self.xmlstream.addObserver("/message", self._message, 1)
		self.xmlstream.addObserver("/presence", self._presence, 1)

		presence = domish.Element((None, 'presence'))
		xs.send(presence)

	def _presence(self, el):
		if el['from'] == client_jid2.user + "%" + client_jid2.host + "@" + transport + "/bot":
			message = domish.Element((None,'message'))
			message['to'] = client_jid2.user + "%" + client_jid2.host + "@" + transport + "/bot"
			message['from'] = self.jid.full()
			message.addElement('body', content = "ping")
			message['type'] = "chat"
			self.xmlstream.send(message)

	def _message(self, el):
		for child in el.children:
			if child.name == "body" and unicode(child) != "pong":
				self.failed()
		self.ok()

	def _discoInfo(self, el):
		iq = domish.Element((None,'iq'))
		iq['to'] = el['from']
		iq['type'] = 'result'
		iq['id'] = el['id']
		q = iq.addElement('query', 'http://jabber.org/protocol/disco#info')

		id = q.addElement('identity')
		id['category'] = "client"
		id['name'] = "test_client"
		id['type'] = "client"
		
		features = ["http://jabber.org/protocol/rosterx","http://jabber.org/protocol/xhtml-im","http://jabber.org/protocol/si/profile/file-transfer","http://jabber.org/protocol/chatstates"]
		
		for feature in features:
			f = q.addElement('feature')
			f['var'] = feature

		self.xmlstream.send(iq)

class Done(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		print "All tests sucessfully done! congrats!"
		os.kill(pid, signal.SIGTERM)
		reactor.stop()

class AddUser(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		AbstractTest.__init__(self, client_jid, secret, cb, i, 800)
		print str(i) + ". Adding test user to roster " + client_jid.userhost() + ": ",
		self.tests = [UnregisterAccount,RegisterAccount,RegisterTransport,LoginTransport,SendMessageTransport,ChangeStatusTransport,Done]
		self.stages = []

	def authenticated(self, xs):
		self.xmlstream.addObserver("/presence[@type='subscribe']", self.onSubscribe, 1)
		self.xmlstream.addObserver("/presence[@type='subscribed']", self.onSubscribed, 1)

		presence = domish.Element((None, 'presence'))
		xs.send(presence)
		
		presence = domish.Element((None, 'presence'))
		presence['to'] = client_jid2.userhost()
		presence['type'] = "subscribe"
		xs.send(presence)
	
	def onSubscribe(self, el):
		presence = domish.Element((None, 'presence'))
		presence['to'] = el['from']
		presence['type'] = "subscribed"
		self.xmlstream.send(presence)
		self.stages.append("subscribe")
		if "subscribed" in self.stages:
			self.ok()
			tests = self.tests
			tests[0](client_jid3, secret3, tests[1:], self.i)
		
	def onSubscribed(self, el):
		presence = domish.Element((None, 'presence'))
		presence['to'] = el['from']
		presence['type'] = "subscribe"
		self.xmlstream.send(presence)
		self.stages.append("subscribed")
		if "subscribe" in self.stages:
			self.ok()
			tests = self.tests
			tests[0](client_jid3, secret3, tests[1:], self.i)

class PassiveClient(AbstractTest):
	def __init__(self, client_jid, secret, cb, i):
		AbstractTest.__init__(self, client_jid, secret, cb, i, timer = 10000)
		print "PASSIVE CLIENT ACTIVE"

	def authenticated(self, xs):
		self.xmlstream.addObserver("/presence[@type='subscribe']", self.onSubscribe, 1)
		self.xmlstream.addObserver("/presence[@type='subscribed']", self.onSubscribed, 1)
		self.xmlstream.addObserver("/message", self._message, 1)
		self.xmlstream.addObserver("/presence", self._presence, 1)

		presence = domish.Element((None, 'presence'))
		xs.send(presence)
		tests = [UnregisterAccount,RegisterAccount,AddUser]
		tests[0](client_jid1, secret1, tests[1:], self.i)

	def _presence(self, el):
		if el['from'] == self.jid.full():
			return
		presence = domish.Element((None, 'presence'))
		for child in el.children:
			if child.name == "status":
				presence.addElement('status', content = unicode(child))
			elif child.name == "show":
				presence.addElement('show', content = unicode(child))
		self.xmlstream.send(presence)

	def _message(self, el):
		message = domish.Element((None,'message'))
		message['to'] = el['from']
		message['from'] = self.jid.full()
		message.addElement('body', content = "pong")
		message['type'] = "chat"
		self.xmlstream.send(message)

	def onSubscribe(self, el):
		presence = domish.Element((None, 'presence'))
		presence['to'] = el['from']
		presence['type'] = "subscribed"
		self.xmlstream.send(presence)
		presence = domish.Element((None, 'presence'))
		presence['to'] = el['from']
		presence['type'] = "subscribe"
		self.xmlstream.send(presence)
		
	def onSubscribed(self, el):
		presence = domish.Element((None, 'presence'))
		presence['to'] = el['from']
		presence['type'] = "subscribe"
		self.xmlstream.send(presence)

tests_passive_client = [UnregisterAccount, RegisterAccount, PassiveClient, UnregisterAccount]
tests_passive_client[0](client_jid2, secret2, tests_passive_client[1:], 1)

reactor.run()
