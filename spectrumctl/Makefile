NAME="xmppbot"
MODULES=spectrum
HTML_DIR="epydoc/html"
HTML=${HTML_DIR}/index.html
PDF_DIR="epydoc/pdf"

doc: epydoc man

man: 
	./make_man_page.py > ../man/spectrumctl.8

epydoc: ${PDF_DIR} ${HTML}
	mkdir -p epydoc

${PDF_DIR}:
	mkdir -p ${PDF_DIR}

${HTML_DIR}:
	mkdir -p ${HTML_DIR}

${HTML}: ${HTML_DIR}
	epydoc -v --html -o ${HTML_DIR} --name ${NAME} --redundant-details --no-private ${MODULES}

check:
	epydoc --no-private --no-imports --check ${MODULES}

clean:
	-rm -rf ${HTML_DIR} ${PDF_DIR}

