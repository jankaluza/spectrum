# Copyright (C) 2009, 2010 Mathias Ertl
#
# This file is part of Spectrumctl.
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

import os, time, signal, subprocess
try:
	from spectrum import spectrumconfigparser
except ImportError:
	import spectrumconfigparser

class spectrum:
	def __init__( self, options, config_path ):
		self.config_path = os.path.normpath( config_path )
		self.config = spectrumconfigparser.SpectrumConfigParser()
		self.options = options
		self.config.read( config_path )
		self.pid_file = self.config.get( 'service', 'pid_file' )

	def get_jid( self ):
		"""
		Get the jid of this service
		"""
		return self.config.get( 'service', 'jid' )

	def enabled( self ):
		return self.config.getboolean( 'service', 'enable' )

	def get_pid( self ):
		"""
		Get the pid of the service, returns -1 if pid does not exist or
		is unparsable.
		"""
		try:
			return int( open( self.pid_file ).readlines()[0].strip() )
		except:
			return -1

	def su_cmd( self, cmd ):
		user = self.options.su
		return [ 'su', user, '-s', '/bin/bash', '-c', ' '.join( cmd ) ]

	def check_environment( self ):
		# we must be root
		if os.getuid() != 0:
			raise RuntimeError( "User", "Must be root to start spectrum" )

		# check if spectrum user exists:
		if os.name == 'posix':
			import pwd
			try:
				user = pwd.getpwnam( self.options.su )
			except KeyError:
				raise RuntimeError( self.options.su, 'User does not exist' )

		def check_dir( dir ):
			if os.path.exists( dir ) and os.path.isdir( dir ):
				stat = os.stat( dir )
				if stat.st_uid != user.pw_uid or stat.st_gid != user.pw_gid:
					raise RuntimeError( dir, 'Unsafe ownership' )
			elif not os.path.exists( dir ):
				os.makedirs( dir )
				os.chown( dir, user.pw_uid, user.pw_gid )
			else:
				raise RuntimeError( dir, "Not a directory" )

		
		check_dir( os.path.dirname( self.pid_file ) )

		log_file = self.config.get( 'logging', 'log_file' )
		check_dir( os.path.dirname( log_file ) )

		cache_file = self.config.get( 'service', 'filetransfer_cache' )
		check_dir( os.path.dirname( cache_file ) )

		purple_dir = self.config.get( 'purple', 'userdir' )
		check_dir( purple_dir )

		if self.config.get( 'database', 'type' ) == 'sqlite':
			db_file = self.config.get( 'logging', 'log_file' )
			check_dir( os.path.dirname( db_path ) )

		return 0, 'ok'

	def list( self ):
		pid = self.get_pid()
		if pid == -1:
			pid = '-'
		proto = self.config.get( 'service', 'protocol' )
		host = self.get_jid()
		status = self.status()[1]
		return (pid, proto, host, status)

	def status( self, pid=None ):
		"""
		Determines if the instance is running.

		This method uses the /proc filesystem if it exists and tries
		to send signal 0 if not.

		@return: (int, string) where int is the exit-code and string is a status message.
		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
		"""
		if not os.path.exists( self.pid_file ):
			return 3, "not running."

		if pid == None:
			pid = self.get_pid()
			if pid <= 0:
				return 4, "could not parse pid file."
		
		if os.path.exists( '/proc' ):
			if os.path.exists( '/proc/%i' %(pid) ):
				return 0, "running."
			else:
				return 1, "not running but pid file exists."
		else: # no proc!
			try:
				os.kill( pid, 0 )
				return 0, "running."
			except OSError, err:
			# except OSError as err: #python3
				if err.errno == errno.ESRCH:
					return 1, "not running but pid file exists."
				else:
					return 4, "unknown."
			except:
				return 4, "unknown."


	def start( self ):
		"""
		Starts the instance.

		@return: (int, string) where int is the exit-code and string is a status message.
		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
		"""
		status = self.status()[0]
		if status == 0:
			return 0, "already running." # starting while we are already running is also success!
		elif status == 1:
			os.remove( self.pid_file )
		elif status != 3:
			return 1, "status unknown." # We cannot start if status != 3

		try:
			self.check_environment()
		except RuntimeError, e:
			return 1, "%s: %s" % e.args

		cmd = [ 'spectrum', self.config_path ]
		cmd = self.su_cmd( cmd )
		retVal = subprocess.call( cmd )
		if retVal != 0:
			return 1, "could not start spectrum instance."

		return 0, "ok."

	def stop( self ):
		"""
		Stops the instance (sends SIGTERM).

		@return: (int, string) where int is the exit-code and string is a status message.
		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
		"""
		status = self.status()[0]
		if status == 3:
			# stopping while not running is also success!
			return 0, "already stopped."
		elif status == 1:
			os.remove( self.pid_file )
			return 0, "pid file was still there"
		elif status != 0: 
			# we cannot stop if status != 0
			return 1, "status unknown." 

		pid = self.get_pid()
		try:
			os.kill( pid, signal.SIGTERM )
			time.sleep( 0.1 )
			for i in range(1, 10):
				status = self.status()[0]
				if status == 3 or status == 1:
					os.remove( self.pid_file )
					return 0, "ok."
				time.sleep( 1 )
				os.kill( pid, signal.SIGTERM )
		except:	
			return 1, "failed."

	def restart( self ):
		"""
		Restarts the instance (kills the process and starts it again).
		
		@return: (int, string) where int is the exit-code and string is a status message.
		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
		"""
		if self.stop()[0] == 0:
			return self.start()
		else:
			return 1, "could not stop spectrum instance."

	def reload( self ): # send SIGHUP to process
		"""
		Reload instance (send SIGHUP) which causes spectrum to reopen
		log-files etc.
		
		@return: (int, string) where int is the exit-code and string is a status message.
		"""
		if self.status()[0] != 0:
			return 1, "not running."
		pid = self.get_pid()
		try:
			os.kill( pid, signal.SIGHUP )
			return 0, "ok."
		except:	
			return 1, "failed."
