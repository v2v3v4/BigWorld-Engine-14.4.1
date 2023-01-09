#ifndef PP_PREVIEW_HPP
#define PP_PREVIEW_HPP

#include "cstdmf/init_singleton.hpp"
#include "romp/py_render_target.hpp"
#include "post_processing/debug.hpp"

BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	class Phase;
};

class PhaseNodeView;
typedef SmartPointer<PhaseNodeView> PhaseNodeViewPtr;

class PPPreview : public PostProcessing::Debug
{
	Py_Header( PPPreview, Debug )
public:
	PPPreview( PyTypeObject *pType = &s_type_ );
	~PPPreview();

	void edLayout( RECT& clientRect, const CPoint& offset, BW::vector<PhaseNodeView*>& pnvVector );
	bool phaseRect( const PostProcessing::Phase *, CRect& out );
	bool phaseRectForResults( const PostProcessing::Phase *, CRect& out );
	void copyResults();
	bool isVisible( PhaseNodeView* pnv ) const;

	virtual void beginChain( uint32 nEffects );
	virtual void beginEffect( uint32 nPhases );
	virtual RECT nextPhase();
	virtual Vector4 phaseUV( uint32 effect, uint32 phase, uint32 nEffects, uint32 nPhases );

	PyRenderTargetPtr pRT() const	{ return previewRT_; }
	ComObjectWrap<DX::Texture> pPreview()	{ return pSystemTex_; }

	PY_FACTORY_DECLARE()
private:
	PyRenderTargetPtr previewRT_;
	ComObjectWrap<DX::Texture> pSystemTex_;
	typedef BW::map<const PostProcessing::Phase*, CRect> LayoutMap;
	LayoutMap layoutMap_;
	LayoutMap resultsLayoutMap_;
};

typedef SmartPointer<PPPreview> PPPreviewPtr;

BW_END_NAMESPACE

#endif //#ifndef PP_PREVIEW_HPP
