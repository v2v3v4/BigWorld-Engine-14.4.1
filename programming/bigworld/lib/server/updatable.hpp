#ifndef UPDATABLE_HPP
#define UPDATABLE_HPP

BW_BEGIN_NAMESPACE

/**
 * 	This interface should be implemented by any object that wants to receive
 * 	updates. The CellApp can register objects that implement this interface
 * 	to receive updates every game tick.
 */ 	
class Updatable
{
public:
	Updatable() : removalHandle_( -1 ) {}
	virtual ~Updatable() {};

	/**
	 *	This virtual method is called regularly on every registered Updatable
	 *	object.
	 */
	virtual void update() = 0;

private:
	friend class Updatables;
	int removalHandle_;
};

BW_END_NAMESPACE

#endif // UPDATABLE_HPP
