#include "pch.hpp"
#include "wpentity.hpp"

#include "chunk/chunk_model.hpp"
#include "chunk/chunk_model_obstacle.hpp"
#include "chunk/geometry_mapping.hpp"

#include "model/super_model.hpp"
#include "moo/render_context.hpp"
#include "pyscript/script.hpp"
#include "pyscript/py_data_section.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"
#include "entitydef/constants.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: WPEntity
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
WPEntity::WPEntity() :
	transform_( Matrix::identity ),
	pSuperModel_( NULL )
{
}


/**
 *	Destructor.
 */
WPEntity::~WPEntity()
{
	BW_GUARD;

	if (pSuperModel_ != NULL)
	{
		delete pSuperModel_;
		pSuperModel_ = NULL;
	}
}

/**
 *	Load method
 */
bool WPEntity::load( DataSectionPtr pSection )
{
	BW_GUARD;

	Matrix temp = pSection->readMatrix34( "transform" );
	transform_ = temp;
	typeName_ = pSection->readString( "type" );
	pProps_ = pSection->openSection( "properties" );
	return true;
}

/**
 *	finishLoad method
 */
void WPEntity::finishLoad()
{
	BW_GUARD;

	PyObject * pModule = PyImport_ImportModule(
		const_cast<char*>(typeName_.c_str()) );
	if (pModule != NULL)
	{
		PyObject * pClass = PyObject_GetAttrString(
			pModule, const_cast<char*>(typeName_.c_str()) );

		if (pClass != NULL)
		{
			// make an instance of the class
			PyObject * pEntityInst = PyObject_CallObject( pClass, PyTuple_New(0) );
			if (pEntityInst != NULL)
			{
				PyObject * pTuple = PyTuple_New(1);

				if (pProps_)
				{
					PyTuple_SetItem( pTuple, 0, new PyDataSection( pProps_ ) );
				}
				else
				{
					Py_INCREF( Py_None );
					PyTuple_SetItem( pTuple, 0, Py_None );
				}

				PyObject * pResult = Script::ask(
					PyObject_GetAttrString( pEntityInst, "getObstacleModel" ),
					pTuple,
					"WPEntity::load: getObstacle",
					/*okIfFunctionNull*/true );
				BW::string obstModelName;
				if (Script::setAnswer( pResult, obstModelName ))
				{
					BW::vector<BW::string> modelNames( 1, obstModelName );
					pSuperModel_ = new SuperModel( modelNames );
					if (pSuperModel_->nModels() != 1)
					{
						ERROR_MSG( "WPEntity::load: "
							"Could not load obstacle model %s\n",
							obstModelName.c_str() );
						delete pSuperModel_;
						pSuperModel_ = NULL;
					}
				}
			}
			Py_DECREF( pClass );
		}

		Py_DECREF( pModule );
	}
	else if (BWResource::fileExists( BWResolver::resolveFilename(
		BW::string( EntityDef::Constants::entitiesEditorPath() ) +
		"/" + typeName_ + ".py")))
	{
		ERROR_MSG( "Could not load editor entity module %s\n", typeName_.c_str() );
		PyErr_Print();
	}
	PyErr_Clear();
}

/**
 *	Toss method
 */
void WPEntity::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk == pChunk_) return;

	if (pChunk_ != NULL)
	{
		if (pSuperModel_ != NULL)
		{
			ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );
		}

		WPEntityCache::instance( *pChunk_ ).del( this );
	}
	this->ChunkItem::toss( pChunk );
	if (pChunk_ != NULL)
	{
		WPEntityCache::instance( *pChunk_ ).add( this );

		if (pSuperModel_ != NULL)
		{
			Matrix world( pChunk_->transform() );
			world.preMultiply( this->transform_ );

			for (int i = 0; i < this->pSuperModel_->nModels(); i++)
			{
				ChunkModelObstacle::instance( *pChunk_ ).addModel(
					this->pSuperModel_->topModel( i ), world, this );
			}
		}
	} 
}


/**
 *	overridden draw method
 */
void WPEntity::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if( pSuperModel_ == NULL )
		finishLoad();

	if (pSuperModel_ != NULL)
	{
		Moo::rc().push();
		Moo::rc().preMultiply( transform_ );

		pSuperModel_->draw( drawContext, Moo::rc().world(), NULL );

		Moo::rc().pop();
	}
}

IMPLEMENT_CHUNK_ITEM( WPEntity, entity, 0 )


// -----------------------------------------------------------------------------
// Section: WPEntityCache
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
WPEntityCache::WPEntityCache( Chunk & )
{
}

/**
 *	Destructor
 */
WPEntityCache::~WPEntityCache()
{
}

/**
 *	Add this entity
 */
void WPEntityCache::add( WPEntityPtr e )
{
	BW_GUARD;

	entities_.push_back( e );
}

/**
 *	Remove this entity
 */
void WPEntityCache::del( WPEntityPtr e )
{
	BW_GUARD;

	WPEntities::iterator found = std::find(
		entities_.begin(), entities_.end(), e );
	if (found != entities_.end())
		entities_.erase( found );
}


/// Static instance accessor initialiser
ChunkCache::Instance<WPEntityCache> WPEntityCache::instance;

BW_END_NAMESPACE

// wpentity.cpp
