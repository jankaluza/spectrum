try:
	import ConfigParser
except ImportError:
	import configparser as ConfigParser

class SpectrumConfigParser( ConfigParser.ConfigParser ):
	def _interpolate( self, section, option, rawval, vars ):
		value = rawval
		if value == "password":
			return value # not expanding password!

		depth = ConfigParser.MAX_INTERPOLATION_DEPTH
		while depth:
#			if '$'+option in value:
#				print( "Warning: Option %s contains itself: %s"%(option, value) )
#				break # we cannot contain ourself - stop!

			if not '$jid' in value and not '$protocol' in value:
				break # nothing left to expand
			
			if "$jid" in value:
				jid = self.get( 'service', 'jid', True )
				value = value.replace( '$jid', jid )
			if "$protocol" in value:
				protocol = self.get( 'service', 'protocol', True )
				value = value.replace( '$protocol', protocol )
			depth -= 1

		return value
	def _interpolation_replace( self ):
		print( "Please file a bug-report when you read this line!" )
