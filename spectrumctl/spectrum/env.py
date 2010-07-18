import os, grp, pwd, stat

try:
        from spectrum import spectrumconfigparser, ExistsError
except ImportError:
        import spectrumconfigparser, ExistsError

ExistsError = ExistsError.ExistsError
options = None

def drop_privs( uid, gid ):
	"""
	Set the process real and effictive userid
	"""
	os.setgid( gid )
	os.setuid( uid )
	os.setegid( gid )
	os.seteuid( uid )

def get_uid():
	# if we explicitly name something on the CLI, we use that:
	if options and options.su:
		try:
			return pwd.getpwnam( options.su ).pw_uid
		except KeyError:
			raise RuntimeError( options.su, "Username does not exist" )

	try:
		# if we have env-variable set, we use that:
		username = os.environ['SPECTRUM_USER']
	except KeyError:
		# otherwise we default to spectrum:
		username = 'spectrum'
	try:
		return pwd.getpwnam( username ).pw_uid
	except KeyError:
		raise RuntimeError( username, "Username does not exist" )

def get_gid():
	return pwd.getpwuid( get_uid() ).pw_gid

def su_cmd( cmd ):
	"""
	TODO: This should be replaced with "drop_privs" or so
	"""
	user = pwd.getpwuid( get_uid() ).pw_name
	return [ 'su', user, '-s', '/bin/sh', '-c', ' '.join( cmd ) ]

def perm_translate( perm ):
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

def chmod_translate( add, rem ):
	changes = []

	for who, what in add.iteritems():
		if what != []:
			changes.append( who + '+' + ''.join( what ) )
	for who, what in rem.iteritems():
		if what != []:
			changes.append( who + '-' + ''.join( what ) )
	
	return ','.join( changes )

def check_ownership( node, uid=None, gid=None ):
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
		uid = get_uid()
	if gid == None:
		gid = get_gid()

	stat_result = os.stat( node )
	if uid != -1 and stat_result.st_uid != uid:
		name = pwd.getpwuid( uid ).pw_name
		raise RuntimeError( node, 'Unsafe ownership (fix with "chown %s %s")'%(name, node) )
	if gid != -1 and stat_result.st_gid != gid:
		name = grp.getgrgid( gid ).gr_name
		raise RuntimeError( node, 'Unsafe ownership (fix with "chgrp %s %s")'%(name, node) )

def check_exists( node, typ='file' ):
	if not os.path.exists( node ):
		raise ExistsError( node, 'Does not exist' )

	if typ == 'file' and not os.path.isfile( node ):
		raise RuntimeError( node, 'Not a file' )
	if typ == 'dir' and not os.path.isdir( node ):
		raise RuntimeError( node, 'Not a directory' )

	return True

def is_named_pipe( node ):
	if os.path.exists( node ):
		mode = os.stat( node )[stat.ST_MODE]
		if stat.S_ISFIFO( mode ):
			return True
	return False

def check_permissions( file, permissions, wildcards=[] ):
	all = [ stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR,
		stat.S_IRGRP, stat.S_IWGRP, stat.S_IXGRP,
		stat.S_IROTH, stat.S_IWOTH, stat.S_IXOTH ]
	mode = os.stat( file )[stat.ST_MODE]
	add = { 'u': [], 'g': [], 'o': [] }
	rem = { 'u': [], 'g': [], 'o': [] }

	for p in permissions:
		if (mode & p) == 0:
			who, what = perm_translate( p )
			add[who].append( what )

	for p in [ p for p in all if ( p not in permissions and p not in wildcards )]:
		if (mode & p) != 0:
			who, what = perm_translate( p )
			rem[who].append( what )

	mode_string = chmod_translate( add, rem )
	if mode_string:
		string = 'chmod ' + mode_string + ' ' + file
		raise RuntimeError( file, 'Incorrect permissions (fix with "%s")' %(string) )

def check_writable( node, uid=None ):
	if uid == None:
		user = pwd.getpwuid( get_uid() )
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

def create_dir( dir ):
	if not dir: # this is the end of a recursion with a relative path
		return
	dirname = os.path.dirname( dir )
	if not os.path.exists( dirname ):
		create_dir( dirname )
	os.mkdir( dir )
	os.chown( dir, get_uid(), get_gid() )
	os.chmod( dir, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IXGRP )
