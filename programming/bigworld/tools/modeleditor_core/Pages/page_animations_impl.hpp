#pragma once

#include "controls/image_button.hpp"
#include "controls/edit_commit.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/slider.hpp"
#include "guimanager/gui_manager.hpp"

BW_BEGIN_NAMESPACE

class PageAnimations;

struct PageAnimationsImpl: public SafeReferenceCount
{
	explicit PageAnimationsImpl( PageAnimations * pPage ) :
		animSubscriber_( NULL ),
		compAnimSubscriber_( NULL ),
		ready( false ),
		inited( false ),
		updating( false ),
		selectClicked( false ),
		updateCount( -1 ),
		reentryFilter( 0 ),
		wasPlaying( false ),
		lastLockedParents( -1 ),
		lastFrameNum( 0 ),
		lastItem( NULL ),
		nodeItem( NULL ),
		compChanged( false  ),
		animChanged( false )
	{
		first.autoSelect( true );
		last.autoSelect( true );

		frameRate.SetNumericType( controls::EditNumeric::ENT_INTEGER );
		frameRate.SetMinimum( 1 );
		frameRate.SetMaximum( 120 );
		frameRate.SetAllowNegative( false );

		frameNum.SetNumericType( controls::EditNumeric::ENT_INTEGER );
		frameNum.SetMinimum( 0 );
		frameNum.SetMaximum( 100 );
		frameNum.SetAllowNegative( false );

		blend.SetNumericType( controls::EditNumeric::ENT_FLOAT );
		blend.SetMinimum( 0.f );
		blend.SetMaximum( 999.f );
		blend.SetAllowNegative( false );

		s_currPage = pPage;
	}

	static PageAnimations* s_currPage;

	GUI::SubscriberPtr animSubscriber_;
	GUI::SubscriberPtr compAnimSubscriber_;

	bool ready;
	bool inited;
	bool updating;
	bool selectClicked;
	int updateCount;

	int reentryFilter;

	bool wasPlaying;

	int lastLockedParents;

	BW::string modelName;
	BW::string fileName;
	BW::string lastAnim;

	int lastFrameNum;
	
	HTREEITEM lastItem;

	HTREEITEM nodeItem;
	BW::string nodeName;

	CToolBarCtrl toolbar;
	controls::EditCommit name;
	CEdit source;
	CButton change_anim;
	controls::EditNumeric frameRate;
	
	controls::EditCommit first;
	controls::EditCommit last;
	controls::EditNumeric frameNum;
	controls::Slider frameNumSlider;

	controls::Slider frameRateSlider;
	CButton frameRateSave;

	CWnd nodeBox;
	CTreeCtrl nodeTree;
	CStatic blendText;
	controls::EditNumeric blend;
	controls::Slider blendSlider;
	CButton blendRemove;

	CWnd compBox;
	controls::Slider compPosSldr;
	controls::Slider compRotSldr;
	controls::Slider compScaleSldr;

	controls::ImageButton compPosMinus;
	controls::ImageButton compRotMinus;
	controls::ImageButton compScaleMinus;

	controls::ImageButton compPosPlus;
	controls::ImageButton compRotPlus;
	controls::ImageButton compScalePlus;

	CStatic compPos;
	CStatic compRot;
	CStatic compScale;

	CStatic compPosText;
	CStatic compRotText;
	CStatic compScaleText;

	CStatic compTotal;

	CToolBarCtrl compToolbar;

	bool compChanged;
	bool animChanged;
};

BW_END_NAMESPACE

