#ifndef DOWNLOAD_STREAMER_HPP
#define DOWNLOAD_STREAMER_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

class DownloadStreamer
{
public:
	DownloadStreamer();

	int maxDownloadRate() const;
	int curDownloadRate() const;
	int maxClientDownloadRate() const;
	int downloadRampUpRate() const;
	int downloadBacklogLimit() const;

	float downloadScaleBack() const;
	void modifyDownloadRate( int delta );

private:
	int					curDownloadRate_;
};

BW_END_NAMESPACE

#endif // DOWNLOAD_STREAMER_HPP
