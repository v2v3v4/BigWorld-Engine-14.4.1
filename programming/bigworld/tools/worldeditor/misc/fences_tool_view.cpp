#include "pch.hpp"
#include "fences_tool_view.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "physics2/collision_callback.hpp"
#include "physics2/collision_obstacle.hpp"
#include "common/editor_views.hpp"

BW_BEGIN_NAMESPACE

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE FencesToolView::

PY_TYPEOBJECT(FencesToolView)

PY_BEGIN_METHODS(FencesToolView)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES(FencesToolView)
PY_END_ATTRIBUTES()
PY_FACTORY_NAMED(FencesToolView, "FencesToolView", View)

VIEW_FACTORY(FencesToolView)

#define DRAG_DISTANCE	2

class FenceCollisionCallback : public CollisionCallback
{
	WorldTriangle closestTri_;
	float closestDistance_;

public:
	FenceCollisionCallback()
		: closestDistance_(-1)
	{ }

	bool useHighestAvailableLod() { return true; }

	bool hasCollided() const { return closestDistance_ != -1; }
	const WorldTriangle &triangle() const { return closestTri_; }
	float hitDistance() const { return closestDistance_; }

private:
	virtual int operator()(const CollisionObstacle &obstacle, const WorldTriangle &triangle, float dist)
	{	
		if ((triangle.flags() & TRIANGLE_TERRAIN) == 0)
			return COLLIDE_ALL;

		if (closestDistance_ == -1 || dist < closestDistance_)
		{
			closestTri_ = triangle;
			closestDistance_ = dist;
		}

		return COLLIDE_BEFORE;
	}
};

FencesToolView::FencesToolView(PyTypePlus *pType): 
	ToolView(pType),
	hasStartedSequence_(false),
	canPlaceNextSection_(false),
	canPlaceFullSection_(false),
	alignToTerrainNormal_(false),
	draggingMode_(false),
	prevSectionAttachmentEdge_(0),
	nextSectionAttachmentEdge_(0),
	firstSectionYaw_(0),
	sectionStep_(0),
	fenceId_(-1),
	pGhostModel_(NULL),
	cursorPos_(0, 0, 0),
	posForDragChecking_(0, 0, 0),
	sectionOffset_(0, 0, 0),
	nextSectionOffset_(0, 0, 0),
	fixedAttachmentDirection_(0, 0, 1)
{
	BW_GUARD;

	lastAddFenceTimestamp_ = timestamp();
}

FencesToolView::~FencesToolView()
{
	BW_GUARD;

	releaseGhostModel();
}

PyObject *FencesToolView::pyNew(PyObject *args)
{
	BW_GUARD;

	return WorldManager::instance().getFenceToolView();
}

bool FencesToolView::isFixedDirection()
{
	if (InputDevices::isCtrlDown() || InputDevices::isShiftDown())
	{
		return true;
	}
	return false;
}


/**
 *	Update the animations in this tool view.
 *	@param tool the tool.
 */
void FencesToolView::updateAnimations( const Tool &tool )
{
	BW_GUARD;

	if (pGhostModel_ == NULL || Moo::rc().reflectionScene())
		return;

	if (!hasStartedSequence_ || canPlaceNextSection_)
	{
		ghostModelMatrix_ = nextSectionMatrix_;
	}

	pGhostModel_->updateAnimations( Matrix::identity,
		NULL, // pPreFashions
		NULL, // pPostFashions
		Model::LOD_MIN ); // atLod
}


void FencesToolView::render( Moo::DrawContext& drawContext, const Tool &tool)
{
	BW_GUARD;

	bool immediatelyFlush = false;
	if (pGhostModel_ == NULL || canPlaceNextSection_ == false)
	{
		return;
	}

	if ( ( drawContext.collectChannelMask() & Moo::DrawContext::OPAQUE_CHANNEL_MASK ) == 0 )
	{
		immediatelyFlush = true;
	}

	if (immediatelyFlush)
	{
		drawContext.begin( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
	}
	

	pGhostModel_->draw( drawContext, Matrix::identity, &materialFashions_ );

	if (immediatelyFlush)
	{
		drawContext.end( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
		drawContext.flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
	}
}

/**
* Calculates nextSectionMatrix_ and update flags, according to the current cursor position.
*/
void FencesToolView::calcNextSectionPlacement()
{
	BW_GUARD;

	canPlaceNextSection_ = canPlaceFullSection_ = false;
	if (pGhostModel_ == NULL)
	{
		return;
	}

	// calculate previous section matrix
	Matrix prevSectionMatrix;

	if (!hasStartedSequence_)
	{
		prevSectionMatrix.setRotateY( firstSectionYaw_ );
		if (InputDevices::instance().isAltDown())
		{
			Vector3 fixedPos = *((Vector3*)(&ghostModelMatrix_._41));
			prevSectionMatrix.postTranslateBy( fixedPos );
		}
		else
			prevSectionMatrix.postTranslateBy( cursorPos_ );
	}
	else
	{
		Vector3 worldScale, worldPos;
		Quaternion worldOrient;
		D3DXMatrixDecompose( 
			&worldScale, &worldOrient, &worldPos, &prevSectionMatrix_ );

		// reset pitch rotation
		Matrix mxRotate;
		mxRotate.setRotateY( prevSectionMatrix_.yaw() );
		prevSectionMatrix.setScale( worldScale );
		prevSectionMatrix.postMultiply( mxRotate );
		prevSectionMatrix.postTranslateBy( worldPos );
	}

	// calculate +z/-z attachment points on section model in local space
	Vector3 ptN = modelBBox_.minBounds(), ptP = modelBBox_.maxBounds();
	ptN.x = ptP.x = 0;
	ptN.y = ptP.y = (ptN.y + ptP.y) * 0.5f;
	ptN.z += ptP.z;
	ptP.z = 0;
		
	Vector3 ptNW ( prevSectionMatrix.applyPoint(ptN) );
	Vector3 ptPW ( prevSectionMatrix.applyPoint(ptP) );
	float fenceLenW = (ptP.z - ptN.z) * prevSectionMatrix.nonUniformScale().z;

	// clamp positions to camera chunk bounds
	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	Vector3 clampMin(pSpace->minCoordX(), -1000.0f, pSpace->minCoordZ());
	Vector3 clampMax(pSpace->maxCoordX(), 1000.0f, pSpace->maxCoordZ());
	ptNW.clamp(clampMin, clampMax);
	ptPW.clamp(clampMin, clampMax);

	if (!hasStartedSequence_)
	{
		Matrix nextSectionOrient;
		nextSectionOrient.setRotateY( prevSectionMatrix.yaw() );

		if (groundPoint( ptNW, pSpace ) && groundPoint( ptPW, pSpace ))
		{
			if (!alignToTerrainNormal_ )
			{
				ptPW.y = min( ptNW.y, ptPW.y );
			}
			else
			{
				float angle = atan( (ptNW.y - ptPW.y) / fenceLenW );
				nextSectionOrient.preRotateX( angle );
			}
		}

		nextSectionMatrix_.setScale( prevSectionMatrix.nonUniformScale() );
		nextSectionMatrix_.postMultiply( nextSectionOrient );
		nextSectionMatrix_.postTranslateBy( ptPW );

		canPlaceNextSection_ = canPlaceFullSection_ = true;

		return;
	}

	// attach next section to the attachment point
	float distToN = (ptNW - cursorPos_).lengthSquared();
	float distToP = (ptPW - cursorPos_).lengthSquared();
	if (distToN < 0.09f || distToP < 0.09f)
	{
		// can't attach (cursor too close to +/-z edge, closer than 30cm)
		return;
	}

	Matrix nextSectionOrient;
	Vector3 nextSectionPosWorld, attachmentPoint;

	Matrix mxRotate;
	mxRotate.setRotateY( prevSectionMatrix.yaw() );
	Vector3 offset( mxRotate.applyPoint( sectionOffset_ ) );
	float sectionStep = sectionOffset_.lengthSquared() > 0 ? 0 : sectionStep_;
		
	if ((distToN <= distToP && prevSectionAttachmentEdge_ == 0)
		|| prevSectionAttachmentEdge_ == -1)
	{
		// attach to -z edge
		Vector3 attachDir = this->isFixedDirection() ?
			fixedAttachmentDirection_ : cursorPos_ - ( ptNW + offset );
		attachDir.y = 0;
		if (attachDir.lengthSquared() > 0)
			attachDir.normalise();
		fixedAttachmentDirection_ = attachDir;
		attachmentPoint = ptNW;
				
		nextSectionOrient.setRotateY(atan2f(-attachDir.x, -attachDir.z));

		if (InputDevices::instance().isShiftDown())
		{
			nextSectionPosWorld = cursorPos_;
		}
		else
		{
			nextSectionPosWorld = ptNW + attachDir * sectionStep + offset;
			nextSectionPosWorld.y = cursorPos_.y;
		}

		canPlaceNextSection_ = true;
		canPlaceFullSection_ = sqrtf(distToN) >= (0.75f * fenceLenW);
		nextSectionAttachmentEdge_ = -1;
	}
	else if ((distToP <= distToN && prevSectionAttachmentEdge_ != -1)
		|| prevSectionAttachmentEdge_ == 1)
	{
		// attach to +z edge
		Vector3 attachDir = this->isFixedDirection() ? 
			fixedAttachmentDirection_ : cursorPos_ - ( ptPW + offset );
		attachDir.y = 0;
		if (attachDir.lengthSquared() > 0)
			attachDir.normalise();
		fixedAttachmentDirection_ = attachDir;
		attachmentPoint = ptPW;
				
		nextSectionOrient.setRotateY(atan2f(attachDir.x, attachDir.z));

		if (InputDevices::instance().isShiftDown())
		{
			nextSectionPosWorld = cursorPos_;
		}
		else
		{
			nextSectionPosWorld = ptPW + attachDir * (fenceLenW + sectionStep) + offset;
			nextSectionPosWorld.y = cursorPos_.y;
		}
		
		canPlaceNextSection_ = true;
		canPlaceFullSection_ = sqrtf( distToP ) >= ( 0.75f * fenceLenW );
		nextSectionAttachmentEdge_ = 1;
	}

	// ground the section
	Vector3 pntStart( nextSectionPosWorld ), pntEnd ( nextSectionPosWorld );
	pntEnd -= nextSectionOrient.applyPoint( Vector3( 0, 0, fenceLenW ) );
	if (groundPoint( pntStart, pSpace ) && groundPoint( pntEnd, pSpace ))
	{
		if (!alignToTerrainNormal_ )
		{
			// in unaligned mode, place section to not 'hang in the air'
			nextSectionPosWorld.y = min( pntStart.y, pntEnd.y );
		}
		else
		{
			// in aligned mode preserve fence origin on its XZ
			nextSectionPosWorld.y = pntStart.y;
			float angle = atan( (pntEnd.y - pntStart.y) / fenceLenW );
			nextSectionOrient.preRotateX(angle);
		}
	}

	// calculate offset for the next section
	Vector3 nextSectionStart( nextSectionPosWorld );
	nextSectionStart.y = attachmentPoint.y;
	if (nextSectionAttachmentEdge_ == 1)
	{
		nextSectionStart += 
			nextSectionOrient.applyPoint( Vector3( 0, 0, -fenceLenW ) );
	}
	mxRotate.setRotateY( -prevSectionMatrix.yaw() );
	nextSectionOffset_ = mxRotate.applyPoint( 
		Vector3( nextSectionStart - attachmentPoint ) );

	// calculate next section matrix
	nextSectionMatrix_.setScale( prevSectionMatrix.nonUniformScale() );
	nextSectionMatrix_.postMultiply(nextSectionOrient);
	nextSectionMatrix_.postTranslateBy(nextSectionPosWorld);
}

void FencesToolView::setModelName(const BW::string &modelName)
{
	BW_GUARD;

	modelName_ = modelName;

	releaseGhostModel();

	BW::vector<BW::string> modelNames;
	modelNames.push_back(modelName);
	pGhostModel_ = new SuperModel(modelNames);

	if (!pGhostModel_->isOK())
	{
		releaseGhostModel();
		return;
	}

	pGhostModel_->localVisibilityBox(modelBBox_);
	pGhostModel_->checkBB(false);

	materialFashions_.clear();

	MaterialFashionPtr pFashion = new GhostModelFashion( pGhostModel_ );
	materialFashions_.push_back( pFashion );

	breakSequence();
}

/**
* Setup to continue placing fence sections starting from given existing section
*/
void FencesToolView::continueSequenceFrom(EditorChunkModel *from)
{
	BW_GUARD;

	canPlaceNextSection_ = true;
	hasStartedSequence_ = true;

	FenceInfo *fenceInfo = from->getFenceInfo(true);
	fenceId_ = fenceInfo->fenceId;
	
	prevSectionMatrix_ = from->edTransform();
	Matrix mChunk = from->chunk()->transform();
	prevSectionMatrix_.postMultiply(mChunk);

	calcNextSectionPlacement();
}

/**
* Breaks current sequence, so the next section will be placed independently, starting new fence
*/
void FencesToolView::breakSequence()
{
	BW_GUARD;

	canPlaceNextSection_ = true;
	hasStartedSequence_ = false;
	prevSectionAttachmentEdge_ = nextSectionAttachmentEdge_ = 0;

	fenceId_ = -1;
	sectionOffset_.setZero();
	nextSectionOffset_.setZero();

	calcNextSectionPlacement();
}

void FencesToolView::releaseGhostModel()
{
	BW_GUARD;

	bw_safe_delete( pGhostModel_ );
}

void FencesToolView::setCursorPos(const Vector3 &pos)
{
	BW_GUARD;

	if (InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE ))
	{
		if (posForDragChecking_.x == 0 &&
			posForDragChecking_.z == 0)
		{
			posForDragChecking_ = cursorPos_;
		}
		if (( posForDragChecking_ - cursorPos_ ).lengthSquared() >= DRAG_DISTANCE)
		{
			draggingMode_ = true;
		}
	}
	else
	{
		this->lock( false );
		if (draggingMode_)
		{
			draggingMode_ = false;
		}
		if (posForDragChecking_.x != 0 ||
			posForDragChecking_.z != 0)
		{
			posForDragChecking_ = Vector3(0, 0, 0);
		}
	}

	if (cursorPos_ != pos)
	{
		if (!hasStartedSequence_ && InputDevices::instance().isAltDown())
		{
			Vector3 fixedPos = *((Vector3*)(&ghostModelMatrix_._41));
			Vector3 dir = (fixedPos - cursorPos_);
			dir.y = 0;
			dir.normalise();
			firstSectionYaw_ = dir.yaw();
		}
		cursorPos_ = pos;
		calcNextSectionPlacement();
	}
}

/**
* Returns true if current cursor position and pressed button allows to place next fence section.
* Uses flags updated in calcNextSectionPlacement()
*/
bool FencesToolView::checkCanPlaceNextSection()
{
	BW_GUARD;

	if (this->locked() || !canPlaceNextSection_)
		return false;

	if (draggingMode_ && !canPlaceFullSection_)
		return false;

	if (MILLIS_SINCE(lastAddFenceTimestamp_) < 200.0f)
		return false;

	return true;
}

Matrix FencesToolView::addSectionToSequence()
{
	BW_GUARD;

	prevSectionMatrix_ = nextSectionMatrix_;
	prevSectionAttachmentEdge_ = nextSectionAttachmentEdge_;
	hasStartedSequence_ = true;
	lastAddFenceTimestamp_ = timestamp();
	if (InputDevices::instance().isShiftDown())
	{
		sectionOffset_ = nextSectionOffset_;
	}
		
	//Lock from placing new fences automatically unless Left-mouse key is released or draggingMode is activated.
	//This can prevent placing fences by mistake.
	if (!draggingMode_)
	{
		this->lock( true );
	}

	calcNextSectionPlacement();
	
	return prevSectionMatrix_;
}

void FencesToolView::setAlignToTerrainNormal(bool align)
{
	BW_GUARD;

	if (align != alignToTerrainNormal_)
	{
		alignToTerrainNormal_ = align;
		calcNextSectionPlacement();
	}
}

void FencesToolView::setFenceStep(float step)
{
	BW_GUARD;

	if (step != sectionStep_)
	{
		sectionStep_ = step;
		calcNextSectionPlacement();
	}
}

bool FencesToolView::locked() const
{
	BW_GUARD;
	//if draggingMode is activated, we simply ignore the locked_ flag.
	return !draggingMode_ && locked_;
}

void FencesToolView::lock( bool toLock )
{
	BW_GUARD;
	locked_ = toLock;
}

bool FencesToolView::groundPoint( Vector3 & pnt, ChunkSpacePtr pSpace )
{
	FenceCollisionCallback cb;
	pSpace->collide (
		pnt + Vector3(0, 100, 0), 
		pnt - Vector3(0, 200, 0), cb );
	if (cb.hasCollided())
	{
		pnt.y -= (cb.hitDistance() - 100);
		return true;
	}
	return false;
}


FencesToolStateOperation::FencesToolStateOperation()
	: UndoRedo::Operation( 0 )
{
	FencesToolView* toolView = WorldManager::instance().getFenceToolView();
	MF_ASSERT( toolView != nullptr );

	modelName_ = toolView->modelName_;
	modelBBox_ = toolView->modelBBox_;
	fenceId_ = toolView->fenceId_;
	prevSectionMatrix_ = toolView->prevSectionMatrix_;
	nextSectionMatrix_ = toolView->nextSectionMatrix_;
	canPlaceNextSection_ = toolView->canPlaceNextSection_;
	canPlaceFullSection_ = toolView->canPlaceFullSection_;
	hasStartedSequence_ = toolView->hasStartedSequence_;
	alignToTerrainNormal_ = toolView->alignToTerrainNormal_;
	firstSectionYaw_ = toolView->firstSectionYaw_;
	sectionStep_ = toolView->sectionStep_;
	sectionOffset_ = toolView->sectionOffset_;
	nextSectionOffset_ = toolView->nextSectionOffset_;
	fixedAttachmentDirection_ = toolView->fixedAttachmentDirection_;
	prevSectionAttachmentEdge_ = toolView->prevSectionAttachmentEdge_;
	nextSectionAttachmentEdge_ = toolView->nextSectionAttachmentEdge_;

	BW::map<uint, BW::vector<EditorChunkModel*>>::iterator it =
		EditorChunkModel::s_fences.find( fenceId_ );

	fenceSize_ = it == EditorChunkModel::s_fences.end() ? 0 : it->second.size();
}

void FencesToolStateOperation::setState()
{
	FencesToolView* toolView = WorldManager::instance().getFenceToolView();
	MF_ASSERT( toolView != nullptr );

	toolView->modelName_ = modelName_;
	toolView->modelBBox_ = modelBBox_;
	toolView->fenceId_ = fenceId_;
	toolView->prevSectionMatrix_ = prevSectionMatrix_;
	toolView->nextSectionMatrix_ = nextSectionMatrix_;
	toolView->canPlaceNextSection_ = canPlaceNextSection_;
	toolView->canPlaceFullSection_ = canPlaceFullSection_;
	toolView->hasStartedSequence_ = hasStartedSequence_;
	toolView->alignToTerrainNormal_ = alignToTerrainNormal_;
	toolView->firstSectionYaw_ = firstSectionYaw_;
	toolView->sectionStep_ = sectionStep_;
	toolView->sectionOffset_ = sectionOffset_;
	toolView->nextSectionOffset_ = nextSectionOffset_;
	toolView->fixedAttachmentDirection_ = fixedAttachmentDirection_;
	toolView->prevSectionAttachmentEdge_ = prevSectionAttachmentEdge_;
	toolView->nextSectionAttachmentEdge_ = nextSectionAttachmentEdge_;
}

void FencesToolStateOperation::undo()
{
	FencesToolView* toolView = WorldManager::instance().getFenceToolView();
	MF_ASSERT( toolView != nullptr );

	BW::map<uint, BW::vector<EditorChunkModel*>>::iterator it =
		EditorChunkModel::s_fences.find( fenceId_ );

	size_t newSize = it == EditorChunkModel::s_fences.end() ?
		0 : it->second.size();

	if (newSize == fenceSize_)
	{
		UndoRedo::instance().add(
			new FencesToolStateOperation() );

		setState();
	}
	else
	{
		setState();

		UndoRedo::instance().add(
			new FencesToolStateOperation() );
	}
}


GhostModelFashion::GhostModelFashion(SuperModel *pSuperModel)
{
	BW_GUARD;

	pGhostModel_ = pSuperModel;
	if (pGhostModel_ == NULL)
		return;

	bindings_.clear();
	for (int i = 0; i < pSuperModel->nModels(); ++i)
	{
		Model *model = pSuperModel->topModel(i).getObject();
		do
		{ 
			typedef BW::vector<Moo::Visual::PrimitiveGroup*> PrimGroups_t;
			PrimGroups_t primGroups;
			model->getVisual()->gatherMaterials("*", primGroups);
			for (PrimGroups_t::iterator j = primGroups.begin(); j != primGroups.end(); j++)
			{
				Moo::Visual::PrimitiveGroup *pPrimGroup = *j;
				Moo::ComplexEffectMaterialPtr initialMaterial = pPrimGroup->material_;

				bindings_.push_back(Binding(pPrimGroup, initialMaterial));
			}
		}
		while (model = model->parent());
	}

	replacementMaterial_ = new Moo::ComplexEffectMaterial();
	replacementMaterial_->initFromEffect( "resources/shadersfences.fx" );

	hWVPParam_ = replacementMaterial_->baseMaterial()->pEffect()->pEffect()->GetParameterByName( NULL, "world" );
	hColorParam_ = replacementMaterial_->baseMaterial()->pEffect()->pEffect()->GetParameterByName( NULL, "color" );
}

void GhostModelFashion::dress(SuperModel &model)
{
	BW_GUARD;

	for (Bindings::iterator it = bindings_.begin(); it != bindings_.end(); it++)
	{
		it->pPrimGroup->material_ = replacementMaterial_;
	}

	Moo::RenderContext &rc = Moo::rc();
	Matrix world = WorldManager::instance().getFenceToolView()->ghostModelMatrix_;
	replacementMaterial_->baseMaterial()->pEffect()->pEffect()->SetValue(hWVPParam_, &world, sizeof(D3DXMATRIX));

	float sin = 0.7f + 0.3f * sinf(3.0f * (float)((double)timestamp() / (double)stampsPerSecond()));
	D3DXCOLOR color(sin, sin, sin, sin);
	replacementMaterial_->baseMaterial()->pEffect()->pEffect()->SetValue(hColorParam_, &color, sizeof(D3DXCOLOR));
}

void GhostModelFashion::undress(SuperModel &model)
{
	BW_GUARD;

	if (pGhostModel_ != &model)
		return;

	for (Bindings::iterator it = bindings_.begin(); it != bindings_.end(); it++)
		it->pPrimGroup->material_ = it->initialMaterial;
}

BW_END_NAMESPACE

