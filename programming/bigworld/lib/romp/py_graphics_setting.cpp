#include "pch.hpp"
#include "py_graphics_setting.hpp"
#include "moo/graphics_settings_picker.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyCallbackGraphicsSetting
// -----------------------------------------------------------------------------
PyCallbackGraphicsSetting::PyCallbackGraphicsSetting(
	const BW::string & label, 
	const BW::string & desc,
	int                 activeOption,
	bool                delayed,
	bool                needsRestart,
	PyObject*			callback):
	GraphicsSetting( label, desc, activeOption, delayed, needsRestart ),
	pCallback_( callback, false )
{
}


/**
 *	This method is called by the Graphics Settings system when our
 *	setting is changed.  The newly selected option is passed in.
 *
 *	@param	optionIndex		The newly selected option in our graphics setting.
 */
void PyCallbackGraphicsSetting::onOptionSelected(int optionIndex)
{
	BW_GUARD;
	if (pCallback_.hasObject())
	{
		Py_INCREF( pCallback_.getObject() );
		Script::call( pCallback_.getObject(), Py_BuildValue( "(i)", optionIndex ),
			"PyCallbackGraphicsSetting::onOptionSelected" );
	}
}


void PyCallbackGraphicsSetting::pCallback( PyObjectPtr pc )
{
	pCallback_ = pc;
}


PyObjectPtr PyCallbackGraphicsSetting::pCallback() const
{
	return pCallback_;
}


// -----------------------------------------------------------------------------
// Section: PyGraphicsSetting
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyGraphicsSetting )
PY_BEGIN_METHODS( PyGraphicsSetting )
	/*~ function PyGraphicsSetting.addOption
	 *	@components{ client, tools }
	 * This method adds an option to the graphics setting.
	 *	@param	label		UI label to display for the option
	 *	@param	desc		More detailed help string for the option.
	 *	@param	isSupported Whether the current host system supports this option.
	 *	@return	int			Index to the newly added option.
	 */
	PY_METHOD( addOption )
	/*~ function PyGraphicsSetting.registerSetting
	 *	@components{ client, tools }
	 * This method registers the graphics setting with the Graphics Setting
	 * registry.  It should be called only after options have been added to
	 * the graphics setting.
	 */
	PY_METHOD( registerSetting )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( PyGraphicsSetting )
	/*~ attribute PyGraphicsSetting.callback
	 *	@components{ client, tools }
	 * This attribute specifies the callback function.  The function is called
	 * when this graphics setting has a new option selected. The callback function
	 * takes a single Integer parameter, the newly selected option index.
	 */
	PY_ATTRIBUTE( callback )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyGraphicsSetting )

/*~ function BigWorld.GraphicsSetting
 *	@components{ client, tools }
 *
 *	This function creates a PyGraphicsSetting.
 *
 *	For example:
 *	@{
 *	def onOptionSelected( idx ):
 *		print "graphics setting option %d selected" % (idx,)
 *
 *	gs = BigWorld.GraphicsSetting( "label", "description", -1, False, False, onOptionSelected )
 *	gs.addOption( "High", "High Quality", True, False )
 *	gs.addOption( "Low", "Low Quality", True, False )
 *	gs.registerSetting()	#always add after the options have been added.
 *
 *	@}
 *
 *	@param		String		Short label to display in the UI.
 *	@param		String		Longer description to display in the UI.
 *	@param		Int			Active option index.	When this setting is added
 *							to the registry, the active option will be either
 *							reset to the first supported option or to the one
 *							restored from the options file, if the provided
 *							active option index is -1.  If, instead, the
 *							provided activeOption is a non negative value, it
 *							will either be reset to the one restored from the
 *							options file (if this happens to be different from
 *							the value passed as parameter) or not be reset at
 *							all.
 *	@param		Boolean		Delayed - apply immediately on select, or batched up.
 *	@param		Boolean		Requires Restart - game needs a restart to take effect.
 *	@param		PyObject	Callback function.
 *
 *	@return		a new PyGraphicsSetting
 */
PY_FACTORY_NAMED( PyGraphicsSetting, "GraphicsSetting", BigWorld )


//constructor
PyGraphicsSetting::PyGraphicsSetting(
	PyCallbackGraphicsSettingPtr pSetting,
	PyTypeObject * pType ):
	PyObjectPlus( pType ),
	pSetting_( pSetting )
{
	BW_GUARD;
}


/**
 *	This method is callable from python, and adds an option to
 *	our graphics setting.
 *	@param	label		UI label to display for the option
 *	@param	desc		More detailed help string for the option.
 *	@param	isSupported Whether the current host system supports this option.
 *  @param  isAdvanced  True if this option is supported only be the advanced renderer pipeline.
 *	@return	int			Index to the newly added option.
 */
int PyGraphicsSetting::addOption(
	const BW::string & label,
	const BW::string & desc,
	bool isSupported,
	bool isAdvanced)
{
	BW_GUARD;
	return pSetting_->addOption( label, desc, isSupported, isAdvanced );
}


/**
 *	This method is callable from python, and registers our graphics
 *	setting.  This should be called after the settings options have
 *	been added.  Once added, there is no need to ever remove.
 */
void PyGraphicsSetting::registerSetting()
{
	BW_GUARD;
	Moo::GraphicsSetting::add( pSetting_ );
}


/**
 *	Factory method.
 */
PyObject * PyGraphicsSetting::pyNew( PyObject * args )
{
	BW_GUARD;
	char * label, * desc;
	int activeOption;
	bool delayed;
	bool needsRestart;
	PyObject* callback;

	if (!PyArg_ParseTuple( args, "ssibbO", &label, &desc, &activeOption, &delayed, &needsRestart, &callback ))
	{		
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.PyGraphicsSetting() expects "
			"a label (string), description (string), default active option "
			"(int), delayed (bool) needsRetart (bool) and callback function "
			"(Python Object)." );
		return NULL;
	}

	PyCallbackGraphicsSettingPtr pSettings = new PyCallbackGraphicsSetting(
		BW::string(label), BW::string(desc), activeOption, delayed, needsRestart, callback );

	return new PyGraphicsSetting( pSettings );
}


//-----------------------------------------------------------------------------
// section - python access to static Moo::GraphicsSetting methods
//-----------------------------------------------------------------------------

/*~ function BigWorld.graphicsSettings
 *	@components{ client,  tools }
 *
 *  Returns list of registered graphics settings
 *	@return	list of 7-tuples in the form of tuple t:
 *			t[0]:	label : string
 *			t[1]:	active_option_index: int 
 *			t[2]:	options list, where each option is a 4-tuple.
 *				// per option opt:
 *				opt[0]:	label : string
 *				opt[1]:	isSupported : bool
 *				opt[2]:	isAdvanced : bool
 *				opt[3]:	description : string
 *			t[3]:	description : string
 *			t[4]:	advanced flag : bool
 *			t[5]:	needsRestart flag : bool
 *			t[6]:	delayed flag : bool
 */
static PyObject * graphicsSettings()
{
	BW_GUARD;
	typedef Moo::GraphicsSetting::GraphicsSettingVector GraphicsSettingVector;
	const GraphicsSettingVector & settings = Moo::GraphicsSetting::settings();
	PyObject * settingsList = PyList_New(settings.size());

	GraphicsSettingVector::const_iterator setIt  = settings.begin();
	GraphicsSettingVector::const_iterator setEnd = settings.end();
	while (setIt != setEnd)
	{
		const Moo::GraphicsSetting::Options& options = (*setIt)->options();
		PyObject * optionsList = PyList_New(options.size());
		Moo::GraphicsSetting::Options::const_iterator optIt  = options.begin();
		Moo::GraphicsSetting::Options::const_iterator optEnd = options.end();
		while (optIt != optEnd)
		{
			PyObject * optionItem = PyTuple_New(4);
			PyTuple_SetItem(optionItem, 0, Script::getData(optIt->m_label));
			PyTuple_SetItem(optionItem, 1, Script::getData(optIt->m_supported));
			PyTuple_SetItem(optionItem, 2, Script::getData(optIt->m_advanced));
			PyTuple_SetItem(optionItem, 3, Script::getData(optIt->m_desc));

			const std::ptrdiff_t optionIndex =
				std::distance(options.begin(), optIt);
			PyList_SetItem(optionsList, optionIndex, optionItem);
			++optIt;
		}
		PyObject * settingItem = PyTuple_New(7);
		PyTuple_SetItem(settingItem, 0, Script::getData((*setIt)->label()));
		PyTuple_SetItem(settingItem, 3, Script::getData((*setIt)->desc()));
		PyTuple_SetItem(settingItem, 4, Script::getData((*setIt)->advanced()));
		PyTuple_SetItem(settingItem, 5, Script::getData((*setIt)->requiresRestart()));
		PyTuple_SetItem(settingItem, 6, Script::getData((*setIt)->delayed()));

		// is setting is pending, use value stored in pending
		// list. Otherwise, use active option in setting
		int activeOption = 0;
		if (!Moo::GraphicsSetting::isPending(*setIt, activeOption))
		{
			activeOption = (*setIt)->activeOption();
		}
		PyTuple_SetItem(settingItem, 1, Script::getData(activeOption));
		PyTuple_SetItem(settingItem, 2, optionsList);

		const std::ptrdiff_t settingIndex =
			std::distance(settings.begin(), setIt);
		PyList_SetItem(settingsList, settingIndex, settingItem);
		++setIt;
	}

	return settingsList;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, graphicsSettings, END, BigWorld )


/*~ function BigWorld.setGraphicsSetting
 *	@components{ client,  tools }
 *
 *  Sets graphics setting option.
 *
 *  Raises a ValueError if the given label does not name a graphics setting, if
 *  the option index is out of range, or if the option is not supported.
 *
 *	@param	label		    string - label of setting to be adjusted.
 *	@param	optionIndex		int - index of option to set.
 */
static bool setGraphicsSetting( const BW::string label, int optionIndex )
{
	BW_GUARD;

	bool result = false;
	typedef Moo::GraphicsSetting::GraphicsSettingVector GraphicsSettingVector;
	GraphicsSettingVector settings = Moo::GraphicsSetting::settings();
	GraphicsSettingVector::const_iterator setIt  = settings.begin();
	GraphicsSettingVector::const_iterator setEnd = settings.end();
	while (setIt != setEnd)
	{
		if ((*setIt)->label() == label)
		{
			if (optionIndex < int( (*setIt)->options().size() ))
			{
				if ((*setIt)->options()[optionIndex].m_supported)
				{
					(*setIt)->selectOption( optionIndex );
					result = true;
				}
				else
				{
					PyErr_SetString( PyExc_ValueError,
						"Option is not supported." );
				}
			}
			else
			{
				PyErr_SetString( PyExc_ValueError,
					"Option index out of range." );
			}
			break;
		}
		++setIt;
	}
	if (setIt == setEnd)
	{
		PyErr_SetString( PyExc_ValueError,
			"No setting found with given label." );
	}

	return result;
}
PY_AUTO_MODULE_FUNCTION( RETOK, setGraphicsSetting,
	ARG( BW::string, ARG( int, END ) ), BigWorld )


/*~ function BigWorld.getGraphicsSetting
 *	@components{ client,  tools }
 *
 *  Gets graphics setting option.
 *
 *  Raises a ValueError if the given label does not name a graphics setting, if
 *  the option index is out of range, or if the option is not supported.
 *
 *	@param	label		    string - label of setting to be retrieved.
 *	@return optionIndex		int - index of option.
 */
static int getGraphicsSetting( const BW::string label )
{
	BW_GUARD;
	bool result = false;
	typedef Moo::GraphicsSetting::GraphicsSettingVector GraphicsSettingVector;
	GraphicsSettingVector settings = Moo::GraphicsSetting::settings();
	GraphicsSettingVector::const_iterator setIt  = settings.begin();
	GraphicsSettingVector::const_iterator setEnd = settings.end();
	while (setIt != setEnd)
	{
		if ((*setIt)->label() == label)
		{
			return ((*setIt)->activeOption());
		}

		++setIt;
	}

	if (setIt == setEnd)
	{
		const BW::string str( "No setting found with given label: " + label );
		PyErr_SetString( PyExc_ValueError, str.c_str() );
	}

	// Return -1, because 0 is a valid option number
	return -1;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, getGraphicsSetting,
	ARG( BW::string, END ), BigWorld )


/*~ function BigWorld.commitPendingGraphicsSettings
 *	@components{ client,  tools }
 *
 *  This function commits any pending graphics settings. Some graphics
 *  settings, because they may block the game for up to a few minutes when
 *  coming into effect, are not committed immediately. Instead, they are
 *  flagged as pending and require commitPendingGraphicsSettings to be called
 *  to actually apply them.
 */
static void commitPendingGraphicsSettings()
{
	BW_GUARD;
	Moo::GraphicsSetting::commitPending();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, commitPendingGraphicsSettings, END, BigWorld )


/*~ function BigWorld.hasPendingGraphicsSettings
 *	@components{ client,  tools }
 *
 *  This function returns true if there are any pending graphics settings.
 *  Some graphics settings, because they may block the game for up to a few
 *  minutes when coming into effect, are not committed immediately. Instead,
 *  they are flagged as pending and require commitPendingGraphicsSettings to be
 *  called to actually apply them.
 */
static bool hasPendingGraphicsSettings()
{
	BW_GUARD;
	return Moo::GraphicsSetting::hasPending();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, hasPendingGraphicsSettings, END, BigWorld )


static bool MRTSupported()
{
	return (((Moo::rc().psVersion() & 0xff00) >> 8) > 2);
}
PY_AUTO_MODULE_FUNCTION( RETDATA, MRTSupported, END, BigWorld )


/*~ function BigWorld.graphicsSettingsNeedRestart
 *	@components{ client,  tools }
 *
 *  This function returns true if any recent graphics setting change
 *	requires the client to be restarted to take effect. If that's the
 *	case, restartGame can be used to restart the client. The need restart
 *	flag is reset when this method is called.
 */
static bool graphicsSettingsNeedRestart()
{
	BW_GUARD;
	return Moo::GraphicsSetting::needsRestart();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, graphicsSettingsNeedRestart, END, BigWorld )

// returns if it is safe to set high textures
static bool isApplyHighQualityPossible()
{
	const int64 minMem = static_cast<int64>(
		Moo::TextureManager::textureMemoryBlock());
	if(minMem == 0)
		return true;
	return Moo::GraphicsSettingsAutoDetector::getTotalVirtualMemory() >= minMem;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isApplyHighQualityPossible, END, BigWorld )

/*~ function BigWorld.graphicsSettingsWereForcedToReset
*	@components{ client,  tools }
*
*	The graphics settings were AUTOMATICALLY REDETECTED and APPLIED 
*	based on the client's system properties.
*/
static bool graphicsSettingsWereForcedToReset()
{
	return Moo::GraphicsSetting::wereForcedToReset();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, graphicsSettingsWereForcedToReset, END, BigWorld )

/*~ function BigWorld.autoDetectGraphicsSettings
 *	@components{ client,  tools }
 *  
 *	Automatically detect the graphics settings
 *	based on the client's system properties.
 */
static int autoDetectGraphicsSettings()
{
	BW_GUARD;
	Moo::GraphicsSettingsAutoDetector detector;
	detector.init();
	return (int)detector.detectBestSettings();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, autoDetectGraphicsSettings, END, BigWorld )

//reverts graphics settings to the previous status
static void revertGraphicsSettings()
{
	Moo::GraphicsSetting::revertSettings();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, revertGraphicsSettings, END, BigWorld )


/*~ function BigWorld.rollBackPendingGraphicsSettings
 *	@components{ client,  tools }
 *
 *  This function rolls back any pending graphics settings.
 */
static void rollBackPendingGraphicsSettings()
{
	BW_GUARD;
	return Moo::GraphicsSetting::rollbackPending();
}
PY_AUTO_MODULE_FUNCTION( RETVOID,
		rollBackPendingGraphicsSettings, END, BigWorld )


BW_END_NAMESPACE

// py_graphics_setting.cpp
