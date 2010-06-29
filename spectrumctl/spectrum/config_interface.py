"""
Interface for accessing the local socket opened by spectrum instances. The class
is designed to ease access to it and abstract common behaviour.
"""

import socket
from xmpp.simplexml import Node, XML2Node
from xmpp.protocol import NS_DATA, NS_COMMANDS, Iq

class config_interface:
	"""
	An instance of this class represents the config_interface opened by a
	spectrum instance.
	"""


	def __init__( self, instance ):
		"""
		Constructor.

		@param instance: A spectrum instance
		@type  instance: L{spectrum<spectrum.spectrum>}
		"""
		self.instance = instance
		self.path = instance.config.get( 'service', 'config_interface' )

	def send( self, data ):
		"""
		Send an XMPP stanza. Use this method if you don't want to
		manipulate the data in any way.
		
		@param data: The XML node to send
		@type  data:
			U{xmpp.simplexml.Node<http://xmpppy.sourceforge.net/apidocs/index.html>}
		@todo: This should actually receive a string (since it also
			returns a raw string)
		@return: The data received in response
		@rtype: str
		"""
		try:
			s = socket.socket( socket.AF_UNIX )
			s.connect( self.path )
			s.send( str(data) )
			response = s.recv( 10240 )
			s.close()
			return response
		except socket.error, e:
			raise RuntimeError( "Error accessing socket: %s."%(e.args[1]) )

	def send_stanza( self, stanza ):
		"""
		Send an xmpp stanza to the spectrum instance. This method
		automatically adds the "from" and "to" attributes.

		@param data: The XML node to send
		@type  data:
			U{xmpp.simplexml.Node<http://xmpppy.sourceforge.net/apidocs/index.html>}
		@return: The data received in response
		@rtype: 
			U{xmpp.simplexml.Node<http://xmpppy.sourceforge.net/apidocs/index.html>}
		"""
		stanza.setFrom( 'spectrumctl@localhost' )
		stanza.setTo( self.instance.get_jid() )
		return XML2Node( self.send( stanza ) )

	def send_iq( self, iq ):
		"""
		Convenience shortcut that wraps I{send_stanza} and casts the
		response to an
		U{IQ
		node<http://xmpppy.sourceforge.net/apidocs/xmpp.protocol.Iq-class.html>}.

		Note that no error-checking of any kind is done, this method
		just assumes that the parameter you pass represents an IQ node.
		This will also fail if you use this method to send a response
		(type "error" or "result") since there is no response to be
		cast.

		@param iq: The IQ node to send
		@type  iq:
			U{xmpp.protocol.Iq<http://xmpppy.sourceforge.net/apidocs/xmpp.protocol.Iq-class.html>}
		@return: The response cast to an IQ node.
		@rtype:
			U{xmpp.protocol.Iq<http://xmpppy.sourceforge.net/apidocs/xmpp.protocol.Iq-class.html>}
		"""
		return Iq( node=self.send_stanza( iq ) )

	def command( self, adhoc_state, vars ):
		"""
		Send an ad-hoc command.
		
		The parameter vars is a list of tupels each defining name, type,
		and value. So if you pass the following tuple as vars::
			
			[("var-name", "text-single", "example")]
		
		... a field with the following XML will be send::
			
			<field var="var-name" type="text-single">
				<value>example</value>
			</field>

		The adhoc_state parameter is special to spectrumctl and
		identifies the command being executed.

		@param adhoc_state: The command identifier.
		@type  adhoc_state: str
		@param vars: A list of tuples defining the fields to send. See
			above for more information.

		@todo: return str instead of None with the payload of the note
			stanza in case of a non-error note.

		@raises RuntimeError: in case the command fails
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
			'sessionid': 'WHATEVER', 'xmlns': NS_COMMANDS }
		cmd = Node( tag='command', attrs=cmd_attrs, payload=[x] )

		# build IQ node
		iq = Iq( typ='set', xmlns=None )
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
		"""
		Shortcut to send an IQ query. This method uses I{send_iq} to
		automatically set from/to attributes and cast the response to an
		IQ node.

		@param children: A list of 
			U{nodes<http://xmpppy.sourceforge.net/apidocs/index.html>}
			that should be added as children. 
		@type  children: list
		@param ns: The namespace of the node.
		@type  ns: str
		@param typ: The type of the node to send.
		@type  typ: str
		@return: the IQ stanza returned.
		@rtype:
			U{xmpp.protocol.Iq<http://xmpppy.sourceforge.net/apidocs/xmpp.protocol.Iq-class.html>}
		"""
		iq = Iq( typ=typ, queryNS=ns, payload=children )

		return self.send_iq( iq )
