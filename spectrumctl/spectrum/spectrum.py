import os, signal, subprocess
try:
	from spectrum import spectrumconfigparser
except ImportError:
	import spectrumconfigparser

class spectrum():
	def __init__( self, config_path ):
		self.config_path = config_path
		self.config = spectrumconfigparser.SpectrumConfigParser()
		self.config.read( config_path )
		self.pid_file = self.config.get( 'service', 'pid_file' )

	def get_jid( self ):
		return self.config.get( 'service', 'jid' )

	def get_pid( self ):
		try:
			return int( open( self.pid_file ).readlines()[0].strip() )
		except:
			return -1

	def status( self ):
		"""
		Determines if the process is running.
		@return: Integer, see LSB-specs for further details.

		@see:	http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
		"""
		if not os.path.exists( self.pid_file ):
			return 3, "not running."

		pid = self.get_pid()
		if pid <= 0:
			return 4, "could not parse pid file."

		# we use the proc-filesystem if it exists:
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
					return 3, "not running."
				else:
					return 4, "unknown."
			except:
				return 4, "unknown."

	def start( self ):
		status = self.status()
		if status == 0:
			return 0, "already running." # starting while we are already running is also success!
		elif status != 3:
			return 1, "status unknown." # We cannot start if status != 3

		cmd = [ 'spectrum', '-c', self.config ]
		retVal = subprocess.call( cmd )
		if retVal != 0:
			return 1, "could not start spectrum instance."

		return 0, "ok."

	def stop( self ):
		status = self.status()
		if status == 3:
			# stopping while not running is also success!
			return 0, "already stopped."
		elif status != 0: 
			# we cannot stop if status != 0
			return 1, "status unknown." 

		pid = self.get_pid()
		try:
			os.kill( pid, signal.SIGTERM )
			return 0, "ok."
		except:	
			return 1, "failed."

	def restart( self ):
		if self.stop() == 0:
			return self.start()
		else:
			return 1, "could not stop spectrum instance."

	def reload( self ): # send SIGHUP to process
		if self.status() != 0:
			return 1, "not running."
		pid = self.get_pid()
		try:
			os.kill( pid, signal.SIGHUP )
			return 0, "ok."
		except:	
			return 1, "failed."
