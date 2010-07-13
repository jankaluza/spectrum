import re
from spectrum.doc import doc

class doc( doc.doc ):
	def translate( self, text ):
		# handle optional commands
		text = re.sub( r'\[I\{([^\}]*)\}\]', '@[@+\\1+@]@', text )

		text = re.sub( r'B\{([^\}]*)\}', '*\\1*', text )
		text = re.sub( r'L\{([^\}]*)\}', '+\\1+', text )
		text = re.sub( r'I\{([^\}]*)\}', '+\\1+', text )
		text = re.sub( r'this group', 'this shell', text )
		text = re.sub( r'method', 'command', text )
		text = text.replace( '\n', '\n\n' )

		# for the register command:
		text = text.replace( 'if this shell currently represents more then one transport', """if you don't specify a single config-file using the +@--config@+ option""" )
		return text

	def create_documentation( self, action ):
		text = self.translate( action.get_arg_string() ) + '\n\n'
		
		action_text = self.translate( action.get_text( self.cl ) )
		lines = []
		for line in action_text.splitlines():
			if line:
				lines.append( 'p(. ' + line )
			else:
				lines.append( line )

		text += '\n'.join( lines )
		return text
