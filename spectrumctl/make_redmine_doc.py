#!/usr/bin/python

import sys, os

pathname = os.path.dirname(sys.argv[0])
abspath = os.path.abspath( pathname )
sys.path.append( abspath )

from spectrum import spectrum_group
from spectrum.doc import redmine, doc

cl = spectrum_group.spectrum_group
backend = redmine.doc( cl )

for cmd in doc.cmds:
	print('')
	print( backend.create_documentation( cmd ) )
