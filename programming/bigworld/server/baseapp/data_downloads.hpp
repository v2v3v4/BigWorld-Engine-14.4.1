#ifndef DATA_DOWNLOADS_HPP
#define DATA_DOWNLOADS_HPP

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/stdmf.hpp"
#include "pyscript/script.hpp"

#include <deque>
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class BinaryOStream;
class DataDownload;
class DataDownloadFactory;
class FileStreamingJob;

namespace Mercury
{
class Bundle;
}

class IFinishedDownloads
{
public:
	virtual void onFinished( uint16 id, bool successful ) = 0;
};


/**
 *  A collection of partially completed downloads.
 */
class DataDownloads
{
public:
	DataDownloads();
	~DataDownloads();

	PyObject * streamToClient( DataDownloadFactory & factory,
		   PyObjectPtr pDesc, int id );

	int addToBundle( Mercury::Bundle & bundle, int & remaining,
		   IFinishedDownloads & finishedDownloads );

	bool push_back( DataDownload *pDL );
	bool contains( uint16 id ) const;
	bool empty() const;

	void abortDownloads( IFinishedDownloads & finishedDownloads );

private:
	typedef std::deque< DataDownload* > Container;

	Container::iterator erase( Container::iterator it );

	Container dls_;

	// The next unused ID to be given out.
	uint16 freeID_;

	// A collection of used IDs
	typedef BW::set< uint16 > IDSet;
	IDSet usedIDs_;

	friend BinaryOStream & operator<<( BinaryOStream & b,
		const DataDownloads & rDownloads );
	friend BinaryIStream & operator>>( BinaryIStream & b,
		DataDownloads & rDownloads );
};


/**
 *  Interface for partially complete downloads.
 */
class DataDownload
{
public:
	static int packetOverhead();

	DataDownload( PyObjectPtr pDesc, uint16 id, DataDownloads &dls );
	virtual ~DataDownload() {}

	// Copies the specified number of bytes into the provided stream.
	virtual void read( BinaryOStream &os, int nBytes ) = 0;

	// Returns true if all data has been written out and this object can be
	// destroyed.
	virtual bool done() const = 0;

	// Returns false if something has gone wrong and the download should be
	// aborted.
	virtual bool good() const { return good_; }

	int addToBundle( Mercury::Bundle & bundle, int & remaining );

	void onDone();

	uint16 id() const		{ return id_; }
	void id( uint16 v )		{ id_ = v; }

protected:
	// Returns the number of bytes that are currently buffered and ready to be
	// written out.
	virtual int available() const = 0;

	bool good_;

private:
	uint16 id_;
	uint8 seq_;
	PyObjectPtr pDesc_;

	int bytesSent_;
	int packetsSent_;
	uint64 start_;
};


/**
 *	This interface is used to create DataDownload instances.
 */
class DataDownloadFactory
{
public:
	DataDownload * create( PyObjectPtr pDesc, uint16 id,
		DataDownloads &dls );

protected:
	virtual DataDownload * onCreate( PyObjectPtr pDesc, uint16 id,
		DataDownloads &dls ) = 0;

private:
	bool checkDescription( PyObjectPtr & rpDesc );
};


class StringDataDownloadFactory : public DataDownloadFactory
{
public:
	StringDataDownloadFactory( PyObjectPtr pData ) : pData_( pData ) {}

private:
	virtual DataDownload * onCreate( PyObjectPtr pDesc, uint16 id,
		DataDownloads &dls );

	PyObjectPtr pData_;
};


class FileDataDownloadFactory : public DataDownloadFactory
{
public:
	FileDataDownloadFactory( const BW::string & path ) : path_( path ) {}

private:
	virtual DataDownload * onCreate( PyObjectPtr pDesc, uint16 id,
		DataDownloads &dls );

	BW::string path_;
};

BW_END_NAMESPACE

#endif // DATA_DOWNLOADS_HPP
