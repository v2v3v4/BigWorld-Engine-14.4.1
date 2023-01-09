#ifndef PY_GRAPHICS_SETTING_HPP
#define PY_GRAPHICS_SETTING_HPP

#include "cstdmf/smartpointer.hpp"
#include "moo/graphics_settings.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements a GraphicsSetting that
 *	calls into python when its option is changed.
 *	The callback takes one parameter, which is the newly selected
 *	option for this graphics setting.
 */
class PyCallbackGraphicsSetting : public Moo::GraphicsSetting
{
public:
	PyCallbackGraphicsSetting(
		const BW::string & label, 
		const BW::string & desc,
		int                 activeOption,
		bool                delayed,
		bool                needsRestart,
		PyObject*			callback);

	~PyCallbackGraphicsSetting()	{};

	//callback from the graphics settings system.
	void onOptionSelected(int optionIndex);

	void pCallback( PyObjectPtr pc );
	PyObjectPtr pCallback() const;

private:
	PyObjectPtr	pCallback_;
};


typedef SmartPointer<PyCallbackGraphicsSetting> PyCallbackGraphicsSettingPtr;


/*~ class BigWorld.PyGraphicsSetting
 *	@components{ client,  tools }
 *
 *	This class is a graphics setting that calls into python
 *	to allow arbitrary script code implementation.
 *
 *  Each GraphicsSetting object offers a number of options.
 *	There is always one and only one active option at any one time.
 *
 *	The callback function is called whenever the option
 *	is changed.  It takes one parameter, which is the newly selected
 *	option for this graphics setting.
 */
/**
 *	This class is a graphics setting that calls into python
 *	to allow arbitrary script code implementation.
 *
 *  Each GraphicsSetting object offers a number of options.
 *	There is always one and only one active option at any one time.
 *
 *	The callback function is called whenever the option
 *	is changed.  It takes one parameter, which is the newly selected
 *	option for this graphics setting.
 */
class PyGraphicsSetting : public PyObjectPlus
{
	Py_Header( PyGraphicsSetting, PyObjectPlus )

public:
	PyGraphicsSetting( PyCallbackGraphicsSettingPtr pSetting, PyTypeObject * pType = &s_type_ );
	~PyGraphicsSetting() {};

	int addOption( const BW::string & label, const BW::string & desc, bool isSupported, bool isAdvanced );
	void registerSetting();
	void onOptionSelected(int optionIndex);

	void pCallback( PyObjectPtr pc ){ pSetting_->pCallback(pc); }
	PyObjectPtr pCallback() const	{ return pSetting_->pCallback(); }

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( PyObjectPtr, pCallback, callback )
	PY_AUTO_METHOD_DECLARE( RETDATA, addOption,
		ARG(BW::string,
		ARG(BW::string,
		ARG(bool,
		ARG(bool, END ) ) ) ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, registerSetting, END );

	PY_FACTORY_DECLARE()

protected:
	PyCallbackGraphicsSettingPtr		pSetting_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyGraphicsSetting )

typedef SmartPointer<PyGraphicsSetting>	PyGraphicsSettingPtr;

BW_END_NAMESPACE

#endif // PY_GRAPHICS_SETTING_HPP
