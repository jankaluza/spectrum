from spectrum.doc import epydoc

class arg:
	def __init__( self, name, optional=False, repeatable=False ):
		self.name = name
		self.optional = optional
		self.repeatable = repeatable

	def string( self ):
		arg_string = 'I{' + self.name + '}'
		if self.optional:
			arg_string = '[' + arg_string + ']'
		return arg_string

class action:
	def __init__( self, name, text=None, args=[] ):
		self.name = name
		self.text = text
		self.args = args

	def get_arg_string( self ):
		arg_list = [ 'B{' + self.name + '}' ]
		for arg in self.args:
			arg_list.append( arg.string() )

		return ' '.join( arg_list )

	def get_text( self, cl ):
		if self.text:
			return self.text
		else:
			func = getattr( cl, self.get_func() )
			return epydoc.get( func )
	
	def get_func( self ):
		return self.name.replace( '-', '_' )

class doc:
	def __init__( self, cl ):
		self.cl = cl

	def create_documenation( self, action ):
		raise RuntimeError( 'not implemented' )

cmds = [
	action( 'start' ),
	action( 'stop' ),
	action( 'restart' ),
	action( 'reload' ),
	action( 'stats' ),
	action( 'upgrade-db' ),
	action( 'message-all', args=[ arg('path', True) ] ),
	action( 'register',
		args=[ arg('jid'), arg('username'), arg('password', True ),
			arg('language', True), arg( 'encoding', True ) ] ),
	action( 'unregister', args=[arg('jid')] ),
	action( 'set-vip-status',
		args=[ arg( 'jid' ), arg( 'status' ) ] ),
	action( 'list' ),
	action( 'no-backtraces' ),
	action( 'help', args=[ arg('cmd', True) ] ),
]

cli_cmds = [
	action( 'shell' ),
]

shell_cmds = [
	action( 'load', """Only act upon the transport that serves I{jid}. The prompt will be updated to reflect the change. If you use the special value B{all}, all transports will be loaded again. Note that you cannot currently load config-files where the filename does not end with .cfg.""",
		[ arg( 'jid' ) ] ),
	action( 'exit', """Exit this shell""" )
]

