#ifndef PROGRESS_HPP__
#define PROGRESS_HPP__

BW_BEGIN_NAMESPACE

class Progress
{
public:
	virtual ~Progress() {}

	virtual void name( const BW::string& task ) {}
	virtual bool isCancelled() { return false; }
	virtual bool step( float progress = 1.f ) = 0;
	virtual bool set( float done = 0.f ) = 0;

	virtual void length( float length ) = 0;
};

BW_END_NAMESPACE

#endif//PROGRESS_HPP__
