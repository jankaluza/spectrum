import socket, xmpp

class config_interface:
	def __init__( self, instance ):
		self.instance = instance
		self.path = instance.config.get( 'service', 'config_interface' )

	def send( self, data ):
		"""
		Send an XMPP stanza, automatically adding a from attribute.
		
		Returns the raw data received in response.
		"""
		s = socket.socket( socket.AF_UNIX )
		s.connect( self.path )
		s.send( str(data) )
		response = s.recv( 10240 )
		s.close()
		return response

	def send_stanza( self, stanza ):
		stanza.setFrom( 'spectrumctl@localhost' )
		stanza.setTo( self.instance.get_jid() )
		return xmpp.simplexml.XML2Node( self.send( stanza ) )

	def send_iq( self, iq ):
		return xmpp.protocol.Iq( node=self.send_stanza( iq ) )

	def command( self, children, node='transport_admin' ):
		"""
		Send an adhoc command, adding the given children to a 
		dynamically created command node.

		Returns the data as an Iq object!
		"""
		cmd_attrs = { 'node': node,
			'sessionid': 'WHATEVER',
			'xmlns': xmpp.protocol.NS_COMMANDS }
		cmd = xmpp.simplexml.Node( tag='command', attrs=cmd_attrs )
		for child in children:
			cmd.addChild( node=child )
		
		iq = xmpp.Iq( typ='set', xmlns=None )
		iq.addChild( node=cmd )
		return self.send_iq( iq )

	def query( self, children, ns, typ='get' ):
		iq = xmpp.Iq( typ=typ, queryNS=ns, payload=children )

		return self.send_iq( iq )
