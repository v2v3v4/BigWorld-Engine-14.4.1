#ifndef MODULE_MANAGER_HPP
#define MODULE_MANAGER_HPP

#include <iostream>
#include <stack>

#include "factory.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class Module;
typedef SmartPointer< class Module> ModulePtr;

/**
 * The ModuleManager class manages a stack of application modules,
 * and encapsulates the logical requirements of said stack.
 */

class ModuleManager : public Factory<Module>
{
public:
	ModuleManager();
	~ModuleManager();

	static ModuleManager& instance();
	static void fini();
	void	popAll();

	bool	push( const BW::string & moduleIdentifier );
	void	push( ModulePtr module );
	ModulePtr currentModule();
	void	pop();

private:
	ModuleManager(const ModuleManager&);
	ModuleManager& operator=(const ModuleManager&);

	// Modules
	typedef std::stack< ModulePtr >	ModuleStack;
	ModuleStack			modules_;

	friend std::ostream& operator<<(std::ostream&, const ModuleManager&);
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "module_manager.ipp"
#endif

#endif
/*module_manager.hpp*/
