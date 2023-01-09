@echo off

@rem Set up MSVC environment for compiling with XP support
@rem See http://blogs.msdn.com/b/vcblog/archive/2012/10/08/windows-xp-targeting-with-c-in-visual-studio-2012.aspx

if not "%3" == "" goto usage

if /i %1 == vc10     goto vc10
if /i %1 == vc11     goto vc11
if /i %1 == vc12     goto vc12
goto usage

:vc10
set VisualStudioVersion=10.0
goto arch

:vc11
set VisualStudioVersion=11.0
goto arch

:vc12
set VisualStudioVersion=12.0
goto arch

:arch
@set VCBINDIR=%ProgramFiles(x86)%\Microsoft Visual Studio %VisualStudioVersion%\VC\bin\
@set WINSDKDIR=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\

if /i %2 == x86       goto x86
if /i %2 == amd64     goto amd64
if /i %2 == x64       goto amd64
goto usage

:x86
if not exist "%VCBINDIR%vcvars32.bat" goto missing
call "%VCBINDIR%vcvars32.bat"
@set LIB=%VCINSTALLDIR%lib;%VCINSTALLDIR%atlmfc\lib;%WINSDKDIR%Lib;
goto :setenv

:amd64
if not exist "%VCBINDIR%amd64\vcvars64.bat" goto missing
call "%VCBINDIR%amd64\vcvars64.bat"
@set LIB=%VCINSTALLDIR%lib\amd64;%VCINSTALLDIR%atlmfc\lib\amd64;%WINSDKDIR%Lib\x64;
goto :setenv

:setenv
@set INCLUDE=%VCINSTALLDIR%include;%VCINSTALLDIR%atlmfc\include;%WINSDKDIR%Include;
@set PATH=%WINSDKDIR%Bin;%PATH%
goto :eof

:usage
echo Error in script usage. The correct usage is:
echo     %0 [vcver] [arch]
echo where [vcver] is: vc10 ^| vc11 ^| vc12
echo and [arch] is: x86 ^| amd64 ^| x64
echo:
echo For example:
echo     %0 vc12 x86
goto :eof

:missing
echo The specified configuration type is missing.  The tools for the
echo configuration might not be installed.
goto :eof

