#include "pch.hpp"

#include "data_download.hpp"

#include "download_segment.hpp"


#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

DataDownload::~DataDownload()
{
	for (iterator it = this->begin(); it != this->end(); ++it)
	{
		bw_safe_delete(*it);
	}

	bw_safe_delete(pDesc_);
}


/**
 *  Insert the segment into this record in a sorted fashion.
 */
void DataDownload::insert( DownloadSegment *pSegment, bool isLast )
{
	if (isLast)
		hasLast_ = true;
	
	this->push_back( pSegment );

}


/**
 *  Returns true if this piece of DataDownload is complete and ready to be returned
 *  back into onStreamComplete().
 */
bool DataDownload::complete()
{
	return hasLast_ && pDesc_ != NULL;
}


/**
 *  Write the contents of this DataDownload into a BinaryOStream.  This DataDownload
 *  must be complete() before calling this.
 */
void DataDownload::write( BinaryOStream &os )
{
	MF_ASSERT_DEV( this->complete() );
	for (iterator it = this->begin(); it != this->end(); ++it)
	{
		DownloadSegment &segment = **it;
		os.addBlob( segment.data(), segment.size() );
	}
}


/**
 *  Set the description for this download from the provided stream.
 */
void DataDownload::setDesc( BinaryIStream &is )
{
	pDesc_ = new BW::string();
	is >> *pDesc_;
}

BW_END_NAMESPACE

// data_download.cpp

