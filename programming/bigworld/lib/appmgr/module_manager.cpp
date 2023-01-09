#include "pch.hpp"

#include "module_manager.hpp"
#include "module.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "Module", 0 );

#ifndef CODE_INLINE
#include "module_manager.ipp"
#endif

BW_BEGIN_NAMESPACE

namespace
{
	ModuleManager *s_instance_ = NULL;
}


/**
 *	Constuctor.
 */
ModuleManager::ModuleManager()
{
}


/**
 *	Destructor.
 */
ModuleManager::~ModuleManager()
{
	this->popAll();
}


/**
 *	This method ensures the singular existence of a ModuleManager, and returns
 *	that instance.
 *
 *	@return The ModuleManager.
 */
/*static*/ ModuleManager& ModuleManager::instance()
{
	SINGLETON_MANAGER_WRAPPER( ModuleManager )
	if (s_instance_ == NULL)
	{
		s_instance_ = new ModuleManager();
		REGISTER_SINGLETON( ModuleManager )
	}
	return *s_instance_;
}


/**
 *	This method deletes the module manager instance.
 */
/*static*/ void ModuleManager::fini()
{
	bw_safe_delete(s_instance_);
}


/**
 * This method removes all modules left on the module stack.
 * This allows for a quick graceful exit from the application.
 */
void ModuleManager::popAll()
{
	while ( currentModule() )
	{
		//NOTE - don't call our own pop method,
		//simply because we don't want to resume
		//any of the modules further down the stack
		//when they become the next "current module"
		currentModule()->onStop();
		modules_.pop();
	}
}


/**
 *	This method pushes a module onto the module stack.
 *	This method works with ModuleManager::create( identifier )
 *
 *	@param identifier the name of the module to push onto the stack.
 *	@return True if succeeded, otherwise false.
 *
 *	@see ModuleManager::push
 */
bool
ModuleManager::push( const BW::string& identifier )
{
	Module* module =this->create( identifier.c_str() );

	if ( module )
	{
		this->push( module );
	}
	else
	{
		//strip whitespace and try again
		BW::string className = "";
		for ( uint i = 0; i < identifier.size(); i++ )
		{
			if ( identifier[i] != 32 )
				className += identifier[i];
		}

		module = this->create( className.c_str() );

		if ( module )
		{
			this->push( module );
		}
	}

	//coz the creator adds a reference,
	//and our stack takes a reference, here
	//we must remove the creation reference.
	if (module)
	{
		module->decRef();
	}

	return (module != NULL);
}


/**
 * This method pushes a module onto the module stack.
 * The provided module becomes the new active module automatically.
 * The existing active module is paused, and the provided
 * module is told to Start.
 *
 * @param module the module that will be the new active module.
 */
void ModuleManager::push( ModulePtr module )
{
	if (currentModule())
		currentModule()->onPause();

	MF_ASSERT( module );

	modules_.push( module );

	module->onStart();
}


/**
 * This method returns the currently active module, or NULL if
 * the module stack is emtpy.
 *
 * @return the currently active module.
 */
ModulePtr ModuleManager::currentModule()
{
	if ( modules_.empty() )
		return NULL;

	return modules_.top();
}


/**
 * This method removes the current module from the module stack.
 * The current module is told to Stop.
 * The new active module ( if there is one ) is told to resume,
 * where previously it was paused during an earlier push() operation.
 */
void ModuleManager::pop()
{
	int exitCode;

	ModulePtr current = currentModule();

	if ( current )
	{
		exitCode = current->onStop();
		modules_.pop();
	}

	if ( currentModule() )
		currentModule()->onResume( exitCode );
}


/**
 *	Output streaming operator for ModuleManager.
 */
std::ostream& operator<<(std::ostream& o, const ModuleManager& t)
{
	o << "ModuleManager\n";
	return o;
}

BW_END_NAMESPACE

/*module_manager.cpp*/
