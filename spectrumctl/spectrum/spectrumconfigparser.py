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

try:
	import ConfigParser
except ImportError:
	import configparser as ConfigParser

class SpectrumConfigParser( ConfigParser.ConfigParser ):
	variables = [ 'jid', 'protocol', 'filename' ]

	def _interpolate( self, section, option, rawval, vars ):
		value = rawval
		if value == "password":
			return value # not expanding password!

		depth = ConfigParser.MAX_INTERPOLATION_DEPTH
		while depth:
			variables = [ v for v in SpectrumConfigParser.variables if '$'+v in value ]
			if variables == []:
				break # nothing left to expand
			
			for var in variables:
				subst = self.get( 'service', var, True )
				value = value.replace( '$'+var, subst )
			depth -= 1

		return value

	def _interpolation_replace( self ):
		print( "Please file a bug-report when you read this line!" )
