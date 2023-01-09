#ifndef TERRAIN_RULER_TOOL_VIEW_HPP
#define TERRAIN_RULER_TOOL_VIEW_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/tool.hpp"

BW_BEGIN_NAMESPACE

class TerrainRulerToolView : public ToolView
{
	Py_Header( TerrainRulerToolView, ToolView )

public:
	TerrainRulerToolView(PyTypePlus * pType = &s_type_);
	~TerrainRulerToolView();

	static TerrainRulerToolView *instance() { return s_instance; }

	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool &tool);

	bool enableRender() const { return enableRender_; }
	void enableRender(bool enable);

	const Vector3 &startPos() const { return startPos_; }
	void startPos(const Vector3 &startPos);

	const Vector3 &endPos() const { return endPos_; }
	void endPos(const Vector3 &endPos);

	float width() const { return width_; }
	void width(float width);

	bool verticalMode() const { return verticalMode_; }
	void verticalMode(bool verticalMode);

	PY_FACTORY_DECLARE()

private:
	VIEW_FACTORY_DECLARE(TerrainRulerToolView())

	void updateVertices();
	bool calcQuadCorners(const Vector3 &a, const Vector3 &b, float width, Vector3 &c0, Vector3 &c1, Vector3 &c2, Vector3 &c3);
	void addLine(const Vector3 &from, const Vector3 &to, float width);
	void addQuad(const Vector3 &c0, const Vector3 &c1, const Vector3 &c2, const Vector3 &c3);

	Vector3 startPos_;
	Vector3 endPos_;
	bool enableRender_;
	float width_;
	bool verticalMode_;

	bool vertsDirty_;

	BW::vector<Moo::VertexXYZ> vertices_;
	BW::vector<ushort> indices_;

	Moo::ManagedEffectPtr effect_;
	D3DXHANDLE hColorParam_;
	D3DXHANDLE hWVPParam_;

	static TerrainRulerToolView *s_instance;
};

BW_END_NAMESPACE

#endif // TERRAIN_RULER_TOOL_VIEW_HPP
