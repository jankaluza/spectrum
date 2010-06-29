# -*- coding: utf-8 -*-
#
# This file is part of spectrumctl. See spectrumctl.py for a description.
#
# Copyright (C) 2009, 2010 Mathias Ertl
#
# Spectrumctl is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>.
"""
Represents a single spectrum instance, see L{spectrum.spectrum}.
"""


import os, pwd, stat, time, signal, subprocess
import env
try:
	from spectrum import spectrumconfigparser, ExistsError, config_interface
except ImportError:
	import spectrumconfigparser, ExistsError, config_interface

ExistsError = ExistsError.ExistsError

class spectrum:
	"""
	An instance of this class represents a single spectrum instance and can
	be used to control the instance. 

	@todo: Replace the params attribute with more convinient named
		parameters (params is just here for historic reasons...)
	"""

	def __init__( self, config_path ):
		"""
		Constructor.

		@param config_path: The full path to the configuration file that
		configures the given instance.
		@type  config_path: str
		"""
		self.config_path = os.path.normpath( config_path )
		
		self.config = spectrumconfigparser.SpectrumConfigParser()
		self.config.read( config_path )
		self.pid_file = self.config.get( 'service', 'pid_file' )

	def get_binary( self ):
		"""
		Convenience function used to get the binary that will be used to
		start the instance. This will return the value of environment
		variable SPECTRUM_PATH or just 'spectrum' if it is not set.

		@return: the binary that will be used to start spectrum
		@rtype: str
		"""
		try:
			return os.environ['SPECTRUM_PATH']
		except KeyError:
			return 'spectrum'

	def get_jid( self ):
		"""
		Convenience function to get the JID of this instance.

		@return: The JID of this instance
		@rtype: str
		"""
		return self.config.get( 'service', 'jid' )

	def enabled( self ):
		"""
		Convenience function to get if this instance is currently
		enabled in the config file.

		@return: True if this instance is enabled, False otherwise.
		@rtype: boolean
		"""
		return self.config.getboolean( 'service', 'enable' )

	def get_pid( self ):
		"""
		Get the pid of the instance as noted in the pid file. 

		@return: The PID of the process or -1 the instance is not
			running
		@rtype: int
		"""
		try:
			return int( open( self.pid_file ).readlines()[0].strip() )
		except:
			return -1

	def check_environment( self ):
		"""
		This function is used to do tight checks on the environment when
		starting the spectrum instance.
		
		@return: A tuple of type (int, str) with the return value (0
			means success, 1 indicates an error) and a
			human-readable string of what happened.
		@rtype: tuple
		@todo: The return value of this method is actually not used
			anymore.
		@raise RuntimeError: If anything (file
			permissions/ownership, ... ) is not as expected.
		"""
		# check if spectrum user exists:
		if os.name == 'posix':
			try:
				user = pwd.getpwuid( env.get_uid() )
			except KeyError:
				raise RuntimeError( env.get_uid(), 'UID does not exist' )
		else:
			# on windows, there is no real way to get the info that
			# we need
			return 0, "ok"
		
		# we must be root
		if os.getuid() != 0:
			raise RuntimeError( "User", "Must be root to start spectrum" )
		
		# check permissions for config-file:
		env.check_ownership( self.config_path, 0 )
		# we don't care about user permissions, group MUST be read-only
		env.check_permissions( self.config_path, [ stat.S_IRGRP ],
			[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR ] )

		# config_interface:
		config_interface = self.config.get( 'service', 'config_interface' )
		if config_interface:
			# on some old (pre 0.1) installations config_interface does not exist
			dir = os.path.dirname( config_interface )
			try: 
				env.check_exists( dir, typ='dir' )
				env.check_writable( dir )
			except ExistsError:
				env.create_dir( dir )

		# filetransfer cache
		filetransfer_cache = self.config.get( 'service', 'filetransfer_cache' )
		try:
			env.check_exists( filetransfer_cache, 'dir' )
			env.check_ownership( filetransfer_cache )
			env.check_permissions( filetransfer_cache, # rwxr-x---
				[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR, 
				  stat.S_IRGRP, stat.S_IXGRP ] )
		except ExistsError:
			env.create_dir( filetransfer_cache )

		# pid_file:
		pid_file = self.config.get( 'service', 'pid_file' )
		pid_dir = os.path.dirname( pid_file )
		try:
			env.check_exists( pid_dir, typ='dir' )
			env.check_writable( pid_dir )
		except ExistsError:
			env.create_dir( pid_dir )

		# log file
		log_file = self.config.get( 'logging', 'log_file' )
		try:
			# does not exist upon first startup:
			if not env.is_named_pipe( log_file ):
				env.check_exists( log_file )
			env.check_ownership( log_file, gid=-1 )
			env.check_permissions( log_file, # rw-r-----
				[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IRGRP ] )
		except ExistsError:
			log_dir = os.path.dirname( log_file )
			try:
				env.check_exists( log_dir, typ='dir' )
			except ExistsError:
				env.create_dir( log_dir )

		# sqlite database
		if self.config.get( 'database', 'type' ) == 'sqlite':
			db_file = self.config.get( 'database', 'database' )
			try:
				# might not exist upon first startup:
				env.check_exists( db_file )
				env.check_ownership( db_file )
				env.check_permissions( db_file, # rw-r-----
					[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IRGRP ] )
			except ExistsError:
				db_dir = os.path.dirname( db_file )
				try:
					env.check_exists( db_dir, typ='dir' )
					env.check_ownership( db_dir )
					env.check_permissions( db_dir, # rwxr-x---
						[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR, 
						  stat.S_IRGRP, stat.S_IXGRP ] )
				except ExistsError:
					env.create_dir( os.path.dirname( db_file ) )
		
		# purple userdir
		userdir = self.config.get( 'purple', 'userdir' )
		try:
			env.check_exists( userdir, 'dir' )
			env.check_ownership( userdir )
			env.check_permissions( userdir, # rwxr-x---
				[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR, 
				  stat.S_IRGRP, stat.S_IXGRP ] )
		except ExistsError:
			env.create_dir( userdir )

		return 0, 'ok'

	def list( self ):
		"""
		Get the current process characteristics of this instance. This
		function is intended to be used to get machine-readable values
		of the spectrumctl action "list" and is used by
		L{spectrum_group<spectrum.spectrum_group.spectrum_group>}.

		The tuple consists of four strings:
		  
		  - B{pid:} the PID of the current process or '-' if the
		    instance is not running.
		  - B{proto:} The protocol this instance serves.
		  - B{host:} The JID this instance serves.
		  - B{status:} A human-readable string describing the status of
		    the instance.

		@return: A tuple with status information, see above for more
			information.
		@rtype: tuple
		"""
		pid = str( self.get_pid() )
		if pid == "-1":
			pid = '-'
		proto = self.config.get( 'service', 'protocol' )
		host = self.get_jid()
		status = self.status_str()
		return (pid, proto, host, status)

	def status_str( self, pid=None ):
		"""
		Translates the int returned by L{status} into a
		human-readable string.

		@return: A human readable string of the current status of this
			instance.
		@rtype: str
		"""
		status = self.status( pid )
		if status == 0:
			return( "running" )
		elif status == 1:
			return( "dead but pid-file exists" )
		elif status == 3:
			return( "not running" )

	def status( self, pid=None ):
		"""
		Determines if the instance is running.

		This method uses the /proc filesystem if it exists and tries
		to send signal 0 (which should be doing nothing) if not.

		@raise RuntimeError: If the pid file cannot be parsed or an
			unknown error occurs.
		@return: The int representing the current status, conforming to
			the LSB standards for init script actions..
		@rtype: int
		@see:	U{LSB standard for init script actions
			<http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html>}
		"""
		if not os.path.exists( self.pid_file ):
			return 3 # "not running"

		if pid == None:
			pid = self.get_pid()
			if pid <= 0:
				raise RuntimeError( "could not parse pid file", 4 )
		
		if os.path.exists( '/proc' ):
			if os.path.exists( '/proc/%i' %(pid) ):
				return 0 # "running"
			else:
				return 1 # "not running but pid file exists."
		else: # no proc!
			try:
				os.kill( pid, 0 )
				return 0 # "running."
			except OSError, err:
			# except OSError as err: #python3
				if err.errno == errno.ESRCH:
					return 1 # "not running but pid file exists."
				else:
					raise RuntimeError( "unknown (OSError)", 4 )
			except:
				raise RuntimeError( "unknown (Error)", 4 )


	def start( self ):
		"""
		Starts the instance. This method will silently return if the
		instance is already started.

		@raise RuntimeError: If starting the instance fails.
		@see:	U{LSB standard for init script actions
			<http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html>}
		"""
		status = self.status()
		if status == 0:
			# starting while we are already running is also success!
			return 
		elif status == 1:
			# not running but pid file exists
			os.remove( self.pid_file )

		# do rigorous permission-checking:
		try:
			self.check_environment()
		except RuntimeError, e:
			raise RuntimeError( "%s: %s" % e.args, 1 )

		# get the absolute path of the config file, so we can change to
		# spectrums homedir
		path = os.path.abspath( self.config_path )
		home = pwd.getpwuid( env.get_uid() )[5]
		os.chdir( home )

		# check if database is at current version:
		check_cmd = [ self.get_binary(), '--check-db-version', path ]
		check_cmd = env.su_cmd( check_cmd )
		process = subprocess.Popen( check_cmd, stdout=subprocess.PIPE )
		process.communicate()
		if process.returncode == 1:
			raise RuntimeError( "db_version table does not exist", 1)
		elif process.returncode == 2: 
			raise RuntimeError( "database not up to date, update with spectrumctl upgrade-db"%(self.config_path), 1 )
		elif process.returncode == 3:
			raise RuntimeError( "Error connecting to the database", 1 )

		# finally start spectrum:
		cmd = [ self.get_binary() ]
		if env.options.no_daemon:
			cmd.append( '-n' )
		if env.options.debug:
			os.environ['MALLOC_PERTURB_'] = '254'
			os.environ['PURPLE_VERBOSE_DEBUG'] = '1'
			os.environ['MALLOC_CHECK_'] = '2'
			cmd[0] = 'ulimit -c unlimited; ' + cmd[0]
		cmd.append( path )
		cmd = env.su_cmd( cmd )
		retVal = subprocess.call( cmd )
		if retVal != 0:
			raise RuntimeError( "Could not start spectrum instance", retVal )

		return

	def stop( self ):
		"""
		Stops the instance (sends SIGTERM). If the instance is not
		running, the method will silently return. The method will also
		return the pid-file, if it still exists.

		@raise RuntimeError: If stopping the instance fails.
		@see:	U{LSB standard for init script actions
			<http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html>}
		"""
		status = self.status()
		if status == 3:
			# stopping while not running is also success!
			return
		elif status == 1:
			os.remove( self.pid_file )
			return

		pid = self.get_pid()
		debug = 0
		try:
			os.kill( pid, signal.SIGTERM )
			debug += 1
			time.sleep( 0.2 )
			
			for i in range(1, 10):
				debug += 1
				status = self.status()
				if status == 3 or status == 1:
					os.remove( self.pid_file )
					return 0
				os.kill( pid, signal.SIGTERM )
				time.sleep( 1 )
			raise RuntimeError( "Spectrum did not die", 1 )
		except OSError, e:
			print( "pid file: '%s' (%s)"%(self.pid_file, debug) )
			raise RuntimeError( "Failed to kill pid '%s'"%(pid), 1 )
		except Exception, e:
			print( e )
			raise RuntimeError( "Unknown Error occured", 1 )

	def restart( self ):
		"""
		Restarts the instance (kills the process and starts it again).
		This method just calles L{stop} and L{start}.
		
		@raise RuntimeError: If stopping/starting the instance fails.
		@see:	U{LSB standard for init script actions
			<http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html>}
		"""
		self.stop()
		self.start()
	
	def reload( self ): # send SIGHUP to process
		"""
		Reload instance (send SIGHUP) which causes spectrum to reopen
		log-files etc.
		
		@raise RuntimeError: If reloading the instance fails (i.e. the
			instance is not running)
		@see:	U{LSB standard for init script actions
			<http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html>}
		"""
		if self.status() != 0:
			raise RuntimeError( "not running", 1 )

		pid = self.get_pid()
		try:
			os.kill( pid, signal.SIGHUP )
		except:	
			raise RuntimeError( "Unknown error occured", 1 )

	def upgrade_db( self ):
		"""
		Call C{specturm --check-db-version} to see if the schema version
		of the database is up to date. If not, call C{spectrum
		--upgrade-db} and try to upgrade the schema.

		@raise RuntimeError: If any of the launched commands fail.
		"""
		path = os.path.abspath( self.config_path )

		check_cmd = [ self.get_binary(), '--check-db-version', path ]
		check_cmd = env.su_cmd( check_cmd )
		process = subprocess.Popen( check_cmd, stdout=subprocess.PIPE )
		process.communicate()

		if process.returncode == 2:
			update_cmd = [ 'spectrum', '--upgrade-db', path ]
			update_cmd = env.su_cmd( update_cmd )
			process = subprocess.Popen( update_cmd, stdout=subprocess.PIPE )
			process.communicate()
			if process.returncode != 0:
				raise RuntimeError( "%s exited with status %s"%(" ".join(update_cmd),process.returncode), 1)
		elif process.returncode == 1:
			raise RuntimeError( "db_version table does not exist", 1)
		elif process.returncode == 3:
			raise RuntimeError( "Error connecting to the database", 1 )

	def message_all( self, params ):
		"""
		Send a message to all users currently online.
		
		@param params: The first and only element of the list should
			be a string representing the message to be send.
		@type  params: list
		@raise RuntimeError: In case the command fails.
		"""
		interface = config_interface.config_interface( self )
		state = 'ADHOC_ADMIN_SEND_MESSAGE'
		fields = [('message', 'text-multi', params[0])]

		interface.command( state, fields )
		return 0
	
	def register( self, params ):
		"""
		Register a user to the transport. The elements of params should
		be as follows:

		  0. The JID of the user to be registered
		  1. The username the user in the legacy network
		  2. The password for the legacy network
		  3. The language code used by the user (e.g. 'en')
		  4. The default character encoding (e.g. 'utf8')
		  5. The VIP status of the user: 1 if true, 0 if not

		@param params: The parameters passed to this request. See above
			for more information.
		@type  params: list
		@raise RuntimeError: In case the command fails.
		"""
		interface = config_interface.config_interface( self )
		state = 'ADHOC_ADMIN_REGISTER_USER'
		fields = [ ('user_jid', 'text-single', params[0] ),
			('user_username', 'text-single', params[1] ),
			('user_password', 'text-single', params[2] ),
			('user_language', 'text-single', params[3] ),
			('user_encoding', 'text-single', params[4] ),
			('user_vip', 'text-single', params[5] ) ]
		interface.command( state, fields )
		return 0

	def unregister( self, params ):
		"""
		Unregister a user from the transport. The only element of params
		should be the JID that should be unregistered.
		
		@param params: The parameters passed to this request. See above
			for more information.
		@type  params: list
		@raise RuntimeError: In case the command fails.
		"""
		interface = config_interface.config_interface( self )
		state = 'ADHOC_ADMIN_UNREGISTER_USER'
		fields = [ ( 'user_jid', 'text-single', params[0] ) ]
		interface.command( state, fields )
		return 0

	def set_vip_status( self, params ):
		"""
		Set the VIP status of a user. The first element of params should
		be the JID to act upon while the second element should be either
		"1" if the user is to become VIP or "0" if the user should no
		longer be VIP. Note that still all elements have to be a string.

		@param params: The parameters passed to this request. See above
			for more information.
		@type  params: list
		@raise RuntimeError: In case the command fails.
		"""
		interface = config_interface.config_interface( self )
		state = 'ADHOC_ADMIN_USER2'
		fields = [('user_jid', 'hidden', params[0] ),
			('user_vip', 'boolean', params[1] ) ]
		interface.command( state, fields )
		return 0

	def get_stats( self ):
		"""
		Get statistics of the current transport. Note that this method
		requires the xmpp library to be installed.

		@return: The IQ packet send back by the client
		@rtype:
			U{xmpp.protocol.Iq<http://xmpppy.sourceforge.net/apidocs/xmpp.protocol.Iq-class.html>}
		"""
		import xmpp

		interface = config_interface.config_interface( self )
		ns = 'http://jabber.org/protocol/stats'
		nodes = []
		for name in [ 'uptime', 'users/registered', 'users/online', 
				'contacts/online', 'contacts/total', 
				'messages/in', 'messages/out', 'memory-usage' ]:
			nodes.append( xmpp.simplexml.Node( 'stat', 
				attrs={'name': name} ) )
		try:
			return interface.query( nodes, ns )
		except RuntimeError, e:
			raise RuntimeError( "%s: %s"%(self.get_jid(), e.message ) )
