"""
This class represents multiple spectrum instances, see L{spectrum_group}.
"""
import os, sys, socket

try:
	from spectrum import spectrum
except ImportError:
	import spectrum

class spectrum_group:
	"""
	An object of this class represents one or more spectrum instances. The
	instances it represents are defined upon instantiation (see the
	L{constructor<__init__>}) or via the "load" command when in interactive
	mode (see L{shell}).

	All public functions of this class represent possible actions that
	spectrumctl supports either via command-line or in interactive mode.
	"""

	def __init__( self, options ):
		"""
		Constructor. 

		Upon instantiation, this object will either represent a single
		instance, if C{options.config} refers to a path (and is not None).
		This is equivalent to the C{--config} option on the command-line.
		
		If options.config is None, the object will represent all
		config-files found in the path referred to by
		C{options.config_dir} (which can be configured with
		C{--config-dir} on the command-line).

		@param options: The options returned by the config parser. This
			is the first return-value of
			U{OptionParser.parse_args<http://docs.python.org/library/optparse.html>}.
		"""
		self.options = options
		if options.config:
			self.instances = [ spectrum( options.config ) ]
		else:
			self._load_instances()

	def _load_instances( self, jid=None ):
		"""
		Load spectrum instances. This method is called by the
		constructor and the "load" command.

		@param jid: The JID to load. If None, it will load all
			parameters in options.config_dir.
		@type jid: str
		@return: The number of transports loaded.
		@rtype: int
		"""
		self.instances = []
		config_list = os.listdir( self.options.config_dir )
		for config in config_list:
			path = '%s/%s'%(self.options.config_dir, config)
			if not os.path.isfile( path ) or not path.endswith( '.cfg' ):
				continue

			instance = spectrum( path )
			if jid and instance.get_jid() == jid:
				# only load the specified JID
				self.instances.append( instance )
			elif not jid: 
				# jid is None - load all instances
				self.instances.append( instance )
		return( len( self.instances ) )
		

	def _log( self, msg, newline=True ):
		if not self.options.quiet:
			if newline:
				print( msg )
			else:   
				print( msg ),
#				print( msg, end='' ) #python 3

	def _simple_action( self, action, title=None ):
		ret = 0
		for instance in self.instances:
			if not title:
				title = action.title()
			jid = instance.get_jid()
			self._log( "%s %s..."%(title, jid), False )
			try:
				getattr( instance, action )()
				self._log( "Ok." )
			except socket.error, e:
				if hasattr( e, 'strerror' ):
					# python2.5 does not have msg.strerror
					err = e.strerror
				else:
					err = e.message
				self._log( "Failed (config_interface: %s)"%(err) )
			except RuntimeError, e:
				self._log( "Failed (%s)"%e.args[0] )
				ret += e.args[1]

		return ret

	def start( self ):
		"""
		Start all instances that this group currently acts upon.

		@return: 0 upon success, an int >1 otherwise. 
		@rtype: int
		"""
		return self._simple_action( 'start' )

	def stop( self ):
		"""
		Stop all instances that this group currently acts upon. 

		@return: 0 upon success, an int >1 otherwise. 
		@rtype: int
		"""
		return self._simple_action( 'stop' )

	def restart( self ):
		"""
		Restart all instances that this group currently acts upon. This
		is essentially an alias for first calling L{stop} and then
		L{start}, so users will be disconnected when invoking this
		method.

		@return: 0 upon success, an int >1 otherwise. 
		"""
		return self._simple_action( 'restart' )

	def reload( self ):
		"""
		Reload all instances that this group currently acts upon. This
		just causes spectrum instances to reopen their log-files, it does
		not change any runtime configuration. Unlike L{restart}, this
		method does not stop the transport, hence users will not notice
		anything.
		
		@return: 0 upon success, an int >1 otherwise.
		@rtype: int
		"""
		return self._simple_action( 'reload' )

	def stats( self ):
		"""
		Get runtime statistics for all instances that this group
		currently acts upon. 

		@return: 0
		@rtype: int
		"""
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
		print( "\n\n".join( output ) )
		return 0
	
	def upgrade_db( self ):
		"""
		Try to upgrade the database schema of every instance that this
		group currently acts upon.
		
		@return: 0
		@rtype: int
		"""
		return self._simple_action( 'upgrade_db', "Upgrading db for" )

	def message_all( self, *args ):
		"""
		Message all users that are currently online. If message starts
		"file:" it will take the remaining part as path and sends its
		contents instead.
		
		@param args: List with the words to send.
		"""
		if len( args ) == 0:
			print( "Error: message_all <message>: Wrong number of arguments" )
			return 1

		if args[0].startswith( "file:" ):
			filename = self.args[0][5:]
			self.args[0] = open( filename ).read()

		print( "Messaging all users:" )
		
		for instance in self.instances:
			print( instance.get_jid() + '...' ),
			try:
				instance.message_all( args )
				print( "ok." )
			except RuntimeError, e:
				print( "Error: " + e.message )

	def register( self, jid, username, passwd=None, lang=None, enc=None ):
		"""
		Register the given user using the given legacy network account.
		The first argument is the JID of the user that wants to register
		and the second argument is his/her username on the legacy
		network. 
		
		B{Note:} If one of the optional arguments is not given, this
		command will interactively ask for more details!
		
		@param jid: The JID of the user to be registered.
		@type  jid: str
		@param username: The username of the user in the legacy network.
		@type  username: str
		@param passwd: The password for the legacy network.
		@type  passwd: str
		@param lang: The language code used by the user (e.g. 'en').
		@type  lang: str
		@param enc: The default character encoding (e.g. 'utf8').
		@type  enc: str
		"""
		if len( self.instances ) != 1:
			print( "Error: This command can only be executed with a single instance" )
			return 1

		if not passwd:
			import getpass
			pwd_in = getpass.getpass( "password to remote network: " )
			pwd2_in = getpass.getpass( "confirm: " )
			if pwd2_in != pwd_in:
				print( "Error: Passwords do not match" )
			else:
				passwd = pwd_in

		if not lang:
			lang = raw_input( 'Language (default: en): ' )
			if lang == '':
				lang = 'en'

		if not enc:
			enc = raw_input( 'Encoding (default: utf8): ' )
			if enc == '':
				enc = 'utf8'

		status = "0" # we don't ask for VIP status
 
		for instance in self.instances:
			print( instance.get_jid() + '...' ),
			answer = instance.register( jid, username, passwd, lang, enc, status)
			print( answer )

	def unregister( self, jid ):
		"""
		Unregister a user.
		
		@param jid: The JID to unregister
		@type  jid: string
		"""
		print( "Unregistering user %s" %(jid) )
		for instance in self.instances:
			print( instance.get_jid() + '...' ),
			try:
				instance.unregister( jid )
				print( "ok." )
			except RuntimeError, e:
				print( "Error: " + e.message )

	def set_vip_status( self, jid, state ):
		"""
		Set the VIP status of a user.
		
		@param jid: The JID that should have its status set
		@type  jid: str
		@param state: The state you want to set. This should be "0" for
			disabling VIP status and "1" for enabling it. Note that
			this argument really I{is} a string.
		@type  state: str
		"""

		status_string = "Setting VIP-status for '%s' to "
		if state == "0":
			status_string += "False:"
		elif state == "1":
			status_string += "True:"
		else:
			print( "Error: status (third argument) must be either 0 or 1" )
			return 1
		print( status_string%(jid) )

		for instance in self.instances:
			print( instance.get_jid() + '...' ),
			try:
				instance.set_vip_status( jid, state )
				print( "ok." )
			except RuntimeError, e:
				print( "Error: " + e.message )


	def list( self ):
		"""
		List all selected transports along with their pid, protocol and hostname.
		"""
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

	def _get_prompt( self ):
		if len(self.instances) == 1:
			prompt = self.instances[0].get_jid()
		elif len( self.instances ) == 0:
			prompt = "<none>"
		else:
			prompt = "<all transports>"
		return prompt + " $ "

	def shell( self ):
		"""
		Launch an interactive shell.
		"""
		cmds = [ x for x in dir( self ) if not x.startswith( '_' ) and x != "shell" ]
		cmds = [ x for x in cmds if type(getattr( self, x )) == type(self.shell) ]
		cmds += ['exit', 'load', 'help' ]
		jids = [ x.get_jid() for x in self.instances ]
		import completer
		compl = completer.completer(cmds, jids)
		cmd = ""

		while( True ):
			try:
				words = raw_input( self._get_prompt() ).split()
				words = [ x.strip() for x in words ]
				if len( words ) == 0:
					continue

				cmd = words[0]

				if cmd == "exit":
					raise EOFError() # same as CTRL+D
				elif cmd == "load":
					try:
						if words[1] == "all":
							ret = self._load_instances()
						else:
							ret = self._load_instances( words[1])

						if ret == 0:
							print( "Error: no transports found." )
						elif ret == 1:
							print( "%s loaded."%(self.instances[0].get_jid() ) )
						else:
							print( "%s transports loaded."%(ret) )

						compl.set_jids( [ x.get_jid() for x in self.instances ] )
					except IndexError:
						print( "Error: Give a JID to load or 'all' to load all (enabled) files" )
				elif cmd == "help":
					print( "Help not implemented yet" )
				elif cmd in cmds:
					getattr( self, cmd )( *words[1:] )
				else:
					print( "Unknown command, try 'help'." )
					print( cmd )
			except TypeError, e:
				print( "Wrong number of arguments, try 'help <cmd>' for help." )
			except KeyboardInterrupt:
				print
				continue
			except RuntimeError, e:
				print( e.message )
			except EOFError:
				print 
				return
			except Exception, e:
				print( "Type: %s"%(type(e)) )
				print( e )
