#include "pch.hpp"

#include "worldeditor/gui/dialogs/noise_setup_dlg.hpp"

#include "controls/utils.hpp"

BW_BEGIN_NAMESPACE

namespace
{
    // The size of the sample picture:
    const size_t IMG_WIDTH          = 128;
    const size_t IMG_HEIGHT         = 128;

    // The range that the image represents:
    const float  IMG_WORLD_WIDTH    = 100.0f;
    const float  IMG_WORLD_HEIGHT   = 100.0f;
    
    // Maximum range of the octaves:
    const float  MAX_OCTAVE_RANGE   = 500.0f;
}


BEGIN_MESSAGE_MAP(NoiseSetupDlg, CDialog)
    ON_WM_HSCROLL()
    ON_EN_CHANGE(IDC_WAVELEN_EDIT1    , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WAVELEN_EDIT2    , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WAVELEN_EDIT3    , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WAVELEN_EDIT4    , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WAVELEN_EDIT5    , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WAVELEN_EDIT6    , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WAVELEN_EDIT7    , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WAVELEN_EDIT8    , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WEIGHT_EDIT1     , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WEIGHT_EDIT2     , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WEIGHT_EDIT3     , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WEIGHT_EDIT4     , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WEIGHT_EDIT5     , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WEIGHT_EDIT6     , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WEIGHT_EDIT7     , OnOctaveEdit)
    ON_EN_CHANGE(IDC_WEIGHT_EDIT8     , OnOctaveEdit)
    ON_EN_CHANGE(IDC_SEED_EDIT1       , OnOctaveEdit)
    ON_EN_CHANGE(IDC_SEED_EDIT2       , OnOctaveEdit)
    ON_EN_CHANGE(IDC_SEED_EDIT3       , OnOctaveEdit)
    ON_EN_CHANGE(IDC_SEED_EDIT4       , OnOctaveEdit)
    ON_EN_CHANGE(IDC_SEED_EDIT5       , OnOctaveEdit)
    ON_EN_CHANGE(IDC_SEED_EDIT6       , OnOctaveEdit)
    ON_EN_CHANGE(IDC_SEED_EDIT7       , OnOctaveEdit)
    ON_EN_CHANGE(IDC_SEED_EDIT8       , OnOctaveEdit)
    ON_EN_CHANGE(IDC_MIN_STRENGTH_EDIT, OnOctaveEdit)
    ON_EN_CHANGE(IDC_MAX_STRENGTH_EDIT, OnOctaveEdit)
	ON_EN_CHANGE(IDC_MIN_SAT_EDIT     , OnOctaveEdit)
	ON_EN_CHANGE(IDC_MAX_SAT_EDIT     , OnOctaveEdit)
END_MESSAGE_MAP()


/**
 *  The NoiseSetupDlg constructor.
 *
 *  @param parent   The parent window.
 */
/*explicit*/ NoiseSetupDlg::NoiseSetupDlg(CWnd *parent /*= NULL*/):
    CDialog(IDD, parent),
	minSat_(0.0f),
	maxSat_(1.0f),
	minStrength_(0.0f),
	maxStrength_(1.0f),
    filter_(0),
    inited_(false)
{
	BW_GUARD;

    octaves_.resize(8);
    for (size_t i = 0; i < octaves_.size(); ++i)
    {
        octaves_[i] = Octave(0.0f, 0.0f);
    }

    noise_.octaves(octaves_);
}


/**
 *  Get the SimplexNoise that was edited.
 *
 *  @returns        The edited SimplexNoise.
 */
SimplexNoise const &NoiseSetupDlg::simplexNoise() const
{
    return noise_;
}


/**
 *  Set the SimplexNoise to edit.
 *
 *  @param noise    The noise to edit.
 */
void NoiseSetupDlg::simplexNoise(SimplexNoise const &noise)
{
    noise_ = noise;
}


/**
 *  This gets the raw octaves, including 0 values.
 *
 *  @returns        The raw octave values.
 */ 
BW::vector<NoiseSetupDlg::Octave> const &NoiseSetupDlg::octaves() const
{
    return octaves_;
}


/**
 *  This sets the raw octaves.
 *
 *  @param oct      The octave values.
 */
void NoiseSetupDlg::octaves(BW::vector<Octave> const &oct)
{
	BW_GUARD;

    size_t sz = std::min(oct.size(), (size_t)8);
    for (size_t i = 0; i < sz; ++i)
    {
        octaves_[i] = oct[i];
    }
    for (size_t i = sz; i < 8; ++i)
    {
        octaves_[i] = Octave();
    }
    noise_.octaves(octaves_);
}


/**
 *	This gets the minimum saturation value.
 *
 *	@returns		The minimum saturation value.
 */
float NoiseSetupDlg::minSaturate() const
{
	return minSat_;
}


/**
 *	This sets the minimum saturation value.
 *
 *	@param ms		The new minimum saturation value.
 */
void NoiseSetupDlg::minSaturate(float ms)
{
	minSat_ = ms;
}


/**
 *	This gets the maximum saturation value.
 *
 *	@returns		The maximum saturation value.
 */
float NoiseSetupDlg::maxSaturate() const
{
	return maxSat_;
}


/**
 *	This sets the maximum saturation value.
 *
 *	@param ms		The new maximum saturation value.
 */
void NoiseSetupDlg::maxSaturate(float ms)
{
	maxSat_ = ms;
}


/**
 *	This gets the minimum strength value.
 *
 *	@returns		The minimum strength value.
 */
float NoiseSetupDlg::minStrength() const
{
	return minStrength_;
}


/**
 *	This sets the minimum strength value.
 *
 *	@param ms		The new minimum strength value.
 */
void NoiseSetupDlg::minStrength(float strength)
{
	minStrength_ = strength;
}


/**
 *	This gets the maximum strength value.
 *
 *	@returns		The maximum strength value.
 */
float NoiseSetupDlg::maxStrength() const
{
	return maxStrength_;
}


/**
 *	This sets the maximum strength value.
 *
 *	@param ms		The new maximum strength value.
 */
void NoiseSetupDlg::maxStrength(float strength)
{
	maxStrength_ = strength;
}


/**
 *	This sets up a default noise pattern.
 *
 *	@param noise	This is filled with the default noise pattern.
 */
/*static*/ void NoiseSetupDlg::defaultNoise( SimplexNoise & noise )
{
	BW_GUARD;

	SimplexNoise::OctaveVec octaves;

	float waveLength = 100.f;  // TODO: Should this be DEFAULT_GRID_RESOLUTION?
	float weight     = 100.0f; // This fits the slider values nicely.

	for (unsigned int i = 0; i < 8; ++i, waveLength *= 0.5f, weight *= 0.5f)
	{
		SimplexNoise::Octave octave( waveLength, weight );
		octaves.push_back( octave );
	}
	noise.octaves( octaves );
}


/**
 *  This is called to initialise the dialog.
 *
 *  @returns        TRUE if the dialog could be initialised, FALSE otherwise.
 */
/*virtual*/ BOOL NoiseSetupDlg::OnInitDialog()
{
	BW_GUARD;

    BOOL result = CDialog::OnInitDialog();

    // Replace the dummy static with our image control:
   	CRect imgExt = 
		controls::childExtents(*this, IDC_NOISE_IMG);
	CWnd *imgStatic = GetDlgItem(IDC_NOISE_IMG);
	imgStatic->ShowWindow(SW_HIDE);
	imageControl_.Create
	(
		WS_CHILD | WS_VISIBLE, 
		imgExt, 
		this, 
		IDC_NOISE_IMG
	); 
    updateNoiseImage();

    ++filter_;

    for (size_t i = 0; i < 8; ++i)
    {
        waveLenSlider_[i].setDigits(1);
        waveLenSlider_[i].setRange(0.0f, MAX_OCTAVE_RANGE);
        waveLenSlider_[i].setValue(octaves_[i].waveLength_);        
        
        waveLenEdit_[i].SetNumDecimals(1);
        waveLenEdit_[i].SetValue(octaves_[i].waveLength_);

        weightSlider_[i].setDigits(1);
        weightSlider_[i].setRange(0.0f, 100.0f);
        weightSlider_[i].setValue(octaves_[i].weight_);        
        
        weightEdit_[i].SetNumDecimals(1);
        weightEdit_[i].SetValue(octaves_[i].weight_);

        seedEdit_[i].SetNumericType(controls::EditNumeric::ENT_INTEGER);
        seedEdit_[i].SetValue((float)octaves_[i].seed_);
    }

    rangeMinSlider_.setDigits(1);
    rangeMinSlider_.setRange(0.0f, 100.0f);
    rangeMinSlider_.setValue(100.0f*minStrength_);

    rangeMinEdit_.SetNumDecimals(1);
    rangeMinEdit_.SetValue(100.0f*minStrength_);

    rangeMaxSlider_.setDigits(1);
    rangeMaxSlider_.setRange(0.0f, 100.0f);
    rangeMaxSlider_.setValue(100.0f*maxStrength_);

    rangeMaxEdit_.SetNumDecimals(1);
    rangeMaxEdit_.SetValue(100.0f*maxStrength_);

    satMinSlider_.setDigits(1);
    satMinSlider_.setRange(0.0f, 100.0f);
    satMinSlider_.setValue(100.0f*minSat_);

    satMinEdit_.SetNumDecimals(1);
    satMinEdit_.SetValue(100.0f*minSat_);

    satMaxSlider_.setDigits(1);
    satMaxSlider_.setRange(0.0f, 100.0f);
    satMaxSlider_.setValue(100.0f*maxSat_);

    satMaxEdit_.SetNumDecimals(1);
    satMaxEdit_.SetValue(100.0f*maxSat_);

    inited_ = true;

    --filter_;

    INIT_AUTO_TOOLTIP();

    return result;
}


/*virtual*/ void NoiseSetupDlg::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_WAVELEN_SLIDER1    , waveLenSlider_[0]);
    DDX_Control(pDX, IDC_WAVELEN_SLIDER2    , waveLenSlider_[1]);
    DDX_Control(pDX, IDC_WAVELEN_SLIDER3    , waveLenSlider_[2]);
    DDX_Control(pDX, IDC_WAVELEN_SLIDER4    , waveLenSlider_[3]);
    DDX_Control(pDX, IDC_WAVELEN_SLIDER5    , waveLenSlider_[4]);
    DDX_Control(pDX, IDC_WAVELEN_SLIDER6    , waveLenSlider_[5]);
    DDX_Control(pDX, IDC_WAVELEN_SLIDER7    , waveLenSlider_[6]);
    DDX_Control(pDX, IDC_WAVELEN_SLIDER8    , waveLenSlider_[7]);

    DDX_Control(pDX, IDC_WAVELEN_EDIT1      , waveLenEdit_  [0]);
    DDX_Control(pDX, IDC_WAVELEN_EDIT2      , waveLenEdit_  [1]);
    DDX_Control(pDX, IDC_WAVELEN_EDIT3      , waveLenEdit_  [2]);
    DDX_Control(pDX, IDC_WAVELEN_EDIT4      , waveLenEdit_  [3]);
    DDX_Control(pDX, IDC_WAVELEN_EDIT5      , waveLenEdit_  [4]);
    DDX_Control(pDX, IDC_WAVELEN_EDIT6      , waveLenEdit_  [5]);
    DDX_Control(pDX, IDC_WAVELEN_EDIT7      , waveLenEdit_  [6]);
    DDX_Control(pDX, IDC_WAVELEN_EDIT8      , waveLenEdit_  [7]);

    DDX_Control(pDX, IDC_WEIGHT_SLIDER1     , weightSlider_ [0]);
    DDX_Control(pDX, IDC_WEIGHT_SLIDER2     , weightSlider_ [1]);
    DDX_Control(pDX, IDC_WEIGHT_SLIDER3     , weightSlider_ [2]);
    DDX_Control(pDX, IDC_WEIGHT_SLIDER4     , weightSlider_ [3]);
    DDX_Control(pDX, IDC_WEIGHT_SLIDER5     , weightSlider_ [4]);
    DDX_Control(pDX, IDC_WEIGHT_SLIDER6     , weightSlider_ [5]);
    DDX_Control(pDX, IDC_WEIGHT_SLIDER7     , weightSlider_ [6]);
    DDX_Control(pDX, IDC_WEIGHT_SLIDER8     , weightSlider_ [7]);

    DDX_Control(pDX, IDC_WEIGHT_EDIT1       , weightEdit_   [0]);
    DDX_Control(pDX, IDC_WEIGHT_EDIT2       , weightEdit_   [1]);
    DDX_Control(pDX, IDC_WEIGHT_EDIT3       , weightEdit_   [2]);
    DDX_Control(pDX, IDC_WEIGHT_EDIT4       , weightEdit_   [3]);
    DDX_Control(pDX, IDC_WEIGHT_EDIT5       , weightEdit_   [4]);
    DDX_Control(pDX, IDC_WEIGHT_EDIT6       , weightEdit_   [5]);
    DDX_Control(pDX, IDC_WEIGHT_EDIT7       , weightEdit_   [6]);
    DDX_Control(pDX, IDC_WEIGHT_EDIT8       , weightEdit_   [7]);

    DDX_Control(pDX, IDC_SEED_EDIT1         , seedEdit_     [0]);
    DDX_Control(pDX, IDC_SEED_EDIT2         , seedEdit_     [1]);
    DDX_Control(pDX, IDC_SEED_EDIT3         , seedEdit_     [2]);
    DDX_Control(pDX, IDC_SEED_EDIT4         , seedEdit_     [3]);
    DDX_Control(pDX, IDC_SEED_EDIT5         , seedEdit_     [4]);
    DDX_Control(pDX, IDC_SEED_EDIT6         , seedEdit_     [5]);
    DDX_Control(pDX, IDC_SEED_EDIT7         , seedEdit_     [6]);
    DDX_Control(pDX, IDC_SEED_EDIT8         , seedEdit_     [7]);

    DDX_Control(pDX, IDC_MIN_STRENGTH_SLIDER, rangeMinSlider_);
    DDX_Control(pDX, IDC_MAX_STRENGTH_SLIDER, rangeMaxSlider_);

    DDX_Control(pDX, IDC_MIN_STRENGTH_EDIT  , rangeMinEdit_  );
    DDX_Control(pDX, IDC_MAX_STRENGTH_EDIT  , rangeMaxEdit_  );

    DDX_Control(pDX, IDC_MIN_SAT_SLIDER     , satMinSlider_  );
    DDX_Control(pDX, IDC_MAX_SAT_SLIDER     , satMaxSlider_  );

    DDX_Control(pDX, IDC_MIN_SAT_EDIT       , satMinEdit_    );
    DDX_Control(pDX, IDC_MAX_SAT_EDIT       , satMaxEdit_    );
}


/**
 *  This is called when the user moves a slider.
 *
 *  @param sBCode       The scroll-bar code.
 *  @param pos          The position of the scroll-bar.
 *  @param scrollBar    The moved scroll-bar.
 */
void NoiseSetupDlg::OnHScroll(UINT sBCode, UINT pos, CScrollBar *scrollBar)
{
	BW_GUARD;

    if (inited_ && filter_ == 0 && scrollBar != NULL)
    {
        ++filter_;

  		for (size_t i = 0; i < 8; ++i)
 		{
 			float freq = waveLenSlider_[i].getValue();
 			waveLenEdit_[i].SetValue(freq);
			octaves_[i].waveLength_ = freq;

            float weight = weightSlider_[i].getValue();
            weightEdit_[i].SetValue(weight);
            octaves_[i].weight_ = weight;
     	}

 		minStrength_ = rangeMinSlider_.getValue();
  		rangeMinEdit_.SetValue(minStrength_);
  		minStrength_ *= 0.01f;

		maxStrength_ = rangeMaxSlider_.getValue();
		rangeMaxEdit_.SetValue(maxStrength_);
		maxStrength_ *= 0.01f;

		minSat_ = satMinSlider_.getValue();
		satMinEdit_.SetValue(minSat_);
		minSat_ *= 0.01f;

		maxSat_ = satMaxSlider_.getValue();
		satMaxEdit_.SetValue(maxSat_);
		maxSat_ *= 0.01f;

		updateNoiseImage();
    
        --filter_;
    }
}


/**
 *  This is called when the user changes an octave edit control.
 */
void NoiseSetupDlg::OnOctaveEdit()
{
	BW_GUARD;

    if (filter_ == 0)
    {
        ++filter_;

        for (size_t i = 0; i < 8; ++i)
        {
            float freq = waveLenEdit_[i].GetValue();
            waveLenSlider_[i].setValue(freq);
            octaves_[i].waveLength_ = freq;

            float weight = weightEdit_[i].GetValue();
            weightSlider_[i].setValue(weight);
            octaves_[i].weight_ = weight;

            int seed = (int)seedEdit_[i].GetValue();
            octaves_[i].seed_ = seed;
        }

        minStrength_ = rangeMinEdit_.GetValue();
        rangeMinSlider_.setValue(minStrength_);
        minStrength_ *= 0.01f;

        maxStrength_ = rangeMaxEdit_.GetValue();
        rangeMaxSlider_.setValue(maxStrength_);
        maxStrength_ *= 0.01f;

		minSat_ = satMinEdit_.GetValue();
		satMinSlider_.setValue(minSat_);
		minSat_ *= 0.01f;

		maxSat_ = satMaxEdit_.GetValue();
		satMaxSlider_.setValue(maxSat_);
		maxSat_ *= 0.01f;

        updateNoiseImage();

        --filter_;
    }
}


/**
 *  This updates the noise image to match the value in the controls.
 */
void NoiseSetupDlg::updateNoiseImage()
{
	BW_GUARD;

    noise_.octaves(octaves_);

    controls::DibSection8 &image = imageControl_.image();
    image.resize(IMG_WIDTH, IMG_HEIGHT);

    float dx = IMG_WORLD_WIDTH /IMG_WIDTH ;
    float dy = IMG_WORLD_HEIGHT/IMG_HEIGHT;

	float minSat = std::min(minSat_, maxSat_);
	float maxSat = std::max(minSat_, maxSat_);

    float wy = -0.5f*IMG_WORLD_HEIGHT;
    for (uint32 y = 0; y < IMG_HEIGHT; ++y, wy += dy)
    {
        float wx = -0.5f*IMG_WORLD_WIDTH;
        uint8 *p = image.getRow(y);
        uint8 *q = p + IMG_WIDTH;
        for (; p != q; ++p, wx += dx)
        {
            float ns = noise_(wx, wy);
			if (ns < minSat) 
				ns = minStrength_;
			else if (ns > maxSat)
				ns = maxStrength_;
			else 
				ns = Math::lerp(ns, minSat, maxSat, minStrength_, maxStrength_);
            *p = (uint8)(ns*255.0f);
        }
    }

    imageControl_.image(image);
    imageControl_.Invalidate(0);
}
BW_END_NAMESPACE

