#ifndef EDITOR_CHUNK_SUBSTANCE
#define EDITOR_CHUNK_SUBSTANCE

#include "math/boundbox.hpp"
#include "worldeditor/project/project_module.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "chunk/chunk_model_obstacle.hpp"
#include "chunk/chunk_model.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "romp/fog_controller.hpp"

BW_BEGIN_NAMESPACE

class   Chunk;
class   Model;
typedef SmartPointer<Model> ModelPtr;

/**
 *	This template class gives substance in the editor to items that
 *	are ordinarily without it.
 */
template <class CI> class EditorChunkSubstance : public CI
{
public:
	EditorChunkSubstance();
	~EditorChunkSubstance();

	bool load( DataSectionPtr pSection );
	bool load( DataSectionPtr pSection, Chunk * pChunk );

	void updateReprModel();

	virtual void toss( Chunk * pChunk );

	virtual bool edShouldDraw();

	virtual void updateAnimations();
	virtual void draw( Moo::DrawContext& drawContext );

	virtual void edBounds( BoundingBox& bbRet ) const;

	virtual const char * sectName() const = 0;
	virtual bool isDrawFlagVisible() const = 0;
	virtual const char * drawFlag() const = 0;
	virtual ModelPtr reprModel() const = 0;

	virtual bool autoAddToSceneBrowser() const { return true; }

	virtual void addAsObstacle();	// usually don't override

	virtual DataSectionPtr pOwnSect()	{ return pOwnSect_; }
	virtual const DataSectionPtr pOwnSect()	const { return pOwnSect_; }

	bool reload();

	virtual bool addYBounds( BoundingBox& bb ) const;

protected:
	DataSectionPtr	pOwnSect_;
	SuperModel*		pReprSuperModel_;
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_SUBSTANCE