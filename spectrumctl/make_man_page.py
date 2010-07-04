#!/usr/bin/python

from spectrum import *


group = spectrum_group.spectrum_group( None, False )

f = open( '../man/spectrumctl.8.head' )
head = f.read()
print( head )

for cmd in dochelp.cmds:
	group._manpage_help( cmd )

f = open( '../man/spectrumctl.8.tail' )
tail = f.read()
print( tail )
