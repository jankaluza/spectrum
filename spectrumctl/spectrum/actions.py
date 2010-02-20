def list( instances ):
	lines = [ ('PID', 'PROTOCOL', 'HOSTNAME', 'STATUS' ) ]

	for instance in instances:
		lines.append( instance.list() )

	widths = [0,0,0,0]
	for item in lines:
		for i in range(0,4):
			val = str( item[i] )
			if len( val ) > widths[i]:
				widths[i] = len( val )

	for item in lines:
		for i in range(0,4):
			val = str( item[i] )
			val += ' ' * (widths[i]-len(val)) + '  '
			print val,
		print
	return 0
