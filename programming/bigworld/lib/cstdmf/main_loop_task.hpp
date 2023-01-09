#ifndef MAIN_LOOP_TASK_HPP
#define MAIN_LOOP_TASK_HPP

#include "cstdmf/bw_vector.hpp"
#include "stringmap.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is an interface that can be implemented by tasks that want
 *	to run in the main loop. It is designed for large tasks, and so it is
 *	required that each task have a unique name. Tasks can be ordered by
 *	using groups (the task name is like a path) and by naming other tasks
 *	that must occur before or after the task considered.
 */
class MainLoopTask
{
public:
	MainLoopTask() : enableDraw_( true ) {	}
	virtual ~MainLoopTask() { }

	virtual bool init() { return true; }
	virtual void fini()	{ }

	virtual bool tick( float eraseMe ) { return false; }
	virtual void tick( float dGameTime, float dRenderTime ) { }
	virtual void updateAnimations( float dGameTime ) const {}
	virtual void draw() { }
	virtual bool inactiveTick( float eraseMe ) { return false; }
	virtual void inactiveTick( float dGameTime, float dRenderTime ) {}

	bool	enableDraw_;
};


/**
 *	This class is a group of tasks with a similar function.
 *	It is not a singleton, but there is one global root group.
 */
class MainLoopTasks : public MainLoopTask
{
public:
	MainLoopTasks();
	virtual ~MainLoopTasks();

	virtual bool init();
	virtual void fini();

	virtual bool tick( float eraseMe ) { return false; };
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void updateAnimations( float dGameTime ) const;
	virtual void draw();

	// this gets called rather than tick when the application is minimised
	virtual bool inactiveTick( float eraseMe ) { return false; };
	virtual void inactiveTick( float dGameTime, float dRenderTime );

	// rules are strings like ">TaskA", "<TaskB"
	CSTDMF_DLL void add( MainLoopTask * pTask, const char * name, ... ); // rules, NULL@end
	void del( MainLoopTask * pTask, const char * name );

	void outputOrder();

	MainLoopTask * getMainLoopTask( const char * name );

	CSTDMF_DLL static MainLoopTasks & root();

	bool initted()							{ return initted_; }

	static void finiAll();
private:
	MainLoopTasks( const MainLoopTasks& );
	MainLoopTasks& operator=( const MainLoopTasks& );

	void sort();

	typedef OrderedStringMap<MainLoopTask*>	TaskMap;
	typedef BW::vector<int>			OrderList;
	typedef BW::vector<const char*>	RulesList;

	TaskMap			tasks_;
	OrderList		order_;
	RulesList		rules_;

	bool			initted_;
	bool			finished_;

	static BW::vector<MainLoopTask*> *s_orphans_;
};

BW_END_NAMESPACE

#endif // MAIN_LOOP_TASK_HPP
