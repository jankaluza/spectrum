#!/usr/bin/env python2

import os, sys
from spectrum import *
from optparse import *

parser = OptionParser()
parser.add_option( '-c', '--config', metavar='FILE',
	help = 'Only act on transport configured in FILE' )
parser.add_option( '-d', '--config-dir', metavar='DIR', default='/etc/spectrum',
	help = 'Act on all transports configured in DIR (default: %default)' )
options, args = parser.parse_args()
action = args[0]

if len( args ) != 1:
	print( "Error: Please give exactly one action." )
elif action not in dir( spectrum.spectrum ):
	print( "Unknown action." )

def act( instance ):
	print( "%s %s..."%(action.title(), instance.get_jid()) ),
	# print( "%s %s..."%(action.title(), instance.get_jid()), end='' ) #python3
	exit, string = getattr( instance, action )()
	print( string )
	return exit

if options.config:
	instance = spectrum.spectrum( options.config )
	ret = act( instance )
	sys.exit( ret )
else:
	if not os.path.exists( options.config_dir ):
		print( "Error: %s: No such directory"%(options.config_dir) )
		sys.exit(1)
	ret = 0
	for file in os.listdir( options.config_dir ):
		path = '%s/%s'%(options.config_dir, file)
		instance = spectrum.spectrum(path)
		retVal = act( instance )
		if retVal != 0:
			ret = retVal

	if ret != 0:
		print( "Some actions failed." )
		sys.exit( ret )
	else:
		sys.exit( 0 )
