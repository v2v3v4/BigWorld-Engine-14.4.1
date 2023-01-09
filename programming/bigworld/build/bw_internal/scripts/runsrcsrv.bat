@rem This runs Microsoft's Source Indexing scripts to write pdb source
@rem paths to refer to the appropriate file in Perforce.
set PATH=%PATH%;%PROGRAMFILES(X86)%\Windows Kits\8.1\Debuggers\x64\srcsrv
set SRCSRV_INI=\\au1-mss\bwsymbols\srcsrv.ini
set SRCSRV_SOURCE=..\..\..\bigworld\src
set SRCSRV_SYMBOLS=..\..\..\bigworld\bin

call p4index /debug
