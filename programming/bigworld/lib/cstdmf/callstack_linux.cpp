#include "pch.hpp"

#include "callstack.hpp"
#include "string_builder.hpp"
#include "stdmf.hpp" // for NEW_LINE definition

#if BW_ENABLE_STACKTRACE

#include "dprintf.hpp"

#include <bfd.h>

#include <execinfo.h>
#include <string.h>
#include <cxxabi.h>


#define ATTEMPT_RESOLVE_INLINE

BW_BEGIN_NAMESPACE

namespace // anonymous
{
bool g_canResolveSymbols = false;

struct symbolFile
{
	bfd * abfd;
	asymbol ** syms;
};


symbolFile g_selfSymbols = { NULL, NULL };



struct symbolResult
{
	bfd_vma pc;
	asymbol ** syms;
	const char * filename;
	const char * functionname;
	unsigned int line;
	bfd_boolean found;
};


/**
 *
 */
void find_address_in_section( bfd *abfd, asection *section, void *data )
{
	bfd_vma vma;
	bfd_size_type size;
	symbolResult *psi = (symbolResult*)data; 

	if (psi->found)
	{
		return;
	}

	if ((bfd_get_section_flags( abfd, section ) & SEC_ALLOC) == 0)
	{
		return;
	}

	vma = bfd_get_section_vma( abfd, section );
	if (psi->pc < vma)
	{
		return;
	}

	size = bfd_get_section_size( section );
	if (psi->pc >= vma + size)
	{
		return;
	}

	psi->found = bfd_find_nearest_line( abfd, section, 
		psi->syms, psi->pc - vma,
		&psi->filename, &psi->functionname, 
		&psi->line );
}


/**
 *
 */
bool loadSymbolsFile( const char * filename, symbolFile * outSymbolFile )
{
	bfd * abfd = bfd_openr( filename, NULL );

	if (!abfd)
	{
		dprintf( "Can't open BFD file: %s\n", 
			bfd_errmsg( bfd_get_error() ) );
		return false;
	}
	
	if (bfd_check_format( abfd, bfd_archive ))
	{
		dprintf( "Can't get addresses, due to Invalid format\n" );
		bfd_close( abfd );
		return false;
	}
	
	char ** matching_formats = NULL;
	if (!bfd_check_format_matches( abfd, bfd_object, &matching_formats ))
	{
		dprintf( "Can't open BFD file: %s\n",
			bfd_errmsg( bfd_get_error () ) );
			
		if (matching_formats)
		{
			dprintf( "Matching formats:\n" );
			char ** p = matching_formats;
			while (*p)
			{
				dprintf( "%s\n", *p++ );
			}
		}
		
		bfd_close( abfd );
		return false;
	}
	
	
	// Read in symbol table
	long symcount;
	unsigned int size;
	asymbol ** syms;

	if ((bfd_get_file_flags( abfd ) & HAS_SYMS) == 0)
	{
		dprintf( "BFD file does not have symbols.\n" );
		bfd_close( abfd );
		return false;
	}

	// Static
	symcount = bfd_read_minisymbols( abfd, FALSE, 
		(void **)&syms, &size );
	
	if (symcount == 0)
	{
		// Dynamic
		symcount = bfd_read_minisymbols( abfd, TRUE, 
			(void **)&syms, &size );
	}

	if (symcount < 0) 
	{
		dprintf( "Failed to read symbols from: %s\n",
			bfd_errmsg( bfd_get_error () ) );
		bfd_close( abfd );
		return false;
	}
	
	outSymbolFile->abfd = abfd;
	outSymbolFile->syms = syms;
	
	return true;
}


/**
 *
 */
void closeSymbolsFile( symbolFile & file )
{
	if (file.syms)
	{
		raw_free( file.syms );
	}
	
	if (file.abfd)
	{
		bfd_close( file.abfd );
	}
}

} // anonymous namespace


/**
 *
 */
void initStacktraces()
{
	static bool hasInitted = false;

	if (hasInitted)
	{
		return;
	}

	hasInitted = true;
	bfd_init();
		
	g_canResolveSymbols = loadSymbolsFile( "/proc/self/exe", &g_selfSymbols );
}


/**
 *
 */
void finiStacktraces()
{

}


/**
 *
 */
bool stacktraceSymbolResolutionAvailable()
{
	return g_canResolveSymbols;
}


/**
 *
 */
int stacktrace( CallstackAddressType * trace, size_t depth, size_t skipFrames )
{
	void ** btrace = (void**)bw_alloca( sizeof(void*) * depth );
	int count = backtrace( btrace, depth );

	// Start from 1 to exclude this function
	int skipCount = 1 + (int)skipFrames;
	for (int i = skipCount; i < count; ++i)
	{
		trace[i-skipCount] = (CallstackAddressType)btrace[i];
	}
	return std::max< int >( 0, count - skipCount );
}


/**
 *
 */
int stacktraceAccurate( CallstackAddressType * trace, size_t depth,
	size_t skipFrames, int /*threadID*/ )
{
	return stacktrace( trace, depth, skipFrames );
}


/**
 *
 */
bool convertAddressToFunction( CallstackAddressType address, char * nameBuf,
		size_t nameBufSize, char * fileBuf, size_t fileBufSize, int * line )
{
	if (!stacktraceSymbolResolutionAvailable())
	{
		return false;
	}
	
	char addr[32] = {0};
	symbolResult si = symbolResult();
	
	symbolFile & symFile = g_selfSymbols;
	
	sprintf( addr, "%p", address );
	si.pc = bfd_scan_vma( addr, NULL, 16 );
	si.syms = symFile.syms;
	si.found = false;
	
	bfd_map_over_sections( symFile.abfd, find_address_in_section, &si );
	
	if (nameBuf && nameBufSize)
	{
		*nameBuf = 0;
	}
	
	if (si.found)
	{
#if defined( ATTEMPT_RESOLVE_INLINE )
		do
		{
#endif
			if (nameBuf && nameBufSize)
			{
				const char * name = si.functionname;
				char * demangled = NULL;
				
				if (name != NULL && *name != 0)
				{
					size_t size;
					int status;
					demangled = abi::__cxa_demangle( name, NULL, &size, &status );
					// demangled = bfd_demangle( symFile.abfd, name, DMGL_ANSI | DMGL_PARAMS );
					if (demangled != NULL && *demangled != 0)
					{
						name = demangled;
					}
				}
			
				if (name == NULL || *name == 0)
				{
					if (*nameBuf == 0)
					{
						name = "??";
						strncpy( nameBuf, name, nameBufSize );
					}
				}
				else
				{
					strncpy( nameBuf, name, nameBufSize );
				}

				if (demangled)
				{
					raw_free( demangled );
				}
			}
			
			
			if (fileBuf)
			{
				strncpy( fileBuf, si.filename ? si.filename : "??", 
					fileBufSize );
			}
			
			if (line)
			{
				*line = si.line;
			}
			
#if defined( ATTEMPT_RESOLVE_INLINE )
			si.found = bfd_find_inliner_info( symFile.abfd, &si.filename,
				&si.functionname, &si.line);
		} while (si.found);
#endif
		
		return true;
	}
	else
	{
		if (nameBuf)
		{
			snprintf( nameBuf, nameBufSize, "0x%p", address );
		}
		
		if (fileBuf)
		{
			strncpy( fileBuf, "??", fileBufSize );
		}
		
		return false;
	}
}

BW_END_NAMESPACE

#endif // BW_ENABLE_STACKTRACE

// callstack_linux.cpp
