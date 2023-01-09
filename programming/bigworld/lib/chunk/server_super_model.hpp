#ifndef SERVER_SUPER_MODEL_HPP
#define SERVER_SUPER_MODEL_HPP

#include "cstdmf/smartpointer.hpp"

#include "math/boundbox.hpp"

#include "resmgr/datasection.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class BSPTree;
class BoundingBox;

typedef SmartPointer< class Model > ModelPtr;

/**
 *	This class is the base class for Models implemented for the server.
 */
class Model : public SafeReferenceCount
{
public:
	static ModelPtr get( const BW::string & resourceID );

	virtual bool valid() const = 0;
	virtual const BSPTree * decompose() const	{ return 0; }
	virtual const BoundingBox & boundingBox() const = 0;

	void	beginRead(){}
	void	endRead(){}

protected:
	static Model * load( const BW::string & resourceID, DataSectionPtr pFile );
};


/**
 *	This class is the base class for ServerModels
 */
class ServerModel : public Model
{
public:
	ServerModel( BSPTree * pTree, BoundingBox & bb );
	ServerModel( const BW::string & type, DataSectionPtr pFile );

	/**
	 *	This method returns whether or not this model is valid.
	 */
	virtual bool valid() const { return pTree_ != NULL; }

	virtual const BoundingBox & boundingBox() const { return bb_; }

	virtual const BSPTree * decompose() const { return pTree_; }

private:
	BSPTree * pTree_;
	BoundingBox bb_;
};


/**
 *	This class implements the SuperModel for the server. It is basically just a
 *	list of Models.
 */
class SuperModel
{
public:
	SuperModel( const BW::vector< BW::string > & modelIDs );

	int nModels() const						{ return models_.size(); }

	ModelPtr topModel( int i )				{ return models_[i]; }

	void boundingBox( BoundingBox& bb ) const;

private:
	BW::vector< ModelPtr >	models_;
};

BW_END_NAMESPACE

#endif // SERVER_SUPER_MODEL_HPP
