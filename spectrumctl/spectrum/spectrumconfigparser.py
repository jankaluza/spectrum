# -*- coding: utf-8 -*-
#
# This file is part of spectrumctl. See spectrumctl.py for a description.
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

try:
	import ConfigParser
except ImportError:
	import configparser as ConfigParser

class SpectrumConfigParser( ConfigParser.ConfigParser ):
	def _interpolate( self, section, option, rawval, vars ):
		value = rawval
		if value == "password":
			return value # not expanding password!

		depth = ConfigParser.MAX_INTERPOLATION_DEPTH
		while depth:
#			if '$'+option in value:
#				print( "Warning: Option %s contains itself: %s"%(option, value) )
#				break # we cannot contain ourself - stop!

			if not '$jid' in value and not '$protocol' in value:
				break # nothing left to expand
			
			if "$jid" in value:
				jid = self.get( 'service', 'jid', True )
				value = value.replace( '$jid', jid )
			if "$protocol" in value:
				protocol = self.get( 'service', 'protocol', True )
				value = value.replace( '$protocol', protocol )
			depth -= 1

		return value
	def _interpolation_replace( self ):
		print( "Please file a bug-report when you read this line!" )
