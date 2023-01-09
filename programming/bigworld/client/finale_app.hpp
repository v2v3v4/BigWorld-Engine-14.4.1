#ifndef FINALE_APP_HPP
#define FINALE_APP_HPP


#include "cstdmf/main_loop_task.hpp"
#include "input/input.hpp"


BW_BEGIN_NAMESPACE

class ProgressTask;


/**
 *	Finale task
 */
class FinaleApp : public MainLoopTask, InputHandler
{
public:
	FinaleApp();
	~FinaleApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

	bool handleKeyEvent(const KeyEvent & event);

public:
	static FinaleApp instance;

private:
	void runPreloads( ProgressTask & preloadTask );
	void findEffects( const BW::StringRef & folderName,
						BW::vector< BW::string > & ret ) const;
	bool cacheEffects( const BW::vector< BW::string > & effects ) const;

	bool				cancelPreloads_;
	bool				disablePreloads_;
	bool				donePreloads_;

	BW::vector< Moo::BaseTexturePtr > preloadedTextures_;
	BW::vector< DataSectionPtr > preloadedDataSections_;
	BW::vector< PyObject* > preloadedPyObjects_;
	BW::vector< Moo::ManagedEffectPtr > preloadedEffects_;
};

BW_END_NAMESPACE

#endif // FINALE_APP_HPP

// finale_app.hpp
