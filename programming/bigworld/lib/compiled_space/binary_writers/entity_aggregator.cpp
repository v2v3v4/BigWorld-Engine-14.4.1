#include "pch.hpp"

#include "entity_aggregator.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/packed_section.hpp"
#include "resmgr/multi_file_system.hpp"

#include "compiled_space/binary_writers/binary_format_writer.hpp"

class ScopedTemporaryFile
{
public:
	ScopedTemporaryFile( const char* prefix )
	{
		memset( filename_, 0, sizeof(filename_) );

		char tempPath[MAX_PATH] = {0};
		GetTempPathA( sizeof(tempPath), tempPath );
		GetTempFileNameA( tempPath, prefix, 0, filename_ );
	}

	~ScopedTemporaryFile()
	{
		DeleteFileA( filename_ );
	}

	const char* filename() { return filename_; }

private:
	char filename_[BW_MAX_PATH];
};

BW_BEGIN_NAMESPACE

//-----------------------------------------------------------------------------
void EntityAggregator::add( const DataSectionPtr& pDS,
		const Matrix& chunkTransform )
{
	Section sec;
	sec.chunkTransform_ = chunkTransform;
	sec.pDS_ = pDS;

	sections_.push_back( sec );
}


//-----------------------------------------------------------------------------
bool EntityAggregator::write( const BW::string& filename,
				const BW::vector<BW::string>& filterOut )
{
	DataResource dataRes( filename, RESOURCE_TYPE_XML, false );
	DataSectionPtr root = dataRes.getRootSection();
	root->delChildren();

	for (size_t i = 0; i < sections_.size(); ++i)
	{
		DataSectionPtr pSrcDS = sections_[i].pDS_;
		DataSectionPtr pNewDS = root->newSection( pSrcDS->sectionName() );
		pNewDS->copy( pSrcDS );

		// Strip out sub sections we don't want
		for (size_t j = 0; j < filterOut.size(); ++j)
		{
			pNewDS->delChild( filterOut[j] );
		}

		// Update transform
		Matrix m = sections_[i].pDS_->readMatrix34( "transform", Matrix::identity );
		m.postMultiply( sections_[i].chunkTransform_ );

		pNewDS->writeMatrix34( "transform", m );
	}

	return dataRes.save() == DataHandle::DHE_NoError;
}


//-----------------------------------------------------------------------------
bool EntityAggregator::writeToBinary( BinaryFormatWriter& writer,
	const FourCC& magic, uint32 version,
	const BW::vector<BW::string>& filterOut )
{
	ScopedTemporaryFile tempXMLFile( "space_converter" );
	{
        FILE* fh = BWResource::instance().fileSystem()->posixFileOpen( tempXMLFile.filename(), "w" );

        if ( fh )
        {
            fprintf( fh, "<root>\n</root>\n" );
            fclose( fh );
        }
	}

	ScopedTemporaryFile tempBINFile( "space_converter" );

	if (!this->write( tempXMLFile.filename(), filterOut ))
	{
		return false;
	}

	if (!PackedSection::convert( tempXMLFile.filename(), 
		tempBINFile.filename(), NULL, true ))
	{
		return false;
	}

	BinaryPtr pBin = BWResource::instance().nativeFileSystem()->readFile( 
		tempBINFile.filename() );
	if (!pBin)
	{
		return false;
	}

	BinaryFormatWriter::Stream* pStream = writer.appendSection( magic, version );
	if (!pStream)
	{
		return false;
	}

	pStream->writeRaw( pBin->data(), pBin->len() );
	return true;
}

BW_END_NAMESPACE

