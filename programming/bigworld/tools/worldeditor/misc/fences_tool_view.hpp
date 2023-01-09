#ifndef FENCES_TOOL_VIEW_HPP
#define FENCES_TOOL_VIEW_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_model.hpp"
#include "gizmo/tool.hpp"

BW_BEGIN_NAMESPACE

class GhostModelFashion;
class FencesToolStateOperation;

class FencesToolView : public ToolView
{
	Py_Header( FencesToolView, ToolView )

	friend class GhostModelFashion;
	friend class FencesToolStateOperation;

public:
	FencesToolView(PyTypePlus * pType = &s_type_);
	~FencesToolView();

	virtual void updateAnimations( const Tool &tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool &tool );

	void setModelName(const BW::string &modelName);
	void breakSequence();
	void continueSequenceFrom(EditorChunkModel *from);
	void setCursorPos(const Vector3 &pos);
	void setAlignToTerrainNormal(bool align);
	void setFenceStep(float step);
	bool checkCanPlaceNextSection();
	Matrix addSectionToSequence();

	const BW::string &modelName() const { return modelName_; }
	
	int fenceId() const { return fenceId_; }
	void fenceId(int id) { fenceId_ = id; }

	bool locked() const;
	void lock( bool toLock);

	PY_FACTORY_DECLARE()

private:
	VIEW_FACTORY_DECLARE(FencesToolView())

	void calcNextSectionPlacement();
	void releaseGhostModel();
	bool isFixedDirection();
	bool groundPoint( Vector3 & pnt, ChunkSpacePtr pSpace );
	BW::string modelName_;
	BoundingBox modelBBox_;
	int fenceId_;

	Matrix prevSectionMatrix_;
	Matrix nextSectionMatrix_;
	Vector3 sectionOffset_;
	Vector3 nextSectionOffset_;
	Vector3 cursorPos_;
	Vector3 posForDragChecking_;
	Vector3 fixedAttachmentDirection_;
	float firstSectionYaw_;
	float sectionStep_;
	bool canPlaceNextSection_;
	bool canPlaceFullSection_;
	bool locked_;
	bool hasStartedSequence_;
	bool alignToTerrainNormal_;
	bool draggingMode_;

	int prevSectionAttachmentEdge_;
	int nextSectionAttachmentEdge_;
	uint64 lastAddFenceTimestamp_;

	SuperModel *pGhostModel_;
	Matrix ghostModelMatrix_;
	MaterialFashionVector materialFashions_;
};


class FencesToolStateOperation : public UndoRedo::Operation
{
public:
	FencesToolStateOperation();

protected:
	void setState();
	void undo();
	bool iseq( const UndoRedo::Operation& oth ) const {	return false; }

	BW::string modelName_;
	BoundingBox modelBBox_;
	int fenceId_;
	size_t fenceSize_;

	Matrix prevSectionMatrix_;
	Matrix nextSectionMatrix_;
	Vector3 sectionOffset_;
	Vector3 nextSectionOffset_;
	Vector3 fixedAttachmentDirection_;
	float firstSectionYaw_;
	float sectionStep_;
	bool canPlaceNextSection_;
	bool canPlaceFullSection_;
	bool hasStartedSequence_;
	bool alignToTerrainNormal_;

	int prevSectionAttachmentEdge_;
	int nextSectionAttachmentEdge_;
};


class GhostModelFashion : public MaterialFashion
{
public:
	GhostModelFashion(SuperModel *pSuperModel);
	
	virtual void dress(SuperModel &model);
	virtual void undress(SuperModel &model);

private:
	struct Binding
	{
		Binding(Moo::Visual::PrimitiveGroup *_pPrimGroup, const Moo::ComplexEffectMaterialPtr &material)
			: pPrimGroup(_pPrimGroup), initialMaterial(material)
		{
		}

		Moo::Visual::PrimitiveGroup *pPrimGroup;
		Moo::ComplexEffectMaterialPtr initialMaterial;
	};

	typedef BW::vector<Binding> Bindings;

	SuperModel *pGhostModel_;
	Bindings bindings_;
	Moo::ComplexEffectMaterialPtr replacementMaterial_;
	D3DXHANDLE hWVPParam_;
	D3DXHANDLE hColorParam_;
};

BW_END_NAMESPACE

#endif // TERRAIN_Fences_FENCES_VIEW_HPP
