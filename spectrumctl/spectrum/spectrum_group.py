import sys, socket

try:
	from spectrum import spectrum
except ImportError:
	import spectrum

class spectrum_group:
	"""This class represents multiple spectrum instances"""

	def __init__( self, options, params, configs ):
		self.options = options
		self.params = params
		self.instances = []
		for config in configs:
			instance = spectrum( config, params )
			if instance.enabled():
				self.instances.append( instance )

	def log( self, msg, newline=True ):
		if not self.options.quiet:
			if newline:
				print( msg )
			else:   
				print( msg ),
#				print( msg, end='' ) #python 3

	def simple_action( self, action, title=None ):
		ret = 0
		for instance in self.instances:
			if not title:
				title = action.title()
			jid = instance.get_jid()
			self.log( "%s %s..."%(title, jid), False )
			try:
				getattr( instance, action )()
				self.log( "Ok." )
			except socket.error, e:
				if hasattr( e, 'strerror' ):
					# python2.5 does not have msg.strerror
					err = e.strerror
				else:
					err = e.message
				self.log( "Failed (config_interface: %s)"%(err) )
			except RuntimeError, e:
				self.log( "Failed (%s)"%e.args[0] )
				ret += e.args[1]

		sys.exit( ret )

	def start( self ):
		self.simple_action( 'start' )

	def stop( self ):
		self.simple_action( 'stop' )

	def restart( self ):
		self.simple_action( 'restart' )

	def reload( self ):
		self.simple_action( 'reload' )

	def stats( self ):
		output = []
		for instance in self.instances:
			iq = instance.get_stats()
			o = [ instance.get_jid() + ':' ]
			for stat in iq.getQueryChildren():
				value = stat.getAttr( 'value' )
				unit = stat.getAttr( 'units' )
				name = stat.getAttr( 'name' )
				o.append( name + ': ' + value + ' ' + unit )
			output.append( "\n".join( o ) )
		print "\n\n".join( output )
	
	def upgrade_db( self ):
		self.simple_action( 'upgrade_db', "Upgrading db for" )

	def message_all( self ):
		self.simple_action( 'message_all' )

	def set_vip_status( self ):
		if len( self.params ) != 2:
			print( "Error: set_vip_status <jid> <status>: Wrong number of arguments" )

		status_string = "Setting VIP-status for '%s' to "
		if self.params[1] == "0":
			status_string += "False:"
		elif self.params[1] == "1":
			status_string += "True:"
		else:
			print( "Error: status (third argument) must be either 0 or 1" )
			return 1
		print( status_string%(self.params[0]) )

		for instance in self.instances:
			print( instance.get_jid() + '...' ),
			cmd = instance.set_vip_status().kids[0]
			if len( cmd.kids ) == 0:
				print( "Ok." )
			else:
				note = cmd.kids[0]
				typ = note.getAttr( 'type' ).title()
				msg = note.getPayload()[0]
				print( "%s: %s" %(typ, msg) )


	def list( self ):
		lines = [ ('PID', 'PROTOCOL', 'HOSTNAME', 'STATUS' ) ]

		for instance in self.instances:
			lines.append( instance.list() )

		if self.options.machine_readable:
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
