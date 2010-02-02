#!/usr/bin/python

import os, sys, xmpp, socket
from spectrum import *

cfg = sys.argv[1]

print( "Read config file" )
config = spectrumconfigparser.SpectrumConfigParser()
config.read( cfg )
interface = config.get( 'service', 'config_interface' )

if os.path.exists( interface ):
	# this is /var/run/spectrum/foobar.socket
	print( "Connecting via local unix socket to " + interface )
	type = socket.AF_UNIX
	address = interface
elif interface.startswith( '[' ):
	# this should be [2001:...:ffff]:80
	print( "IPv6 not supported" )
	type = socket.AF_INET6
	sys.exit()
elif ':' in interface:
	# this is 127.0.0.1:80
	print( "Connecting via IPv4 to " + interface )
	type = socket.AF_INET
	hostname, port = interface.split( ':' )
	address = (hostname, int(port))
	
jid = config.get( 'service', 'jid' )
ns = 'http://jabber.org/protocol/stats'

pkg = xmpp.Iq( typ='get', queryNS=ns, to=jid )

print( "Connecting to " + interface )
s = socket.socket( type, socket.SOCK_STREAM )
s.connect( address )

print( "Send data: " )
print( pkg )
s.send( str( pkg ) )
print()

print( "Receiving data..." )
data = s.recv( 4096 ).strip( "\n " )

print( "This should be the same package that would be send back over a normal xmpp connection:" )
print( data )
