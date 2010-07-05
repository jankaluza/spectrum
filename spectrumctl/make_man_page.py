#!/usr/bin/python

from spectrum import *


group = spectrum_group.spectrum_group( None, False )

print( """.\"
.\"     Title: spectrumctl
.\"    Author: Moritz Wilhelmy <crap@wzff.de>
.\"  Language: English
.\"      Date: 2010-02-21
.\" This document is the result of painful hand work. I still like writing manpages more than html :)
.\"
.TH spectrumctl 8  "February 21, 2010" "Version 0.1\-git" "Spectrum Manual"
.SH NAME
spectrumctl \- Utility to control one or more spectrum instances
.SH SYNOPSIS
.B spectrumctl
[options] action
.SH DESCRIPTION
The purpose of the \\fBspectrumctl\\fR utility is to control one or more Spectrum
instances. Spectrum only handles one legacy network per instance, so you need
more than one spectrum instance if you want support for more than one legacy
network. The tool offers standard tasks usually performed by
initscripts but also enables some runtime configuration of spectrum instances.
.sp
An interactive shell with comfortable tab-completion is also supported to help
with more complex administration tasks. See the \\fBSHELL\\fR for a detailed
explanation.
.SH ACTIONS
.sp
By default, spectrumctl acts on all transports defined in \\fI/etc/spectrum/\\fR.
You can use a different directory with the \\fI--config-dir\\fR option. If you
just want to act on a single file, you can use the \\fI--config\\fR option. Unless
you use \\fI--config\\fR, spectrumctl will silently ignore files where the
filename does not end with \\fB.cfg\\fR.""" )

for cmd in dochelp.cmds:
	group._manpage_help( cmd )

print( """.SH OPTIONS
.RE
\\fB\-c\\fR \\fIFILE\\fR, \\fB\-\-config\\fR=\\fIFILE\\fR
.RS 4
Only act on transport configured in FILE (ignored for list)
.sp
.RE
\\fB\-d\\fR \\fIDIR\\fR, \\fB\-\-config\-dir\\fR=\\fIDIR\\fR
.RS 4
Act on all transports configured in DIR (default: \\fI/etc/spectrum\\fR)
.sp
.RE
\\fB\-q\\fR, \\fB\-\-quiet\\fR
.RS 4
Do not print any output (ignored for list)
.sp
.RE
\\fB\-h\\fR, \\fB\-\-help\\fR
.RS 4
show a help message and exit
.sp
.RE
\\fB\-\-version\\fR
.RS 4
show program's version number and exit
.RE
.sp
.RE
Options for action "\\fBlist\\fR":
.sp
.RS 4
\\fB\-\-status\\fR=\\fISTATUS\\fR
.RS 4
Only show transports with given status. Valid values are "\\fIrunning\\fR", "\\fIstopped\\fR" and "\\fIunknown\\fR"
.sp
.RE
\\fB\-\-machine-readable\\fR
.RS 4
Output data as comma-seperated values.
.sp
.RE
.RE
Options for action "\\fBstart\\fR":
.sp
.RS 4
\\fB\-\-su\\fR=\\fISU\\fR
.RS 4
Start spectrum as this user (default: \\fIspectrum\\fR). Overrides the SPECTRUM_USER environment variable.
If neither --su nor SPECTRUM_USER is specified, the user defaults to 
\\fIspectrum\\fR.
.RE
.sp
\\fB\-\-no-daemon\\fR
.RS 4
Do not start spectrum as daemon.
.sp
.RE
\\fB\-\-debug\\fR
.RS 4
Start spectrum in debug mode. Currently this just sets the maximum size of the
core files to unlimited.
.RE
.SH SHELL
This tool also sports a fancy interactive shell that you can use to perform
multiple administration tasks at once. You can launch the shell with the
\\fBshell\\fR action. The shell features tab-completion for commands and a few
commands only available within the shell. 
.sp
The prompt of the spectrumctl shell represents the transports that commands will
act upon. If the shell acts upon all config-files defined in \\fI/etc/spectrum\\fR,
the prompt will show \\fB<all transports>\\fR and the JID of the current transport
otherwise.
.sp
In addition to the actions defined above, spectrumctl also supports the
following commands:""" )

for cmd in dochelp.shell_cmds:
	group._manpage_help( cmd )

print( """.RE
.SH ENVIRONMENT
The behaviour of spectrumctl can be influenced by the following environment variables:
.sp
\\fBSPECTRUM_PATH\\fR
.RS 4
Path where the spectrum binary is located. If omitted, spectrum is assumed to be in your PATH.
.RE
.sp
\\fBSPECTRUM_USER\\fR
.RS 4
The user with which spectrum is started. Overridden by the --su command line
option. 
If neither --su nor SPECTRUM_USER is specified, the user defaults to 
\\fIspectrum\\fR.
.RE
.SH AUTHORS
Copyright \(co 2009\-2010 by Spectrum engineers:
.sp
.\" template start
.RS 4
.ie n \{\
\h'-04'\(bu\h'+03'\c
.\}
.el \{\
.sp -1
.IP \(bu 2.3
.\}
Jan Kaluza <hanzz@soc.pidgin\&.im>
.RE
.\" template end, and once again template start
.RS 4
.ie n \{\
\h'-04'\(bu\h'+03'\c
.\}
.el \{\
.sp -1
.IP \(bu 2.3
.\}
Mathias Ertl <mati@fsinf\&.at>
.RE
.\" template end ;)
.RS 4
.ie n \{\
\h'-04'\(bu\h'+03'\c
.\}
.el \{\
.sp -1
.IP \(bu 2.3
.\}
Paul Aurich <paul@darkrain42\&.org>
.RE
.\" again template end
.sp
.\" TODO: Contributors section. Contributors should add themselves
.br
License GPLv3+: GNU GPL version 3 or later.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
.sp
See http://gnu.org/licenses/gpl.html for more information.
.SH SEE ALSO
\\fBspectrum\\fP(1), \\fBspectrum.cfg\\fP(5)
.sp
For more information, see the spectrum homepage at http://spectrum.im/

.SH BUGS
Please submit bugs to our issue tracker at github: http://github.com/hanzz/spectrum/issues""" )
