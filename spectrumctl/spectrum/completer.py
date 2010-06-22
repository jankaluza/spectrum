import readline

class completer:
	def __init__( self, cmds, jids ):
		readline.set_completer( self.complete )
		readline.parse_and_bind("tab: complete")
		self.cmds = cmds
		self.jids = jids
	
	def set_jids( self, jids ):
		self.jids = jids

	def complete( self, word, state, choices=None ):
		line = readline.get_line_buffer()
		
		if choices == None:
			# initial call
			choices = self.cmds

		if line.strip() == '':
			choice = choices[state]
			return choice

		cmds = [ s for s in choices if s.startswith( word ) ]
		if len( cmds ) == 1 and state == 0:
			return cmds[0] + ' '
		elif line.endswith( ' ' ):
			pass
		else:
			return cmds[state]

