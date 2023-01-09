#include "pch.hpp"

#include "debug.hpp"
#include "guard.hpp"
#include "main_loop_task.hpp"

DECLARE_DEBUG_COMPONENT2( "CStdMF", 0 )

//#define DISPLAY_MEMORY

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: MainLoopTasks
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
MainLoopTasks::MainLoopTasks() :
	initted_( false ),
	finished_( false )
{
}


/**
 *	Destructor.
 */
MainLoopTasks::~MainLoopTasks()
{
	if (initted_) this->fini();
}

#ifdef DISPLAY_MEMORY
extern uint32 memUsed();
extern uint32 memoryAccountedFor();
static BW::string s_levels;
#endif

/**
 *	Call init methods in order
 */
bool MainLoopTasks::init()
{
	// first copy the tasks to init in case others are added or the
	// order changes or some such
	BW::vector<MainLoopTask*> toinit;
	toinit.reserve( order_.size() );
#ifdef DISPLAY_MEMORY
	BW::vector<const char*> toinitNames;
	toinitNames.reserve( order_.size() );
#endif
	for (OrderList::iterator it = order_.begin(); it != order_.end(); it++)
	{
		toinit.push_back( (tasks_.begin() + *it)->second );
#ifdef DISPLAY_MEMORY
		toinitNames.push_back( (tasks_.begin() + *it)->first );
#endif
	}


	bool initResult = true;
	for (uint i = 0; i < toinit.size(); i++)
	{
#ifdef DISPLAY_MEMORY
		uint osll = s_levels.length();
		s_levels.append( "/" );
		s_levels.append( toinitNames[i] );
		bool ok = toinit[i]->init();
		s_levels.erase( osll );
		if (!ok) return false;
		dprintf( "After %s/%s using %d/%dKB\n",
			s_levels.c_str(), toinitNames[i], memoryAccountedFor(), memUsed() );
#else
		//finish the initialization of all tasks, no matter successful of not,
		//so we can call fini() correctly if one of the tasks failed.
		initResult &= toinit[i]->init();
#endif
	}

	// change initted to true so any new tasks will get initted
	initted_ = true;
	return initResult;
}

/**
 *	Call fini methods in order
 */
void MainLoopTasks::fini()
{
	for ( size_t i = order_.size(); i > 0; --i )
		(tasks_.begin() + order_[i - 1])->second->fini();
	initted_ = false;
	finished_ = true;
}

/*static*/ void MainLoopTasks::finiAll()
{
	if (MainLoopTasks::root().initted())
	{
		MainLoopTasks::root().fini();
	}

	// make sure we do this, because killing off the orphans
	// will invalidate a whole bunch of pointers.
	MainLoopTasks::root().tasks_.clear();
	MainLoopTasks::root().order_.clear();

	// "orphaned" tasks are ones that were created specifically
	// as a task category and is not a task in itself (so no one
	// else is manging their pointer).
	if (!s_orphans_)
		return;

	for (uint i = 0; i < s_orphans_->size(); i++)
		bw_safe_delete((*s_orphans_)[i]);

	s_orphans_->clear();
	bw_safe_delete(s_orphans_);
}
BW::vector<MainLoopTask*> * MainLoopTasks::s_orphans_ = NULL;

#ifdef DISPLAY_MEMORY
static uint32 lastMemUsed = 0;
#endif

/**
 *	Call tick methods in order
 */
void MainLoopTasks::tick( float dGameTime, float dRenderTime )
{
	for (OrderList::iterator it = order_.begin(); it != order_.end(); it++)
	{
#ifdef DISPLAY_MEMORY
		uint osll = s_levels.length();
		s_levels.append( "/" );
		s_levels.append( (tasks_.begin() + *it)->first );
		(tasks_.begin() + *it)->second->tick( dGameTime, dRenderTime );
		s_levels.erase( osll );

		uint32 nowMemUsed = memUsed();
		if (lastMemUsed != nowMemUsed)
		{
			lastMemUsed = nowMemUsed;
			dprintf( "After tick %s/%s using %d/%dKB\n",
				s_levels.c_str(), (tasks_.begin() + *it)->first, memoryAccountedFor(), memUsed() );
		}
#else
		(tasks_.begin() + *it)->second->tick( dGameTime, dRenderTime );
#endif
	}
}

/**
 *	Call inactive tick methods in order.
 *	The inactive tick is used to update the client when the application
 *	is inactive. Currently this is when the application is minimised.
 */
void MainLoopTasks::inactiveTick( float dGameTime, float dRenderTime )
{
	for (OrderList::iterator it = order_.begin(); it != order_.end(); it++)
	{
		(tasks_.begin() + *it)->second->inactiveTick( dGameTime, dRenderTime );
	}
}

void MainLoopTasks::outputOrder()
{
	for (OrderList::iterator it = order_.begin(); it != order_.end(); it++)
	{
		TRACE_MSG( "MainLoopTasks::outputOrder - %s\n",
			(tasks_.begin() + *it)->first );
	}
}


/**
 *	Call the methods for updating LODs and animations.
 */
void MainLoopTasks::updateAnimations( float dGameTime ) const
{
	BW_GUARD;

	for (OrderList::const_iterator it = order_.begin();
		it != order_.end();
		++it)
	{
		(tasks_.begin() + *it)->second->updateAnimations( dGameTime );
	}
}

/**
 *	Call draw methods in order
 */
void MainLoopTasks::draw()
{
	BW_GUARD;

	if(!enableDraw_)
	{
		return;
	}

	for (OrderList::iterator it = order_.begin(); it != order_.end(); ++it)
	{
#ifdef DISPLAY_MEMORY
		uint osll = s_levels.length();
		s_levels.append( "/" );
		s_levels.append( (tasks_.begin() + *it)->first );
		(tasks_.begin() + *it)->second->draw();
		s_levels.erase( osll );

		uint32 nowMemUsed = memUsed();
		if (lastMemUsed != nowMemUsed)
		{
			lastMemUsed = nowMemUsed;
			dprintf( "After draw %s/%s using %d/%dKB\n",
				s_levels.c_str(), (tasks_.begin() + *it)->first,
				memoryAccountedFor(), memUsed() );
		}
#else
		(tasks_.begin() + *it)->second->draw();
#endif
	}
}


/**
 *	Adds the given task to our list of tasks.
 *
 *	The name can contain slashes '/' to place it within the hierarchy of groups.
 *
 *	The variable length arguments are a list of strings (char *s) specifying the
 *	sorting rules for this task. The rules are of the form '>TaskA', '<TaskB',
 *	meaning that this task should execute after TaskA and before TaskB.
 *	The tasks named in rules are only within the innermost group.
 *
 *	The task pointer can be NULL or equal to an already added task, in which
 *	case the rules for the named task are rewritten. This is necessary to
 *	specify rules for possibly automatically created groups. If the task is
 *	NULL but no such task exists, a group task is created as if automatically.
 *
 *	The list of tasks is re-sorted after this call.
 */
void MainLoopTasks::add( MainLoopTask * pTask, const char * name, ... )
{
	// first find the group (have to do it internally due to var args)
	MainLoopTasks * pGroup = this;
	const char * rname = name;
	const char * slash;
	while ((slash = strchr( rname, '/' )) != NULL)
	{
		BW::string pathBit( rname, slash-rname );
		TaskMap::iterator found = pGroup->tasks_.find( pathBit );

		if (found == pGroup->tasks_.end())
		{
			MainLoopTasks * pNewGroup = new MainLoopTasks();
			if (!s_orphans_)
				s_orphans_ = new BW::vector<MainLoopTask*>;
			s_orphans_->push_back( pNewGroup );

			pGroup->add( pNewGroup, pathBit.c_str(), NULL );
			pGroup = pNewGroup;
		}
		else
		{
			pGroup = (MainLoopTasks*)found->second;
		}

		rname = slash + 1;
	}

	// extract the rules into a vector
	const char * arule;
	RulesList	newRules;
	va_list rules;
	va_start( rules, name );
	do
	{
		arule = va_arg( rules, const char * );
		newRules.push_back( arule );
	} while (arule != NULL );
	va_end( rules );

	// make sure it's not already there
	TaskMap::iterator found = pGroup->tasks_.find( rname );
	if (found != pGroup->tasks_.end())
	{
		if (pTask != NULL && pTask != found->second)
		{
			ERROR_MSG( "MainLoopTasks::add: "
				"Tried to replace existing task %s\n", name );
			return;
		}

		// ok, rewrite rules then
		RulesList::iterator rit = pGroup->rules_.begin();
		for (size_t index = found - pGroup->tasks_.begin(); index > 0; index--)
		{
			while (*rit++ != NULL) ; // scan
		}

		RulesList::iterator rnd = rit;
		while (*rnd++ != NULL) ; // scan

		// replace rules from rit to rnd with args
		size_t rindex = rit - pGroup->rules_.begin();
		pGroup->rules_.erase( rit, rnd );
		pGroup->rules_.insert( pGroup->rules_.begin() + rindex,
			newRules.begin(), newRules.end() );
	}
	else
	{
		if (pTask == NULL)
		{
			pTask = new MainLoopTasks();
			if (!s_orphans_)
				s_orphans_ = new BW::vector<MainLoopTask*>;
			s_orphans_->push_back( pTask );
		}
		pGroup->tasks_.insert( std::make_pair( rname, pTask ) );
		MF_ASSERT( pGroup->tasks_.size() <= INT_MAX );
		pGroup->order_.push_back( ( int )pGroup->tasks_.size() - 1 );
		pGroup->rules_.insert( pGroup->rules_.end(),
			newRules.begin(), newRules.end() );

		// initialise it if we have already been
		if (initted_) pTask->init();
	}

	// and sort it all
	pGroup->sort();
}


/**
 *	Deletes the given task from the list of tasks.
 */
void MainLoopTasks::del( MainLoopTask * pTask, const char * name )
{
	// first find the group
	MainLoopTasks * pGroup = this;
	const char * rname = name;
	const char * slash;
	while ((slash = strchr( rname, '/' )) != NULL)
	{
		BW::string pathBit( rname, slash-rname );
		TaskMap::iterator found = pGroup->tasks_.find( pathBit );

		if (found == pGroup->tasks_.end())
		{		// break if there's no such group
			rname = "/";	// guaranteed not to exist
			break;
		}
		else
		{
			pGroup = (MainLoopTasks*)found->second;
		}

		rname = slash + 1;
	}

	// now find the task
	TaskMap::iterator found = pGroup->tasks_.find( rname );
	if (found != pGroup->tasks_.end() && found->second == pTask)
	{
		// find its rules
		RulesList::iterator rit = pGroup->rules_.begin();
		for (size_t index = found - pGroup->tasks_.begin(); index > 0; index--)
		{
			while (*rit++ != NULL) ; // scan
		}

		RulesList::iterator rnd = rit;
		while (*rnd++ != NULL) ; // scan

		// erase rules from rit to rnd
		pGroup->rules_.erase( rit, rnd );

		// and erase the task
		pGroup->tasks_.erase( found );

		// sort it all out
		pGroup->sort();
	}

	// if it's not there (or an imposter is) then fail silently
}


/**
 *	Helper class to sort our index alphabetically
 */
class AlphaOrderPredicate
{
public:
	AlphaOrderPredicate( const OrderedStringMap<MainLoopTask*> & smap ) :
		smap_( smap ) { }

	bool operator()( int a, int b )
	{
		return strcmp(
			(smap_.begin() + a)->first, (smap_.begin() + b)->first ) < 0;
	}

private:
	const OrderedStringMap<MainLoopTask*> & smap_;
};

/**
 *	Sorts the list of tasks, following the rules.
 */
void MainLoopTasks::sort()
{
	// get rid of the old order
	order_.clear();

	// don't bother sorting if we're finished.
	if (!finished_)
	{
		// this is a vector of the set of tasks that must come before
		// each task, in the order that they are in the tasks map.
		BW::vector< std::basic_string< int > >	prereqs( tasks_.size() );

		// fill in the vector
		int index = 0;
		MF_ASSERT( tasks_.size() < INT_MAX );

		for (RulesList::iterator it = rules_.begin(); it != rules_.end(); it++)
		{
			while (*it != NULL)
			{
				const char * arule = *it;
				if (arule[0] == '<')		// index comes before this task name
				{
					TaskMap::iterator found = tasks_.find( arule+1 );
					if (found != tasks_.end())
						prereqs[ found - tasks_.begin() ].append( 1, index );
				}
				else if (arule[0] == '>')	// index comes after this task name
				{
					TaskMap::iterator found = tasks_.find( arule+1 );
					if (found != tasks_.end())
						prereqs[ index ].append( 1, ( int ) ( found - tasks_.begin() ) );
				}
				else
				{
					ERROR_MSG( "MainLoopTasks::resort: "
						"Unrecognised rule '%s'\n", arule );
				}

				it++;
			}
			index++;
		}

		// sort the tasks alphabetically and always examine them in this order.
		// this means we are independent on linking and initialisation orders.
		BW::vector< int >	alphaOrder;
		for (uint i = 0; i < tasks_.size(); i++)
			alphaOrder.push_back( i );
		// could traverse the stringmap ... but that ain't so easy.
		AlphaOrderPredicate aop( tasks_ );
		std::sort( alphaOrder.begin(), alphaOrder.end(), aop );

		// until we have run out of tasks...
		for (uint t = 0; t < tasks_.size(); t++)
		{
			// find a task that has no prerequisites
			int index;
			uint ai;
			for (ai = 0; ai < tasks_.size(); ai++)
				if (prereqs[ index = alphaOrder[ai] ].empty()) break;
			if (ai == tasks_.size())
			{
				ERROR_MSG( "MainLoopTasks::resort: Cannot resolve "
					"sorting order from conflicting rules! (VERY BAD)\n" );
				return;
			}

			// and make it the next one
			order_.push_back( index );

			// and remove it from everyone's prerequisite lists
			for (uint i = 0; i < tasks_.size(); i++)
			{
				std::basic_string< int > & ipr = prereqs[i];
				for (uint j = 0; j < ipr.size(); j++)
					if (ipr[j] == index)
						ipr.erase( j--, 1 );
			}

			// and make sure it'll never be chosen again
			prereqs[index].append( 1, -1 );
		}
	}
}


/**
 *	Static root accessor
 */
MainLoopTasks & MainLoopTasks::root()
{
	static MainLoopTasks root;
	return root;
}


/**
 *	Gets a main loop task by name.
 *  Returns NULL if not found
 */
MainLoopTask * MainLoopTasks::getMainLoopTask( const char * name )
{
	TaskMap::iterator found = tasks_.find( name );
	if( found != tasks_.end() )
		return found->second;
	else
		return NULL;
}

BW_END_NAMESPACE

// main_loop_task.cpp
