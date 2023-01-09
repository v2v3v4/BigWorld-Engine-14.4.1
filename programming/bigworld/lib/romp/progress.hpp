#ifndef PROGRESS_HPP
#define PROGRESS_HPP

#include "cstdmf/bw_vector.hpp"

#include "font.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/progress.hpp"


BW_BEGIN_NAMESPACE

class ProgressTaskCB
{
public:
	virtual void onCancelProgress() = 0;
};

/**
 *	This class keeps track of the progress of a single task
 */
class ProgressTask : public Progress
{
public:
	ProgressTask( class ProgressDisplay * pOwner,
		const BW::string & name,
		float length = 0 );				// length <=0 means indeterminate time

	virtual ~ProgressTask();


	bool step( float progress = 1 );	// add this amount to done

	bool set( float done = 0 );			// set to this much done

	void length( float length )		{ length_ = length; }

	bool isCancelled();

	void registerCallback( ProgressTaskCB* pCallback );

	void onCancel();


private:
	class ProgressDisplay			* pOwner_;	// the ProgressDisplay we belong to
	bool							isCancelled_;
	BW::vector<ProgressTaskCB*>	callbacks_;

	float			done_;				// work done so far
	float			length_;			// total work to do

	void			detach()		{ pOwner_ = NULL; }

	// for indeterminate time progress tasks, length_ is always done_ + 1.f

	friend class ProgressDisplay;
	friend class SuperModelProgressDisplay;
	friend class GUIProgressDisplay;
};


/**
 *	This class manages and displays a group of ProgressTasks.
 */
class ProgressDisplay
{
public:
	typedef bool (*ProgressCallback)();

	ProgressDisplay( FontPtr pFont = NULL,
		ProgressCallback pCallback = NULL,
		uint32 colour = 0xFF26D1C7 );

	virtual ~ProgressDisplay();

	virtual void		add( const BW::string & str );
	virtual void		append( const BW::string & str );
	virtual bool		draw( bool force = false );

protected:

	FontPtr				pFont_;
	ProgressCallback	pCallback_;
	uint32				colour_;

	void				drawBar( int row, float fraction );
	void				onCancel();
	virtual void		checkCancel() {}

	// called by progress tasks
	bool				onProgress();
	virtual void		add( ProgressTask & task, const BW::string & name );
	virtual void		del( ProgressTask & task );

	/**
	 * TODO: to be documented.
	 */
	struct ProgressNode
	{
		ProgressTask*		task;
		BW::string			name;
		BW::vector<int>	children;
		int					level;
	};

	BW::vector<ProgressNode> tasks_;

	int					deepestNode_;
	BW::vector<int>	roots_;

	uint64				lastDrawn_;
	uint64				minRedrawTime_;

	float				rowHeight_;
	float				rowSep_;

	BW::vector<BW::string>	messages_;

	friend class ProgressTask;
};

BW_END_NAMESPACE

#endif
/*progress.hpp*/
