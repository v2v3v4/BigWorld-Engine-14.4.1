#ifndef PRIMITIVE_FILE_HPP
#define PRIMITIVE_FILE_HPP

#include "cstdmf/smartpointer.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class BinaryBlock;
typedef SmartPointer<BinaryBlock> BinaryPtr;

class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

class PrimitiveFile;
typedef SmartPointer<PrimitiveFile> PrimitiveFilePtr;

/**
 *	This class provides access to a file that contains binary data
 *	for a number of primitive resources. At some stage this kind
 *	of functionality will be integrated into the resource manager
 *	(using either a file system or a data section or some strange
 *	hybrid - zip file reform will happen then too), but for now it
 *	serves only primitive data
 */
class PrimitiveFile : public ReferenceCount
{
public:
	~PrimitiveFile();

	static DataSectionPtr get( const BW::string & resourceID );

#if 0	 // ifdef'd out since functionality moved to BinSection
	BinaryPtr readBinary( const BW::string & name );

	BinaryPtr updateBinary( const BW::string & name,
		void * data, int len );
	void updateBinary( const BW::string & name, BinaryPtr bp );

	void deleteBinary( const BW::string & name );


	void save( const BW::string & name );

#ifdef _WIN32
private:
#else
// Avoiding warning under gcc.
public:
#endif
	PrimitiveFile( BinaryPtr pFile );
private:

	PrimitiveFile( const PrimitiveFile& );
	PrimitiveFile& operator=( const PrimitiveFile& );

	typedef BW::map<BW::string,BinaryPtr> Directory;
	Directory	dir_;
	// intentionally not using stringmap as these are not likely
	// to be the kind of strings that it handles well
	// (i.e. they'll probably have common prefixes)

	static void add( const BW::string & id, PrimitiveFile * pPrimitiveFile );
	static void del( PrimitiveFile * pPrimitiveFile );

	BinaryPtr	file_;
#endif
};


// utility functions until primitive references have transitioned

void splitOldPrimitiveName( const BW::string & resourceID,
	BW::string & file, BW::string & part);

BinaryPtr fetchOldPrimitivePart( BW::string & file, BW::string & part );


#ifdef CODE_INLINE
#include "primitive_file.ipp"
#endif

BW_END_NAMESPACE

#endif // PRIMITIVE_FILE_HPP
