import re 

# ANSI escape sequences
ansi = { 'bold': "\033[1m", 'italic': "\033[4m", 'reset': "\033[0;0m" }

class arg:
	def __init__( self, name, optional=False, repeatable=False ):
		self.name = name
		self.repeatable = repeatable
		self.optional = optional

	def interactive_string( self ):
		string = '%s%s%s'%(ansi['italic'], self.name, ansi['reset'])
		if self.optional:
			return '[%s]'%(string)
		else:
			return string
	
	def man_string( self ):
		string = '\\fI%s\\fR'%(self.name)
		if self.optional:
			return '[%s]'%(string)
		else:
			return string

class action:
	def __init__( self, name, text, args=[] ):
		self.name = name
		self.args = args
		self.text = text

	def interactive_help( self, text=None ):
		header = [ '%s%s%s'%(ansi['bold'] ,self.name, ansi['reset'] ) ]
		for arg in self.args:
			header.append( arg.interactive_string() )
		print( ' '.join( header ) )
		print( '' )

		if text:
			text = self.parse_epydoc( text )
		else:
			text = self.text
		text = self.epydoc_to_terminal( text )
		print_terminal( text )

	def create_man_doc( self, text=None ):
		print( '.sp' )
		header = [ '\\fB%s\\fR'%(self.name) ]
		for arg in self.args:
			header.append( arg.man_string() )
		print( ' '.join( header ) )
		print( '.RS 4' )
		if text:
			text = self.parse_epydoc( text )
		else:
			text = self.text

		text = self.epydoc_to_man( text )
		print( text )
		print( '.RE' )

	def parse_epydoc( self, text ):
		new_doc = ''
		for line in text.splitlines():
			if line.strip().startswith( '@' ):
				break

			if line.strip() == '':
				new_doc += "\n"
			else:
				new_doc += line.strip() + " "

		new_doc = new_doc.strip()

		return new_doc
	
	def epydoc_to_terminal( self, text ):
		text = re.sub( r'B\{([^\}]*)\}', '%s\\1%s'%(ansi['bold'], ansi['reset']), text )
		text = re.sub( r'L\{([^\}]*)\}', '%s\\1%s'%(ansi['bold'], ansi['reset']), text )
		text = re.sub( r'I\{([^\}]*)\}', '%s\\1%s'%(ansi['italic'], ansi['reset']), text )
		text = re.sub( r'this group', 'this shell', text )
		text = re.sub( r'method', 'command', text )
		return text

	def epydoc_to_man( self, text ):
		text = re.sub( r'B\{([^\}]*)\}', '\\\\fB\\1\\\\fR', text )
		text = re.sub( r'L\{([^\}]*)\}', '\\\\fB\\1\\\\fR', text )
		text = re.sub( r'I\{([^\}]*)\}', '\\\\fI\\1\\\\fR', text )
		text = re.sub( r'this group', 'this shell', text )
		text = re.sub( r'method', 'command', text )
		text = text.replace( '\n', '\n\n' )

		# for the register command:
		text = text.replace( 'if this shell currently represents more then one transport', """if you don't specify a single config-file using the \\fB--config\\fR option""" )
		return text


def print_terminal( text, width=80 ):
	def real_len( string ):
		result = len( string )
		for code in ansi.values():
			count = string.count( code )
			result -= count * len( code )
		return result

	lines = []
	line_tmp = []

	for line in text.splitlines():
		words = line.split()
		for word in words:
			cur_line = ' '.join( line_tmp )
			
			if real_len( cur_line ) + real_len( word ) + 1 > width:
				lines.append( cur_line )
				line_tmp = [ word ]
			else:
				line_tmp.append( word )

		lines.append( ' '.join( line_tmp ) )
		line_tmp = []

		lines.append( '' )
	lines.append( ' '.join( line_tmp ) )

	print( '\n'.join( lines ).strip() )
	
cmds = [
	action( 'start', None ),
	action( 'stop', None ),
	action( 'restart', None ),
	action( 'reload', None ),
	action( 'stats', None ),
	action( 'upgrade-db', None ),
	action( 'message_all', None ),
	action( 'register', None, 
		[ arg('jid'), arg('username'), arg('password', True ), 
			arg('language', True), arg( 'encoding', True ) ] ),
	action( 'unregister', None, [arg('jid')] ),
	action( 'set-vip-status', None,
		[ arg( 'jid' ), arg( 'status' ) ] ),
	action( 'list', None ),

]

shell_cmds = [
	action( 'load', """Only act upon the transport that serves I{jid}. The prompt will be updated to reflect the change. If you use the special value B{all}, all transports will be loaded again. Note that you cannot currently load config-files where the filename does not end with .cfg.""", 
		[ arg( 'jid' ) ] )
]
