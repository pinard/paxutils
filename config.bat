@echo off
echo Configuring GNU Tar for DJGPP v2.x...

Rem Find out where the sources are
set XSRC=.
if not "%XSRC%" == "." goto SmallEnv
if "%1" == "" goto InPlace
set XSRC=%1
if not "%XSRC%" == "%1" goto SmallEnv
redir -e /dev/null update %XSRC%/configure.orig ./configure
if not exist configure update %XSRC%/configure ./configure

:InPlace
Rem Update configuration files
echo Updating configuration scripts...
if not exist configure.orig update configure configure.orig
patch -p0 -i %XSRC%/contrib/cfgdjgpp.pat -o ./configure configure.orig

Rem `configure' won't run correctly without these variables
set PATH_SEPARATOR=:
if not "%PATH_SEPARATOR%" == ":" goto SmallEnv
set PATH_EXPAND=y
if not "%PATH_EXPAND%" == "y" goto SmallEnv
set INSTALL=${DJDIR}/bin/ginstall -c
if not "%INSTALL%" == "${DJDIR}/bin/ginstall -c" goto SmallEnv
set CONFIG_SHELL=sh
if not "%CONFIG_SHELL%" == "sh" goto SmallEnv

echo Running ./configure...
sh %XSRC%/configure --src=%XSRC% --host=i386-pc-msdosdjgpp --disable-nls

echo Done.
goto End

Rem Protect them against too small environment size
:SmallEnv
echo Your environment size is too small.  Enlarge it and run me again.

:End
set INSTALL=
set CONFIG_SHELL=
