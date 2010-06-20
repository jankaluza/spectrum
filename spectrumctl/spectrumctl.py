#!/usr/bin/python -Wignore::DeprecationWarning
# -*- coding: utf-8 -*-
#
# spectrumctl can be used to control your spectrum-instances. Valid actions are
# start, stop, restart and reload. By default, spectrumctl acts on all instances
# defined in /etc/spectrum/
#
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

import os, sys
from spectrum import *
from optparse import *

description='''spectrumctl can be used to control your spectrum-instances.
Valid actions are start, stop, restart, reload, upgrade-db, set-vip-status, list and stats. By default, spectrumctl 
acts on all transports defined in /etc/spectrum/'''

parser = OptionParser( usage='Usage: %prog [options] action', version='0.2', description=description)
parser.add_option( '-c', '--config', metavar='FILE',
	help = 'Only act on transport configured in FILE (ignored for list)' )
parser.add_option( '-d', '--config-dir', metavar='DIR', default='/etc/spectrum',
	help = 'Act on all transports configured in DIR (default: %default)' )
parser.add_option( '-q', '--quiet', action='store_true', default=False,
	help = 'Do not print any output (ignored for list)' )

list_group = OptionGroup( parser, 'Options for action "list"' )
list_group.add_option( '--status', choices=['running', 'stopped', 'unknown'],
	help='Only show transports with given status. Valid values are "running", "stopped" and "unknown"' )
parser.add_option_group( list_group )

start_group = OptionGroup( parser, 'Options for action "start"' )
start_group.add_option( '--su', # NOTE: the default is set by 
	# spectrum.get_uid(). We need this so we can distinguish between
	# actually setting --su=spectrum and setting nothing at all.
	help = 'Start spectrum as this user (default: spectrum)' )
start_group.add_option( '--no-daemon', action='store_true', default=False,
	help = 'Do not start spectrum in daemon mode' )
start_group.add_option( '--debug', action='store_true', default=False,
	help = 'Start spectrum in debug mode.' )
parser.add_option_group( start_group )

list_group = OptionGroup( parser, 'Options for action "list"' )
list_group.add_option( '--machine-readable', action='store_true', default=False,
	help= 'Output data as comma-seperated values' )
parser.add_option_group( list_group )

options, args = parser.parse_args()
env.options = options

all_actions = [ x for x in dir( spectrum_group.spectrum_group ) if not x.startswith( '__' ) ]
if len( args ) == 0:
	print( "Please give an action (one of %s)" %(', '.join(all_actions) ) )
	sys.exit(1)

action = args[0].replace( '-', '_' )
params = args[1:]
if action == 'help':
	parser.print_help()
	sys.exit(0)
if action not in all_actions:
	print( "Error: %s: Unknown action." %(action) )
	sys.exit(1)

configs = []
if options.config:
	configs = [ options.config ]
else:
	if not os.path.exists( options.config_dir ):
		print( "Error: %s: No such directory"%(options.config_dir) )
		sys.exit(1)

	config_list = os.listdir( options.config_dir )
	instances = []
	for file in config_list:
		path = '%s/%s'%(options.config_dir, file)
		if os.path.isfile( path ) and path.endswith( '.cfg' ):
			configs.append( path )

group = spectrum_group.spectrum_group( options, params, configs )
getattr( group, action )()
