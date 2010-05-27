import socket

def list( options, params, instances ):
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

def stats( options, params, instances ):
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
					'contacts/online',
					'contacts/total',
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
			if hasattr( msg, 'strerror' ):
				# python2.5 does not have msg.strerror
				err = msg.strerror
			else:
				err = msg.message
			o.append( config_interface + ': ' + err )
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

def send_message( config_interface, pkg ):
	"""Just a small helper function for AdHoc Command tests."""
	if config_interface == '':
		raise RuntimeError( "Error: %s: No config_interface defined." %(instance.config_path) )

	s = socket.socket( socket.AF_UNIX )
	s.connect( config_interface )
	s.send( str(pkg) )
	data = s.recv( 10240 )
	s.close()
	return data

def adhoc_test( options, params, instances ):
	try:
		import xmpp
	except ImportError:
		print( "Error: xmpppy library not found." )
		return 1

	for instance in instances:
		jid = instance.config.get( 'service', 'jid' )
		config_interface = instance.config.get( 'service', 'config_interface' )

		cmd_attrs = { 'node': 'transport_admin', 
			'sessionid': 'WHATEVER', 
			'xmlns': xmpp.protocol.NS_COMMANDS }
		cmd = xmpp.simplexml.Node( tag='command', attrs=cmd_attrs )
		x = xmpp.simplexml.Node( tag='x',
			attrs={'xmlns': xmpp.protocol.NS_DATA, 'type':'submit'} )
		
		fields = [ 
			('adhoc_state', 'ADHOC_ADMIN_USER2', 'hidden'),
			('user_jid', params[0], 'hidden' ),
			('user_vip', params[1], 'boolean' ) ]

		for field in fields:
			value = xmpp.simplexml.Node( tag='value', payload=[field[1]] )
			field = xmpp.simplexml.Node( tag='field', payload=[value],
				attrs={'type':field[2], 'var':field[0] } )
			x.addChild( node=field )

		iq = xmpp.Iq( typ='set', to=str(jid), xmlns=None )
		cmd.addChild( node=x )
		iq.addChild( node=cmd )
		print( "Sending:\n" + str(iq) )
		print( "" )
		
		try:
			data = send_message( config_interface, str(iq) )
			print( "Received:\n" + data )
		except socket.error, msg:
			if hasattr( msg, 'strerror' ):
				# python2.5 does not have msg.strerror
				print( config_interface + ': ' + msg.strerror )
			else:
				print( config_interface + ': ' + msg.message )
			continue

def message_all( options, params, instances ):
	try:
		import xmpp
	except ImportError:
		print( "Error: xmpppy library not found." )
		return 1

	for instance in instances:
		jid = instance.config.get( 'service', 'jid' )
		config_interface = instance.config.get( 'service', 'config_interface' )
		
		# first field stanza:
		c_value = xmpp.simplexml.Node( tag='value', payload=['ADHOC_ADMIN_SEND_MESSAGE'] )
		c_field = xmpp.simplexml.Node( tag='field', attrs={'type': 'hidden', 'var': 'adhoc_state'} )
		c_field.addChild( node=c_value )

		# x stanza with enclosed field:
		x = xmpp.simplexml.Node( tag='x', attrs={ 'xmlns': xmpp.protocol.NS_DATA, 'type': 'submit' } )
		x_field = xmpp.simplexml.Node( tag='field', attrs={'type':'text-multi', 'var': 'message' } )
		x_value = xmpp.simplexml.Node( tag='value', payload=['Awesome message'] )
		x_field.addChild( node=x_value )
		x.addChild( node=x_field )

		cmd_attrs = { 'node': 'transport_admin', 
			'sessionid': 'WHATEVER', 
			'xmlns': xmpp.protocol.NS_COMMANDS }
		cmd = xmpp.simplexml.Node( tag='command', attrs=cmd_attrs )
		cmd.addChild( node=c_field )
		cmd.addChild( node=x )

		iq = xmpp.Iq( typ='set', to=str(jid), xmlns=None )
		iq.addChild( node=cmd )

		print( "Sending:\n" + str(iq) )
		print( "" )
		data = send_message( config_interface, str(iq) )
		print( "Received:\n" + data )
