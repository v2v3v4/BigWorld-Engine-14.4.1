#ifndef CONSOLIDATE_HPP
#define CONSOLIDATE_HPP

#include "cstdmf/stdmf.hpp"

#include "network/basictypes.hpp"
#include "network/endpoint.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class Consolidate
{
public:
	Consolidate( BW::string dbFilename );
	~Consolidate();

	bool transferTo( Mercury::Address & receivingAddress );

private:
	bool prepareDBForSend();
	bool readAndSendDB();
	bool waitForDeleteCommand();

	void error( BW::string errorStr, BW::string supplementalStr );

	// Consolidation data
	Endpoint endpoint_;

	off_t sqliteSize_;
	BW::string sqliteFilename_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_HPP
