#ifndef FREQUENT_TASKS_HPP
#define FREQUENT_TASKS_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Interface for a task to be added to the FrequentTasks collection.
 */
class FrequentTask
{
public:
	FrequentTask( const char * name = "UnnamedTask" ) : name_( name )
		{}
	virtual ~FrequentTask() {}

	virtual void doTask() = 0;

	const char * taskName()
		{ return name_; }
private:
	const char * name_;
};



/**
 *	This class is used to maintain the collection of frequent tasks.
 */
class FrequentTasks
{
public:
	FrequentTasks();
	~FrequentTasks();

	void add( FrequentTask * pTask );

	bool cancel( FrequentTask * pTask );

	void process();

private:
	
	typedef BW::vector< Mercury::FrequentTask * > Container;
	Container container_;

	bool isDirty_;

	bool * pGotDestroyed_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // FREQUENT_TASKS_HPP

