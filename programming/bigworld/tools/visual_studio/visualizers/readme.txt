Debug Visualizers for Bigworld types.

OVERVIEW:
	Natvis files to define custom visualizers for bigworld types.
	This allows data structures to be evaluated and displayed easily and conveniently in the debugger.
	The initial use case is to automatically translate SceneObject runtime typeID's into actual typeNames
	when a scene object is viewed in the debugger.

see: http://msdn.microsoft.com/en-us/library/vstudio/jj620914(v=vs.110).aspx#BKMK_Using_Natvis_files

INSTALLATION:
	Copy paste the *.natvis files into your:
	%USERPROFILE%\My Documents\Visual Studio 2012\Visualizers\
	directory. No restarts are required, just restart your debugging session if you are already in one.