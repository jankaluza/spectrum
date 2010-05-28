import sys, socket

try:
	from spectrum import spectrum
except ImportError:
	import spectrum

class spectrum_group:
	"""This class represents multiple spectrum instances"""

	def __init__( self, options, params, configs ):
		self.options = options
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

	def simple_action( self, action ):
		ret = 0
		for instance in self.instances:
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

	def message_all( self ):
		self.simple_action( 'message_all' )

	def set_vip_status( self ):
		self.simple_action( 'set_vip_status' )

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
