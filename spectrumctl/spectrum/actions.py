import socket

def list( options, instances ):
	lines = [ ('PID', 'PROTOCOL', 'HOSTNAME', 'STATUS' ) ]

	for instance in instances:
		lines.append( instance.list() )

	if options.machine_readable:
		for line in lines[1:]:
			print( ','.join( line ) )
		return 0

	widths = [0,0,0,0]
	for item in lines:
		for i in range(0,4):
			val = str( item[i] )
			if len( val ) > widths[i]:
				widths[i] = len( val )

	for item in lines:
		for i in range(0,4):
			val = str( item[i] )
			val += ' ' * (widths[i]-len(val)) + '  '
			print val,
		print
	return 0

def stats( options, instances ):
	try:
		import xmpp
	except ImportError:
		print( "Error: xmpppy library not found." )
		return 1

	output = []

	for instance in instances:
		s = None
		jid = instance.config.get( 'service', 'jid' )
		o = [ jid + ':' ]

		# form the package:
		ns = 'http://jabber.org/protocol/stats'
		pkg = xmpp.Iq( typ='get', queryNS=ns, to=jid )
		for name in [ 'uptime', 'users/registered', 'users/online', 
					'legacy-network-users/online',
					'legacy-network-users/registered',
					'messages/in', 'messages/out',
					'memory-usage' ]:
			child = xmpp.simplexml.Node( 'stat', {'name': name} )
			pkg.kids[0].addChild( node=child )

		config_interface = instance.config.get( 'service', 'config_interface' )
		if config_interface == '':
			o.append( "Error: %s: No config_interface defined." %(instance.config_path) )
			output.append( "\n".join( o ) )
			continue 

		try:
			s = socket.socket( socket.AF_UNIX )
			s.connect( config_interface )
		except socket.error, msg:
			o.append( config_interface + ': ' + msg.strerror )
			output.append( "\n".join( o ) )
			continue

		s.send( str(pkg) )
		data = s.recv( 10240 )
		s.close()
		node = xmpp.simplexml.XML2Node( data )
		node = xmpp.protocol.Iq( node=node )

		for stat in node.getQueryChildren():
			value = stat.getAttr( 'value' )
			unit = stat.getAttr( 'units' )
			name = stat.getAttr( 'name' )
			o.append( name + ': ' + value + ' ' + unit )

		output.append( "\n".join( o ) )
	
	print "\n\n".join( output )
	return 0
