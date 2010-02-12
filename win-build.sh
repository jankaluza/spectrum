mkdir -p win-deps
cd win-deps

### PIDGIN-FETCH
if [ ! -d pidgin-2.6.5 ]
then
    wget -c http://gaim-extprefs.sf.net/winpidgin-build-fetcher.sh
    sh winpidgin-build-fetcher.sh
    if [ "$?" -ne 0 ]
    then
        exit
    fi
fi

### OLDER GCC - if needed

if [ "$1" == "win2k" ]
then
    cd win32-dev
    cd mingw
    wget -O gcc.tar.gz -c http://sourceforge.net/projects/mingw/files/GCC%20Version%203/Previous%20Release_%20gcc-3.4.2/gcc-core-3.4.2-20040916-1.tar.gz/download
    tar -xzf gcc.tar.gz
    cd ..
    cd ..
    
fi
### WIN32-MINGW
cd win32-dev
cd mingw
wget -O w32.tar.gz -c http://sourceforge.net/projects/mingw/files/MinGW%20API%20for%20MS-Windows/w32api-3.14/w32api-3.14-mingw32-dev.tar.gz/download
tar -xzf w32.tar.gz
cd ..
cd ..

### LIBPURPLE
cd pidgin-2.6.5
cd libpurple
if [ ! -e libpurple.dll ]
then
    make -f Makefile.mingw
    if [ "$?" -ne 0 ]
    then
        exit
    fi
    make -f Makefile.mingw install
fi
cd ..
cd ..

### G++
echo "downloading g++"
cd win32-dev
cd mingw
if [ "$1" == "win2k" ]
then
    wget -O g++.tar.gz -c http://sourceforge.net/projects/mingw/files/GCC%20Version%203/Previous%20Release_%20gcc-3.4.2/gcc-g%2B%2B-3.4.2-20040916-1.tar.gz/download
else
    wget -O g++.tar.gz -c http://downloads.sourceforge.net/project/mingw/GCC%20Version%203/Current%20Release_%20gcc-3.4.5-20060117-3/gcc-g%2B%2B-3.4.5-20060117-3.tar.gz?use_mirror=surfnet
fi
echo "unpacking g++"
tar -xzf g++.tar.gz
cd ..
cd ..

### GLOOX
echo "downloading gloox"
wget -c http://camaya.net/download/gloox-1.0.tar.bz2
if [ ! -d gloox ]
then
    tar -xjf gloox-1.0.tar.bz2
    mv gloox-1.0 gloox
fi
cd gloox
if [ ! -e src/.libs/cyggloox-8.dll ]
then
    rm -rf src/.deps
    ./configure --with-schannel --without-examples --without-tests
    if [ "$?" -ne 0 ]
    then
	exit
    fi
    #make clean
    make
    if [ "$?" -ne 0 ]
    then
	exit
    fi
fi
cd ..

### POCO
echo "downloading poco"
wget -c http://downloads.sourceforge.net/project/poco/sources/poco-1.3.5/poco-1.3.5-all.tar.gz?use_mirror=dfn
if [ ! -d poco-1.3.5-all ]
then
    echo "unpacking poco"
    tar -xzf poco-1.3.5-all.tar.gz
fi
cd poco-1.3.5-all
if [ ! -e Data/lib/MinGW/ia32/libPocoSQLite.a ]
then
    ./configure --config=MinGW --no-tests --no-samples
    echo "patching"
    sed -i 's/LIBPATH  = $(POCO_BUILD)\/$(LIBDIR)/LIBPATH  = ..\/$(LIBDIR)/g' build/rules/global
    sed -i 's/OBJPATH  = $(POCO_BUILD)\/$(COMPONENT)\/$(OBJDIR)/OBJPATH  = $(OBJDIR)/g' build/rules/global
    sed -i 's/INCLUDE = $(foreach p,$(POCO_ADD_INCLUDE),-I$(p)) -Iinclude $(foreach p,$(COMPONENTS),-I$(POCO_BASE)\/$(p)\/$(INCDIR))/INCLUDE = $(foreach p,$(POCO_ADD_INCLUDE),-I$(p)) -Iinclude $(foreach p,$(COMPONENTS),-I..\/$(p)\/$(INCDIR)) $(foreach p,$(COMPONENTS),-I..\/..\/$(p)\/$(INCDIR))/g' build/rules/global
    sed -i 's/libexecs: $(filter-out $(foreach f,$(OMIT),$f%),$(libexecs))/libexecs: Foundation-libexec Data-libexec Data\/SQLite-libexec/g' Makefile
    sed -i 's/all: libexecs tests samples/all: libexecs/g' Makefile
    echo "compiling"
    sleep 3
    make
    if [ "$?" -ne 0 ]
    then
	exit
    fi
fi
cd ..

### GLib Binaries
wget -O glib.zip -c http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.22/glib_2.22.4-1_win32.zip
unzip -n glib.zip
cd ..

### SPECTRUM
echo "copying gloox headers"
cp win-deps/gloox/src/*.h win-deps/gloox
echo "aclocal"
aclocal
echo "automake"
automake --add-missing
echo "autoconf"
autoconf
echo "removing .deps"
rm -rf src/.deps
./configure
#make clean
make
if [ "$?" -ne 0 ]
then
    exit
fi

### SPECTRUM INSTALL
echo "copying libraries into bin/"
mkdir -p bin
cp src/spectrum.exe bin/spectrum.exe
strip bin/spectrum.exe
cp spectrum.cfg.win bin/spectrum.cfg
cp -r win-deps/pidgin-2.6.5/win32-install-dir/* bin/
cp win-deps/bin/*.dll bin/
cp nsis/*.bat bin/
cp win-deps/gtk_installer/gtk_install_files/bin/intl.dll bin/
cp win-deps/gtk_installer/gtk_install_files/bin/iconv.dll bin/
cp win-deps/gtk_installer/gtk_install_files/bin/asprintf.dll bin/
cp win-deps/gtk_installer/gtk_install_files/bin/libexpat.dll bin/
cp win-deps/gtk_installer/gtk_install_files/bin/zlib1.dll bin/
cp win-deps/gtk_installer/gtk_install_files/bin/xmlparse.dll bin/
cp win-deps/gtk_installer/gtk_install_files/bin/xmltok.dll bin/
cp win-deps/gtk_installer/gtk_install_files/bin/charset.dll bin/
cp win-deps/gtk_installer/gtk_install_files/bin/libatk-1.0-0.dll bin/
cp win-deps/win32-dev/libxml2-2.6.30/bin/libxml2.dll bin/
cp win-deps/gloox/src/.libs/cyggloox-8.dll bin/
strip bin/cyggloox-8.dll

### Package
rm -rf spectrum-git
mv bin spectrum-git
echo "creating spectrum-git.tar.gz"
tar -czf spectrum-git.tar.gz spectrum-git
echo "creating spectrum-git.tar.bz2"
tar -cjf spectrum-git.tar.bz2 spectrum-git

## NSIS installer
cd nsis
makensis spectrum.nsi
cd ..
