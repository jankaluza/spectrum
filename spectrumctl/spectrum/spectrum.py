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
	def __init__( self, config_path, params ):
		self.config_path = os.path.normpath( config_path )
		self.params = params
		
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
		status = self.status_str()
		return (pid, proto, host, status)

	def status_str( self, pid=None ):
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
		to send signal 0 if not.

		@return: (int, string) where int is the exit-code and string is a status message.
		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
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
		Starts the instance.

		@return: (int, string) where int is the exit-code and string is a status message.
		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
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
		process = subprocess.Popen( check_cmd, stdout=subprocess.PIPE )
		process.communicate()
		if process.returncode == 1:
			raise RuntimeError( "db_version table does not exist", 1)
		elif process.returncode == 2: 
			raise RuntimeError( "database not up to date, update with spectrumctl upgrade-db"%(self.config_path), 1 )
		elif process.returncode == 3:
			raise RuntimeError( "Error connecting to the database", 1 )

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
			raise RuntimeError( "Could not start spectrum instance", retVal )

		return

	def stop( self ):
		"""
		Stops the instance (sends SIGTERM).

		@return: (int, string) where int is the exit-code and string is a status message.
		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
		"""
		status = self.status()
		if status == 3:
			# stopping while not running is also success!
			return
		elif status == 1:
			os.remove( self.pid_file )
			return

		pid = self.get_pid()
		try:
			os.kill( pid, signal.SIGTERM )
			time.sleep( 0.1 )
			
			for i in range(1, 10):
				status = self.status()
				if status == 3 or status == 1:
					os.remove( self.pid_file )
					return 0
				time.sleep( 1 )
				os.kill( pid, signal.SIGTERM )
			raise RuntimeError( "Spectrum did not die", 1 )
		except:	
			raise RuntimeError( "Unknown Error occured", 1 )

	def restart( self ):
		"""
		Restarts the instance (kills the process and starts it again).
		
		@return: (int, string) where int is the exit-code and string is a status message.
		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
		"""
		self.stop()
		self.start()
	
	def reload( self ): # send SIGHUP to process
		"""
		Reload instance (send SIGHUP) which causes spectrum to reopen
		log-files etc.
		
		@return: (int, string) where int is the exit-code and string is a status message.
		"""
		if self.status() != 0:
			raise RuntimeError( "not running", 1 )

		pid = self.get_pid()
		try:
			os.kill( pid, signal.SIGHUP )
		except:	
			raise RuntimeError( "Unknown error occured", 1 )

	def message_all( self ):
		from xmpp.simplexml import Node
		from xmpp.protocol import NS_DATA
		try:
			from spectrum import config_interface
		except ImportError:
			import config_interface

		interface = config_interface.config_interface( self )

		# first field stanza:
		field_value = Node( tag='value', 
			payload=['ADHOC_ADMIN_SEND_MESSAGE'] )
		field = Node( tag='field', payload=[field_value],
			attrs={'type': 'hidden', 'var': 'adhoc_state'} )

		# x stanza with enclosed field:
		x_value = Node( tag='value', payload=['Awesome message'] )
		x_field = Node( tag='field', payload=[x_value],
			attrs={'type':'text-multi', 'var': 'message' } )
		x = Node( tag='x', payload=[x_field],
			attrs={ 'xmlns': NS_DATA, 'type': 'submit' } )
	
		response = interface.command( [field, x] )
		print( response )
		return 0
	
	def upgrade_db( self ):
		path = os.path.abspath( self.config_path )

		check_cmd = [ 'spectrum', '--check-db-version', path ]
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


	def set_vip_status( self ):
		from xmpp.simplexml import Node
		from xmpp.protocol import NS_DATA
		try:
			from spectrum import config_interface
		except ImportError:
			import config_interface
		
		interface = config_interface.config_interface( self )
		
		x = Node( tag='x', attrs={'xmlns': NS_DATA, 'type':'submit'} )

		fields = [
			('adhoc_state', 'ADHOC_ADMIN_USER2', 'hidden'),
			('user_jid', self.params[0], 'hidden' ),
			('user_vip', self.params[1], 'boolean' ) ]

		for field in fields:
			value = Node( tag='value', payload=[field[1]] )
			field = Node( tag='field', payload=[value],
				attrs={'type':field[2], 'var':field[0] } )
			x.addChild( node=field )

		response = interface.command( [x] )
		print( response )
		return 0

	def get_stats( self ):
		import xmpp
		try:
			from spectrum import config_interface
		except ImportError:
			import config_interface

		interface = config_interface.config_interface( self )
		ns = 'http://jabber.org/protocol/stats'
		nodes = []
		for name in [ 'uptime', 'users/registered', 'users/online', 
				'contacts/online', 'contacts/total', 
				'messages/in', 'messages/out', 'memory-usage' ]:
			nodes.append( xmpp.simplexml.Node( 'stat', 
				attrs={'name': name} ) )

		return interface.query( nodes, ns )
