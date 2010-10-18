#!/usr/bin/env python

import os, sys, tempfile
from optparse import OptionParser
from subprocess import *
from spectrum import spectrum

# Parse options
parser = OptionParser()
parser.add_option( '-j', '--jid', help="Get a backtrace on JID." )
parser.add_option( '-c', '--config', metavar='FILE', 
	help="Get a backtrace on instance configured by FILE" )
parser.add_option( '-d', '--output-dir', metavar='DIR',
	help="Put output files in DIR. [default: current working directory]" )
parser.add_option( '--core-dump', metavar='FILE',
	help="Override location of coredump file." )
options, args = parser.parse_args()

# parse options, do sanity checks:
if options.jid and options.config:
	print( "Error: Options --jid and --config are mutually exclusive." )
	sys.exit(1)
if not options.jid and not options.config:
	print( "Error: Please name either --config or --jid." )
	sys.exit(1)
if not options.output_dir:
	options.output_dir = os.getcwd()

# find instance
if options.config:
	instance = spectrum.spectrum( options.config )
if options.jid:
	print( "Kaboom: Not yet implemented." )

# check that core-dump exists
if not options.core_dump:
	userdir = instance.config.get( 'purple', 'userdir' )
	options.core_dump = os.path.normpath( userdir + '/core' )
if not os.path.exists( options.core_dump ):
	print( "Error: %s: Could not find coredump."%(options.core_dump) )
	sys.exit(1)

# create core dump:
core_output_path = '%s/%s.trace'%(options.output_dir, instance.get_jid() )
core_output_file = open( core_output_path, 'w' )
cmd = ['gdb', 'spectrum', options.core_dump]
p = Popen( cmd, stdin=PIPE, stderr=STDOUT, stdout=core_output_file )
p.stdin.write( 'bt full\n' )
p.stdin.write( 'quit\n' )
p.wait()
p.stdin.close()
print( "Coredump written to %s"%(core_output_path) )

# tail logfile
#TODO

# print version
#TODO
