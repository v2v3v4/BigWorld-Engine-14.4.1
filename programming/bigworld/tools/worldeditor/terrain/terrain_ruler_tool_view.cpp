#include "pch.hpp"
#include "worldeditor/terrain/terrain_ruler_tool_view.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/dynamic_vertex_buffer.hpp"

BW_BEGIN_NAMESPACE

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE TerrainRulerToolView::


PY_TYPEOBJECT(TerrainRulerToolView)

PY_BEGIN_METHODS(TerrainRulerToolView)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES(TerrainRulerToolView)
PY_END_ATTRIBUTES()
PY_FACTORY_NAMED(TerrainRulerToolView, "TerrainRulerToolView", View)

VIEW_FACTORY(TerrainRulerToolView)


TerrainRulerToolView *TerrainRulerToolView::s_instance = NULL;

TerrainRulerToolView::TerrainRulerToolView(PyTypePlus *pType): 
	ToolView(pType),
	startPos_(Vector3(0, 0, 0)),
 	endPos_(Vector3(0, 0, 1)),
	width_(2),
	vertsDirty_(true),
	enableRender_(false),
	verticalMode_(false)
{
	MF_ASSERT(s_instance == NULL);
	s_instance = this;

	effect_ = Moo::EffectManager::instance().get( "resources/shaders/ruler.fx" );

	if (effect_ != NULL)
	{
		hColorParam_ = effect_->pEffect()->GetParameterByName(NULL, "color");
		hWVPParam_ = effect_->pEffect()->GetParameterByName(NULL, "worldViewProj");
	}
}

TerrainRulerToolView::~TerrainRulerToolView()
{
	s_instance = NULL;
}


/**
 *	Update the animations in this tool view.
 *	@param tool the tool.
 */
void TerrainRulerToolView::updateAnimations( const Tool& tool )
{
	BW_GUARD;

	if (effect_ == NULL || !enableRender_)
		return;

	if (vertsDirty_)
	{
		updateVertices();
		vertsDirty_ = false;
	}
}


void TerrainRulerToolView::render( Moo::DrawContext& drawContext, const Tool &tool )
{
	BW_GUARD;

	if (effect_ == NULL || !enableRender_)
	{
		return;
	}

	if (indices_.size() < 30)
		return;

	Moo::RenderContext &rc = Moo::rc();

	// states to save. ruler.fx modifies them, affecting the rest of the scene. 
	rc.pushRenderState( D3DRS_ALPHATESTENABLE );
	rc.pushRenderState( D3DRS_ALPHABLENDENABLE );
	rc.pushRenderState( D3DRS_FOGENABLE );

	Moo::DynamicIndexBufferBase &indBuf = rc.dynamicIndexBufferInterface().get(D3DFMT_INDEX16);
	Moo::IndicesReference indLock = indBuf.lock(static_cast<uint32>(indices_.size()));
	if (!indLock.size())
	{
		indBuf.unlock();
		return;
	}
	indLock.fill(&indices_.front(), static_cast<uint32>(indices_.size()));
	indBuf.unlock();
	uint32 ibLockIndex = indBuf.lockIndex();

	typedef Moo::VertexXYZ VertexType;
	int vertexSize = sizeof(VertexType);
	
	Moo::DynamicVertexBuffer & dvb = 
		Moo::DynamicVertexBuffer::instance( vertexSize );

	void * vertLock = dvb.lock( static_cast<uint32>(vertices_.size()) );
	if (!vertLock)
		return;

	memcpy(vertLock, &vertices_.front(), vertexSize * vertices_.size());
	dvb.unlock();
	uint32 vbLockIndex = dvb.lockIndex();

	rc.setFVF( VertexType::fvf() );
	dvb.set();
	indBuf.indexBuffer().set();

	uint32 nPasses;
	ID3DXEffect *pEffect = effect_->pEffect();
	pEffect->SetValue(hWVPParam_, &rc.viewProjection(), sizeof(D3DXMATRIX));
	pEffect->Begin(&nPasses, 0);

	D3DXCOLOR color;
	for (uint i = 0; i < 2; i++)
	{
		pEffect->BeginPass(i);

		// draw the quad
		color = D3DXCOLOR(1.0f, 1.0f, 0.3f, 0.15f);
		pEffect->SetValue(hColorParam_, &color, sizeof(D3DXCOLOR));
		pEffect->CommitChanges();
		rc.drawIndexedPrimitive(D3DPT_TRIANGLELIST, vbLockIndex, 0, (UINT)vertices_.size(), ibLockIndex, 2);

		// draw quad outline
		color = D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.1f);
		pEffect->SetValue(hColorParam_, &color, sizeof(D3DXCOLOR));
		pEffect->CommitChanges();
		rc.drawIndexedPrimitive(D3DPT_TRIANGLELIST, vbLockIndex, 0, (UINT)vertices_.size(), ibLockIndex + 6, 8);

		// draw lines inside quad
		if (indices_.size() > 30)
		{
			color = D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.25f);
			pEffect->SetValue(hColorParam_, &color, sizeof(D3DXCOLOR));
			pEffect->CommitChanges();
			rc.drawIndexedPrimitive(D3DPT_TRIANGLELIST, vbLockIndex, 0, (UINT)vertices_.size(), ibLockIndex + 30, (UINT)(indices_.size() - 30) / 3);
		}

		pEffect->EndPass();
	}

	pEffect->End();

	// restore render states
	rc.popRenderState();
	rc.popRenderState();
	rc.popRenderState();
}

void TerrainRulerToolView::updateVertices()
{
	vertices_.clear();
	indices_.clear();

	if (width_ <= 0)
		return;

	Vector3 startPos = startPos_;
	Vector3 endPos = endPos_;
	if (verticalMode_)
	{
		endPos = startPos_;
		endPos.y += (endPos_ - startPos_).length();
	}
	else
	{
		startPos.y += 0.5f;
		endPos.y += 0.5f;
	}

	Vector3 c0, c1, c2, c3;
	if (!calcQuadCorners(startPos, endPos, width_, c0, c1, c2, c3))
		return;

	// inner shape
	addQuad(c0, c1, c2, c3); 

	// outline
	const float outlineWidth = 0.03f;
	addLine(c0, c1, outlineWidth);
	addLine(c1, c2, outlineWidth);
	addLine(c2, c3, outlineWidth);
	addLine(c3, c0, outlineWidth);

	// inner perpendicular lines, from start to end pos, after every 1m
	float innerLine1MLength = width_ * 0.5f;
	float innerLine10MLength = width_ * 0.95f;

	Vector3 moveDir = endPos - startPos;
	float remainingDistance = moveDir.length();
	moveDir.normalise();
	Vector3 lineDir = c3 - c0;
	lineDir.normalise();
	float distanceFromStart = 0;
	int iLine = 0;

	while (remainingDistance > 0)
	{
		if (iLine > 0)
		{
			Vector3 lineCenter = startPos + distanceFromStart * moveDir;
			float lineLength = (iLine % 10 == 0) ? innerLine10MLength : innerLine1MLength;
			float lineWidth = (iLine % 10 == 0) ? 0.1f : 0.05f;
			
			addLine(lineCenter + lineDir * lineLength * 0.5f,
				lineCenter - lineDir * lineLength * 0.5f,
				lineWidth);
		}

		iLine++;
		distanceFromStart += 1.0f;
		remainingDistance -= 1.0f;
	}
}

void TerrainRulerToolView::addLine(const Vector3 &from, const Vector3 &to, float width)
{
	Vector3 c0, c1, c2, c3;
	if (calcQuadCorners(from, to, width, c0, c1, c2, c3))
		addQuad(c0, c1, c2, c3);
}

void TerrainRulerToolView::addQuad(const Vector3 &c0, const Vector3 &c1, const Vector3 &c2, const Vector3 &c3)
{
	size_t baseIndex = vertices_.size();

	Moo::VertexXYZ v;
	Vector3 pos[4] = { c0, c1, c2, c3 };
	for (int i = 0; i < 4; i++)
	{
		v.pos_ = pos[i];
		vertices_.push_back(v);
	}

	ushort idx[6] = { 0, 1, 2, 2, 0, 3 };
	for (int i = 0; i < 6; i++)
		indices_.push_back(static_cast<ushort>(baseIndex + idx[i]));
}

bool TerrainRulerToolView::calcQuadCorners(const Vector3 &a, const Vector3 &b, float width, Vector3 &c0, Vector3 &c1, Vector3 &c2, Vector3 &c3)
{
	//            --ab-->
	//   ^    c0 ---------- c1      -
	//   |    |              |      |
	//  side  a              b    width
	//   |    |              |      |
	//   |    c3 ---------- c2      -

	Vector3 ab = b - a;
	if (ab.lengthSquared() == 0)
		return false;
	
	ab.normalise();	

	Vector3 side;
	if (verticalMode_)
	{
		Matrix viewMatrix = Moo::rc().view();
		viewMatrix.invert();
		side = ab.crossProduct(Vector3(viewMatrix._31, 0, viewMatrix._33));
	}
	else
		side = ab.crossProduct(Vector3(0, 1, 0));
	side.normalise();

	Vector3 halfSide = side * width * 0.5f;
	c0 = a + halfSide;
	c1 = b + halfSide;
	c2 = b - halfSide;
	c3 = a - halfSide;
	return true;
}

void TerrainRulerToolView::enableRender(bool enable)
{
	enableRender_ = enable;
}

void TerrainRulerToolView::startPos(const Vector3 &startPos)
{
	if (startPos_ != startPos)
	{
		startPos_ = startPos;
		vertsDirty_ = true;
	}
}

void TerrainRulerToolView::endPos(const Vector3 &endPos)
{
	if (endPos_ != endPos)
	{
		endPos_ = endPos;
		vertsDirty_ = true;
	}
}

void TerrainRulerToolView::width(float width)
{
	if (width_ != width)
	{
		width_ = width;
		vertsDirty_ = true;
	}
}

void TerrainRulerToolView::verticalMode(bool verticalMode)
{
	if (verticalMode_ != verticalMode)
	{
		verticalMode_ = verticalMode;
		vertsDirty_ = true;
	}
}

PyObject *TerrainRulerToolView::pyNew(PyObject *args)
{
	BW_GUARD;

	return new TerrainRulerToolView();
}
BW_END_NAMESPACE

