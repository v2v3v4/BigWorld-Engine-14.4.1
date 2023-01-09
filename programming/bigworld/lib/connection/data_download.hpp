#ifndef DATA_DOWNLOAD_HPP
#define DATA_DOWNLOAD_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_string.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;
class DownloadSegment;

/**
 *  A list of DownloadSegments used to collect chunks of data as they are
 *  downloaded from an addProxyData() or addProxyFileData() call.
 */
class DataDownload : public BW::list< DownloadSegment* >
{
public:
	DataDownload( uint16 id ) :
		id_( id ),
		pDesc_( NULL ),
		hasLast_( false )
	{}

	~DataDownload();

	void insert( DownloadSegment *pSegment, bool isLast );
	bool complete();
	void write( BinaryOStream &os );
	uint16 id() const { return id_; }
	const BW::string *pDesc() const { return pDesc_; }
	void setDesc( BinaryIStream &stream );

protected:

	// The id of this DataDownload transfer
	uint16 id_;

	// The description associated with this download
	BW::string *pDesc_;

	// Set to true when the final segment has been added.
	bool hasLast_;
};

BW_END_NAMESPACE

#endif // DATA_DOWNLOAD_HPP

