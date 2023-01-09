#ifndef VERSION_INFO_HPP
#define VERSION_INFO_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/cstdmf_windows.hpp"

#include <psapi.h>

#include <iostream>


BW_BEGIN_NAMESPACE

typedef BOOL (WINAPI * VI_GETPROCESSMEMORYINFO)(HANDLE, PROCESS_MEMORY_COUNTERS*, DWORD);
#define VI_GETPROCESSMEMORYINFO_NAME	"GetProcessMemoryInfo"


/**
 *	This class displays driver and operating system info
 */
class VersionInfo
{
public:
	VersionInfo();
	~VersionInfo();

	static VersionInfo&	instance( void );

	///This must be called to initialise all values
	void		queryAll( void );

	///Accessors for individual values
	BW::string OSName( void ) const;
	int			OSMajor( void ) const;
	int			OSMinor( void ) const;
	BW::string OSServicePack( void ) const;

	BW::string DXName( void ) const;
	int			DXMajor( void ) const;
	int			DXMinor( void ) const;


	///Global memory
	size_t		totalPhysicalMemory( void ) const;
	size_t		availablePhysicalMemory( void ) const;

	size_t		totalVirtualMemory( void ) const;
	size_t		availableVirtualMemory( void ) const;

	size_t		totalPagingFile( void ) const;
	size_t		availablePagingFile( void ) const;

	uint		memoryLoad( void ) const;

	///Process memory
	uint		pageFaults( void ) const;
	size_t		peakWorkingSet( void ) const;
	size_t		workingSet( void ) const;
	size_t		quotaPeakedPagePoolUsage( void ) const;
	size_t		quotaPagePoolUsage( void ) const;
	size_t		quotaPeakedNonPagePoolUsage( void ) const;
	size_t		quotaNonPagePoolUsage( void ) const;
	size_t		peakPageFileUsage( void ) const;
	size_t		pageFileUsage( void ) const;

	size_t		workingSetRefetched( void ) const;

	///Adapter information
	BW::string	adapterDriver( void ) const;
	BW::string adapterDesc( void ) const;
	uint32		adapterDriverMajorVer( void ) const;
	uint32		adapterDriverMinorVer( void ) const;



private:
	void		queryOS( void );
	void		queryDX( void );
	void		queryMemory( void ) const;
	void		queryHardware( void );
	DWORD		queryDXVersion( void );

	VersionInfo(const VersionInfo&);
	VersionInfo& operator=(const VersionInfo&);

	int			osMajor_;
	int			osMinor_;
	BW::string osName_;
	BW::string osServicePack_;
	int			dxMajor_;
	int			dxMinor_;
	BW::string dxName_;

	BW::string adapterDriver_;
	BW::string adapterDesc_;
	uint32		adapterDriverMajorVer_;
	uint32		adapterDriverMinorVer_;

	mutable VI_GETPROCESSMEMORYINFO vi_getProcessMemoryInfo_;
	mutable HINSTANCE loadedDll_;
	mutable MEMORYSTATUS memoryStatus_;
	mutable PROCESS_MEMORY_COUNTERS processMemoryStatus_;
	mutable unsigned long lastMemoryCheck_;

	friend std::ostream& operator<<(std::ostream&, const VersionInfo&);
};

#ifdef CODE_INLINE
#include "version_info.ipp"
#endif

BW_END_NAMESPACE


#endif
/*version_info.hpp*/
