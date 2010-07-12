import re
from spectrum.doc import doc

class doc( doc.doc ):
	def translate( self, text ):
		text = re.sub( r'B\{([^\}]*)\}', '\\\\fB\\1\\\\fR', text )
		text = re.sub( r'L\{([^\}]*)\}', '\\\\fB\\1\\\\fR', text )
		text = re.sub( r'I\{([^\}]*)\}', '\\\\fI\\1\\\\fR', text )
		text = re.sub( r'this group', 'this shell', text )
		text = re.sub( r'method', 'command', text )
		text = text.replace( '\n', '\n\n' )
		return text

	def create_documentation( self, action ):
		text = '.sp\n'

		text += self.translate( action.get_arg_string() ) + '\n'
		text += '.RS 4\n'

		text += self.translate( action.get_text( self.cl ) )
		text += '\n.RE'
		return text
		
