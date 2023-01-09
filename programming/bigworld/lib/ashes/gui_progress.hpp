#ifndef GUI_PROGRESS_HPP
#define GUI_PROGRESS_HPP


#include "romp/progress.hpp"
#include "ashes/simple_gui.hpp"


BW_BEGIN_NAMESPACE

class SimpleGUIComponent;

/**
 * TODO: to be documented.
 */
class GUIProgressDisplay : public ProgressDisplay
{
public:
	GUIProgressDisplay( const BW::string& guiName, ProgressCallback pCallback = NULL );
	virtual ~GUIProgressDisplay();

	SmartPointer< SimpleGUIComponent > gui();

	virtual void add( const BW::string & str );
	virtual void append( const BW::string & str );
	virtual bool draw( bool force = false );
protected:	
	virtual void add( ProgressTask & task, const BW::string & name );

	SmartPointer< SimpleGUIComponent >	gui_;
};

BW_END_NAMESPACE

#endif // GUI_PROGRESS_HPP

