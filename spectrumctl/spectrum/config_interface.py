import socket, xmpp
from xmpp.simplexml import Node
from xmpp.protocol import NS_DATA

class config_interface:
	def __init__( self, instance ):
		self.instance = instance
		self.path = instance.config.get( 'service', 'config_interface' )

	def send( self, data ):
		"""
		Send an XMPP stanza, automatically adding a from attribute.
		
		Returns the raw data received in response.
		"""
		try:
			s = socket.socket( socket.AF_UNIX )
			s.connect( self.path )
			print( "Send: " + str( data ) )
			s.send( str(data) )
			response = s.recv( 10240 )
			s.close()
			return response
		except socket.error, e:
			raise RuntimeError( "Error accessing socket: %s."%(e.args[1]) )

	def send_stanza( self, stanza ):
		stanza.setFrom( 'spectrumctl@localhost' )
		stanza.setTo( self.instance.get_jid() )
		return xmpp.simplexml.XML2Node( self.send( stanza ) )

	def send_iq( self, iq ):
		return xmpp.protocol.Iq( node=self.send_stanza( iq ) )

	def command( self, adhoc_state, vars ):
		"""
		Send an adhoc command, adding the given children to a 
		dynamically created command node.

		@raises: RuntimeError in case the command fails
		"""
		

		# build adhoc_state field:
		state_value = Node( tag='value', payload=[adhoc_state] )
		state = Node( tag='field', payload=[state_value],
			attrs={'type': 'hidden', 'var': 'adhoc_state'} )

		# build variable fields:
		fields = [state]
		for var in vars:
			name, typ, val = var
			
			# build payload for field:
			var_payload = []
			if typ == 'text-multi':
				for line in val.splitlines():
					var_payload.append( Node( tag='value', payload=[line] ) )
			else:
				var_payload.append( Node( tag='value', payload=[val] ) )

			# build field stanza:
			var_node =  Node( tag='field', payload=var_payload,
				attrs={'type': typ, 'var': name } )
			fields.append( var_node )

		# build x stanza
		x = Node( tag='x', payload=fields,
			attrs={ 'xmlns': NS_DATA, 'type': 'submit' } )

		# build command node
		cmd_attrs = { 'node': 'transport_admin',
			'sessionid': 'WHATEVER',
			'xmlns': xmpp.protocol.NS_COMMANDS }
		cmd = Node( tag='command', attrs=cmd_attrs, payload=[x] )

		# build IQ node
		iq = xmpp.Iq( typ='set', xmlns=None )
		iq.addChild( node=cmd )
		answer = self.send_iq( iq )
		print( "Answer: " + str(answer) )
		cmd = answer.kids[0]
		if len( cmd.kids ) == 0:
			return

		note = cmd.kids[0]
		if note.getAttr( 'type' ) == 'error':
			raise RuntimeError( note.getPayload()[0] )

	def query( self, children, ns, typ='get' ):
		iq = xmpp.Iq( typ=typ, queryNS=ns, payload=children )

		return self.send_iq( iq )
