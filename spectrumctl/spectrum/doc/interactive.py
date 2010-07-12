import re
from spectrum.doc import doc

class doc( doc.doc ):
	bold = "\033[1m"
	italic = "\033[4m"
	reset = "\033[0;0m"

	def get_real_strlen( self, string ):
		result = len( string )
		for code in [ doc.bold, doc.italic, doc.reset ]:
			count = string.count( code )
			result -= count * len( code )
		return result

	def translate( self, text ):
		# for the register command:
#		text = text.replace( 'if this shell currently represents more then one transport', """if you don't specify a single config-file using the I{--config} option""" )

		text = re.sub( r'B\{([^\}]*)\}', '%s\\1%s'%(doc.bold, doc.reset), text )
		text = re.sub( r'L\{([^\}]*)\}', '%s\\1%s'%(doc.italic, doc.reset), text )
		text = re.sub( r'I\{([^\}]*)\}', '%s\\1%s'%(doc.italic, doc.reset), text )
		text = re.sub( r'this group', 'this shell', text )
		text = re.sub( r'method', 'command', text )
#		text = text.replace( '\n', '\n\n' )
		
		return text


	def create_documentation( self, action ):
		text = self.translate( action.get_arg_string() ) + '\n'
		
		action_text = self.translate( action.get_text( self.cl ) )
		lines = []
		for line in action_text.splitlines():
			if line:
				lines.append( '\t' + line )
			else:
				lines.append( line )

		text += '\n'.join( lines )
		return text


	def print_terminal( self, text, width=80, indent='    ' ):
		lines = [] 
		line_tmp = []

		for line in text.splitlines():
			words = line.split()
			for word in words:
				cur_line = ' '.join( line_tmp )
	
				if self.get_real_strlen( cur_line ) + self.get_real_strlen( word ) + 1 > width:
					lines.append( cur_line )
					if indent:
						line_tmp = [ indent, word ]
					else:
						line_tmp = [ word ]
				else:   
					if len( line_tmp ) == 0 and indent:
						line_tmp.append( indent )
					line_tmp.append( word )

			lines.append( ' '.join( line_tmp ) )
			line_tmp = []

			lines.append( '' )
		lines.append( ' '.join( line_tmp ) )

		print( '\n'.join( lines ).strip() + '\n' )
