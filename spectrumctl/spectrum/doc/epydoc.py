"""
Various functions related to epydoc documentation
"""

def get( func ):
	doc = ''
	for line in func.__doc__.splitlines():
		if line.strip().startswith( '@' ):
			break

		if line.strip() == '':
			doc += "\n"
		else:   
			doc += line.strip() + " "

	return doc.strip()
