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

import os, pwd, stat, time, signal, subprocess
import env
try:
	from spectrum import spectrumconfigparser, ExistsError
except ImportError:
	import spectrumconfigparser, ExistsError

ExistsError = ExistsError.ExistsError

class spectrum:
	def __init__( self, config_path ):
		self.config_path = os.path.normpath( config_path )
		
		self.config = spectrumconfigparser.SpectrumConfigParser()
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

	def check_environment( self ):
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

		# do rigorous permission-checking:
		try:
			self.check_environment()
		except RuntimeError, e:
			return 1, "%s: %s" % e.args

		try:
			binary = os.environ['SPECTRUM_PATH']
		except KeyError:
			binary = 'spectrum'

		cmd = [ binary ]

		# get the absolute path of the config file, so we can change to
		# spectrums homedir
		path = os.path.abspath( self.config_path )
		home = pwd.getpwuid( env.get_uid() )[5]
		os.chdir( home )

		# check if database is at current version:
		check_cmd = [ 'spectrum', '--check-db-version', path ]
		check_cmd = env.su_cmd( check_cmd )
		retVal = subprocess.call( check_cmd )
		if retVal == 1:
			return 1, "db_version table does not exist"
		elif retVal == 2: 
			return 1, "database not up to date, update with spectrum --upgrade-db"
		elif retVal == 3:
			return 1, "Error connecting to the database"

		# finally start spectrum:
		if env.options.no_daemon:
			cmd.append( '-n' )
		if env.options.debug:
			os.environ['MALLOC_PERTURB_'] = '254'
			os.environ['PURPLE_VERBOSE_DEBUG'] = '1'
			cmd[0] = 'ulimit -c unlimited; ' + cmd[0]
		cmd.append( path )
		cmd = env.su_cmd( cmd )
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
			return 1, "failed."
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
