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

import os, sys, pwd, stat, time, signal, subprocess, resource
import spectrumconfigparser, config_interface, env
from ExistsError import ExistsError

class spectrum:
	"""
	An instance of this class represents a single spectrum instance and can
	be used to control the instance. 
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
		self.pid_file = self.config.get( 'service', 'pid_file' ).lower()

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

	def running( self ):
		"""
		Shortcut for L{status}, returns True if the instance is running,
		False otherwise.

		@rtype: boolean
		"""
		if self.status() == 0:
			return True
		else:
			return False

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


	def start( self, no_daemon=False, debug=False ):
		"""
		Starts the instance. This method will silently return if the
		instance is already started.

		@param no_daemon: Do not start spectrum as daemon if True.
		@type  no_daemon: boolean
		@param debug: Start spectrum in debug mode if True.
		@type  debug: boolean
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

		# fork and drop privileges:
		pid = os.fork()
		if pid:
			# we are the parent!
			pid, status = os.waitpid( pid, 0 )
			status = (status & 0xff00) >> 8
			if status == 0:
				return
			elif status == 1:
				raise RuntimeError( "db_version table does not exist", 1 )
			elif status == 2:
				raise RuntimeError( "database not up to date, update with spectrumctl upgrade-db", 1 )
			elif status == 3:
				raise RuntimeError( "Error connecting to the database", 1 )
			elif status == 100:
				raise RuntimeError( "Could not find spectrum binary", 1 )
			elif status == 101:
				raise RuntimeError( "Exception thrown, please contact the maintainers", 1 )
			elif status == 102:
				raise RuntimeError( "Username does not exist", 1 )
			else:
				raise RuntimeError( "Child exited with unknown exit status", 1 )
		else:
			exit_code = 0 
			try:
				# we are the child
				try:
					env.drop_privs( env.get_uid(), env.get_gid() )
				except RuntimeError, e:
					os._exit( 102 )

				# get the absolute path of the config file
				path = os.path.abspath( self.config_path )

				# check if database is at current version:
				check_cmd = [ self.get_binary(), '--check-db-version', path ]
				process = subprocess.Popen( check_cmd, stdout=subprocess.PIPE )
				process.communicate()
				if process.returncode != 0:
					os._exit( process.returncode )

				# finally start spectrum:
				cmd = [ self.get_binary() ]
				if no_daemon:
					cmd.append( '-n' )
				if debug:
					os.environ['MALLOC_PERTURB_'] = '254'
					os.environ['PURPLE_VERBOSE_DEBUG'] = '1'
					os.environ['MALLOC_CHECK_'] = '2'
					resource.setrlimit( resource.RLIMIT_CORE, (-1,-1) )

				cmd.append( path )
				os._exit( subprocess.call( cmd ) )
			except OSError, e:
				os._exit( 100 ) # spectrum wasn't found
			except Exception, e:
				print( "%s: %s"%( type(e), e.message ) )
				os._exit( 101 )

	def stop( self, debug=False ):
		"""
		Stops the instance (sends SIGTERM). If the instance is not
		running, the method will silently return. The method will also
		return the pid-file, if it still exists.

		@raise RuntimeError: If stopping the instance fails.
		@param debug: Print loads of debug-output if True.
		@type  debug: boolean
		@see:	U{LSB standard for init script actions
			<http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html>}
		"""
		status = self.status()
		if status == 3:
			# stopping while not running is also success!
			if debug:
				print( 'Transport already stopped' )
			return
		elif status == 1:
			if debug:
				print( 'Transport already stopped, but PID file stil existed' )
			os.remove( self.pid_file )
			return

		pid = self.get_pid()
		try:
			if debug:
				env.log( 'Attempting to kill PID: %s'%(pid) )

			os.kill( pid, signal.SIGTERM )
			time.sleep( 0.2 )
			
			for i in range(1, 10):
				if debug:
					env.log( 'Try again...' )
				status = self.status()
				if status == 3 or status == 1:
					os.remove( self.pid_file )
					return 0
				os.kill( pid, signal.SIGTERM )
				time.sleep( 1 )

			status = self.status()
			if status == 3 or status == 1:
				return 0
			else:
				if debug:
					env.log( 'Attempting to kill with SIGABRT' )
					os.kill( pid, signal.SIGABRT )
				else:
					env.log( 'Attempting to kill with SIGKILL' )
					os.kill( pid, signal.SIGKILL )
				time.sleep( 0.2 )
				status = self.status()
				if status == 3 or status == 1:
					if debug:
						env.log( 'ok' )
					return 0
				else:
					raise RuntimeError( "Spectrum did not die", 1 )
		except OSError, e:
			env.log( "pid file: '%s' (%s)"%(self.pid_file, debug) )
			raise RuntimeError( "Failed to kill pid '%s'"%(pid), 1 )
		except Exception, e:
			env.log( "%s: %s"%(type(e), e) )
			raise RuntimeError( "Unknown Error occured", 1 )

	def restart( self, no_daemon=False, debug=False ):
		"""
		Restarts the instance (kills the process and starts it again).
		This method just calles L{stop} and L{start}.
		
		@param no_daemon: Passed to L{start}.
		@type  no_daemon: boolean
		@param debug: Passed to L{start}.
		@type  debug: boolean
		@raise RuntimeError: If stopping/starting the instance fails.
		@see:	U{LSB standard for init script actions
			<http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html>}
		"""
		self.stop( debug )
		self.start( no_daemon, debug )
	
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

		# fork and drop privileges:
		pid = os.fork()
		if pid:
			# we are the parent!
			pid, status = os.waitpid( pid, 0 )
			status = (status & 0xff00) >> 8
			if status == 0:
				return
			elif status == 1:
				raise RuntimeError( "db_version table does not exist", 1 )
			elif status == 3:
				raise RuntimeError( "Error connecting to the database", 1 )
			elif status == 100:
				raise RuntimeError( "Could not find spectrum binary", 1 )
			elif status == 101:
				raise RuntimeError( "Exception thrown, please contact the maintainers", 1 )
			elif status == 102:
				raise RuntimeError( "Username does not exist", 1 )
			else:
				raise RuntimeError( "Child exited with unknown exit status", 1 )
		else:
			exit_code = 0
			try:
				# drop privileges:
				try:
					env.drop_privs( env.get_uid(), env.get_gid() )
				except RuntimeError, e:
					os._exit( 102 )

				check_cmd = [ self.get_binary(), '--check-db-version', path ]
				process = subprocess.Popen( check_cmd, stdout=subprocess.PIPE )
				process.communicate()

				if process.returncode == 2:
					update_cmd = [ 'spectrum', '--upgrade-db', path ]
					process = subprocess.Popen( update_cmd, stdout=subprocess.PIPE )
					process.communicate()
					os._exit( process.returncode )

				else:
					os._exit( process.returncode )
			except OSError, e:
				os._exit( 100 ) # spectrum wasn't found
			except Exception, e:
				print( "%s: %s"%( type(e), e.message ) )
				os._exit( 101 )
	
	def message_all( self, message ):
		"""
		Send a message to all users currently online.
		
		@param message: The message to send
		@type  message: str
		@raise RuntimeError: In case the command fails.
		"""
		interface = config_interface.config_interface( self )
		state = 'ADHOC_ADMIN_SEND_MESSAGE'
		fields = [('message', 'text-multi', message)]

		interface.command( state, fields )
		return 0
	
	def register( self, jid, username, passwd, lang, enc, status ):
		"""
		Register a user to the transport. 

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
		@param status: The VIP status of the user: 1 if true, 0 if not
		@type  status: str

		@raise RuntimeError: In case the command fails.
		"""
		interface = config_interface.config_interface( self )
		state = 'ADHOC_ADMIN_REGISTER_USER'
		fields = [ ('user_jid', 'text-single', jid ),
			('user_username', 'text-single', username ),
			('user_password', 'text-single', passwd ),
			('user_language', 'text-single', lang ),
			('user_encoding', 'text-single', enc ),
			('user_vip', 'text-single', status ) ]
		interface.command( state, fields )
		return 0

	def unregister( self, jid ):
		"""
		Unregister a user from the transport.
		
		@param jid: The JID to unregister
		@type  jid: string
		@raise RuntimeError: In case the command fails.
		"""
		interface = config_interface.config_interface( self )
		state = 'ADHOC_ADMIN_UNREGISTER_USER'
		fields = [ ( 'user_jid', 'text-single', jid ) ]
		interface.command( state, fields )
		return 0

	def set_vip_status( self, jid, state ):
		"""
		Set the VIP status of a user.

		@param jid: The JID that should have its status set
		@type  jid: str
		@param state: The state you want to set. This should be "0" for
			disabling VIP status and "1" for enabling it. Note that
			this argument really I{is} a string.
		@type  state: str
		@raise RuntimeError: In case the command fails.
		"""
		interface = config_interface.config_interface( self )
		state = 'ADHOC_ADMIN_USER2'
		fields = [('user_jid', 'hidden', jid ),
			('user_vip', 'boolean', state ) ]
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
			raise RuntimeError( "%s"%(e.message ) )
