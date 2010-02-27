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

import os, grp, pwd, stat, time, signal, subprocess
try:
	from spectrum import spectrumconfigparser
except ImportError:
	import spectrumconfigparser

class ExistsError( RuntimeError ):
	"""Thrown when a file or directory does not exist."""
	pass

class spectrum:
	def __init__( self, options, config_path ):
		self.options = options
		self.config_path = os.path.normpath( config_path )
		
		# check permissions for config-file:
		self.check_ownership( self.config_path, 0 )
		# we don't care about user permissions, group MUST be read-only
		self.check_permissions( self.config_path, [ stat.S_IRGRP ],
			[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR ] )

		filename = os.path.splitext( os.path.basename( self.config_path ) )[0]
		self.config = spectrumconfigparser.SpectrumConfigParser({
			'filename': filename,
			'filetransfer_web': '',
			'config_interface': '',
			'pid_file': '/var/run/spectrum/$jid',
			'language': 'en',
			'encoding': '',
			'log_file': '',
			'only_for_vip': 'false',
			'vip_mode': 'false',
			'use_proxy': 'false'
			})
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

	def check_exists( self, file, typ=file ):
		if not os.path.exists( file ):
			raise ExistsError( file, 'Does not exist' )

		if typ == 'file' and not os.path.isfile( file ):
			raise ExistsError( file, 'Not a file' )
		if typ == 'dir' and not os.path.isdir( file ):
			raise ExistsError( file, 'Not a directory' )

		return True

	def check_ownership( self, node, uid=None, gid=None ):
		"""
		Check the ownership and group ownership of the given filesystem
		node. If uid or gid is None, the uid (and its respective primary
		gid) is used. If you don't want to check uid or gid, just pass
		-1.
		"""

		if uid == -1 and gid == -1:
			print( "Warning: uid and gid are -1. That doesn't make sense." )

		if not os.path.exists( node ):
			raise RuntimeError( node, "Does not exist" )

		if uid == None:
			uid = pwd.getpwnam( self.options.su ).pw_uid
		if gid == None:
			gid = pwd.getpwnam( self.options.su ).pw_gid

		stat = os.stat( node )
		if uid != -1 and stat.st_uid != uid:
			name = pwd.getpwuid( uid ).pw_name
			raise RuntimeError( node, 'Unsafe ownership (fix with "chown %s %s")'%(name, node) )
		if gid != -1 and stat.st_gid != gid:
			name = grp.getgrgid( gid ).gr_name
			raise RuntimeError( node, 'Unsafe ownership (fix with "chgrp %s %s")'%(name, node) )
	
	def perm_translate( self, perm ):
		"""
		Transforms one of the modes defined in the stats module into a
		string tuple ('x', 'y') where 'x' is one of 'u', 'g' or 'o' and
		'y' is either 'r', 'w' or 'x'. This method can be used transform
		a permission into string that can be used with chmod.
		"""
		if perm == stat.S_IRUSR:
			return ('u', 'r')
		elif perm == stat.S_IWUSR:
			return ('u', 'w')
		elif perm == stat.S_IXUSR:
			return ('u', 'x')
		elif perm == stat.S_IRGRP:
			return ('g', 'r')
		elif perm == stat.S_IWGRP:
			return ('g', 'w')
		elif perm == stat.S_IXGRP:
			return ('g', 'x')
		elif perm == stat.S_IROTH:
			return ('o', 'r')
		elif perm == stat.S_IWOTH:
			return ('o', 'w')
		elif perm == stat.S_IXOTH:
			return ('o', 'x')

	def chmod_translate( self, add, rem ):
		changes = []

		for who, what in add.iteritems():
			if what != []:
				changes.append( who + '+' + ''.join( what ) )
		for who, what in rem.iteritems():
			if what != []:
				changes.append( who + '-' + ''.join( what ) )

		return ','.join( changes )
	
	def check_permissions( self, file, permissions, wildcards=[] ):
		all = [ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR,
			stat.S_IRGRP, stat.S_IWGRP, stat.S_IXGRP,
			stat.S_IROTH, stat.S_IWOTH, stat.S_IXOTH ]
		mode = os.stat( file )[stat.ST_MODE]
		add = { 'u': [], 'g': [], 'o': [] }
		rem = { 'u': [], 'g': [], 'o': [] }

		for p in permissions:
			if (mode & p) == 0:
				who, what = self.perm_translate( p )
				add[who].append( what )

		for p in [ p for p in all if ( p not in permissions and p not in wildcards )]:
			if (mode & p) != 0:
				who, what = self.perm_translate( p )
				rem[who].append( what )

		mode_string = self.chmod_translate( add, rem )
		if mode_string:
			string = 'chmod ' + mode_string + ' ' + file
			raise RuntimeError( file, 'Incorrect permissions (fix with "%s")' %(string) )

	def check_writable( self, node, uid=None ):
		if uid == None:
			user = pwd.getpwnam( self.options.su )
		else:
			user = pwd.getpwuid( uid )

		s = os.stat( node )
		groups = [g for g in grp.getgrall() if user.pw_name in g.gr_mem]
		groups.append( user.pw_gid )
		cmd = ''
		if s.st_uid == user.pw_uid:
			# user owns the file
			if s.st_mode & stat.S_IWUSR == 0:
				cmd = 'chmod u+w %s' %(node)
			if os.path.isdir( node ) and s.st_mode & stat.S_IXUSR == 0:
				cmd = 'chmod u+wx %s' %(node)
		elif s.st_gid in groups:
			if s.st_mode & stat.S_IWGRP == 0:
				cmd = 'chmod g+w %s' %(node)
			if os.path.isdir( node ) and s.st_mode & stat.S_IXGRP == 0:
				cmd = 'chmod g+wx %s' %(node)
		else: 
			if s.st_mode & stat.S_IWOTH == 0:
				cmd = 'chmod o+w %s' %(node)
			if os.path.isdir( node ) and s.st_mode & stat.S_IXOTH == 0:
				cmd = 'chmod o+wx %s' %(node)
		
		if cmd != '':
			raise RuntimeError( node, 'Not writable (fix with %s)' %(cmd) )
	
	def su_cmd( self, cmd ):
		user = self.options.su
		return [ 'su', user, '-s', '/bin/bash', '-c', ' '.join( cmd ) ]

	def check_environment( self ):
		# check if spectrum user exists:
		if os.name == 'posix':
			try:
				user = pwd.getpwnam( self.options.su )
			except KeyError:
				raise RuntimeError( self.options.su, 'User does not exist' )
		else:
			# on windows, there is no real way to get the info that
			# we need
			return 0, "ok"
		
		# we must be root
		if os.getuid() != 0:
			raise RuntimeError( "User", "Must be root to start spectrum" )

		# config_interface:
		config_interface = self.config.get( 'service', 'config_interface' )
		if config_interface:
			# on some old (pre 0.1) installations config_interface does not exist
			dir = os.path.dirname( config_interface )
			self.check_exists( dir, 'dir' )
			self.check_writable( dir )

		# filetransfer cache
		filetransfer_cache = self.config.get( 'service', 'filetransfer_cache' )
		try:
			self.check_exists( filetransfer_cache, 'dir' )
			self.check_ownership( filetransfer_cache )
			self.check_permissions( filetransfer_cache, # rwxr-x---
				[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR, 
				  stat.S_IRGRP, stat.S_IXGRP ] )
		except ExistsError:
			dir = os.path.dirname( filetransfer_cache )
			self.check_exists( dir )
			self.check_writable( dir )

		# pid_file:
		pid_file = self.config.get( 'service', 'pid_file' )
		dir = os.path.dirname( pid_file )
		self.check_exists( dir )
		self.check_writable( dir )

		# log file
		log_file = self.config.get( 'logging', 'log_file' )
		try:
			# does not exist upon first startup:
			self.check_exists( log_file )
			self.check_ownership( log_file, gid=-1 )
			self.check_permissions( log_file, # rw-r-----
				[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IRGRP ] )
		except ExistError:
			dir = os.path.dirname( log_file )
			self.check_exists( dir )
			self.check_writable( dir )

		# sqlite database
		if self.config.get( 'database', 'type' ) == 'sqlite':
			db_file = self.config.get( 'database', 'database' )
			try:
				# might not exist upon first startup:
				self.check_exists( db_file )
				self.check_ownership( db_file )
				self.check_permissions( db_file, # rw-r-----
					[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IRGRP ] )
			except ExistsError:
				dir = os.path.dirname( db_file )
				self.check_writable( dir )
		
		# purple userdir
		userdir = self.config.get( 'purple', 'userdir' )
		try:
			self.check_exists( userdir, 'dir' )
			self.check_ownership( userdir )
			self.check_permissions( userdir, # rwxr-x---
				[ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR, 
				  stat.S_IRGRP, stat.S_IXGRP ] )
		except ExistsError:
			dir = os.path.dirname( userdir )
			self.check_exists( dir )
			self.check_writable( dir )

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
