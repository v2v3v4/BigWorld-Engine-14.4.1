#ifndef DOWNLOAD_SEGMENT_HPP
#define DOWNLOAD_SEGMENT_HPP

#include "cstdmf/bw_string.hpp"

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

/**
 *  A single chunk of a DataDownload sent from the server.
 */
class DownloadSegment
{
public:
	DownloadSegment( const char *data, int len, int seq ) :
		seq_( seq ),
		data_( data, len )
	{}

	const char *data() { return data_.c_str(); }
	unsigned int size() { return static_cast<uint>(data_.size()); }

	bool operator< (const DownloadSegment &other)
	{
		return seq_ < other.seq_;
	}

	uint8 seq_;

protected:
	BW::string data_;
};

BW_END_NAMESPACE

#endif // DOWNLOAD_SEGMENT_HPP

