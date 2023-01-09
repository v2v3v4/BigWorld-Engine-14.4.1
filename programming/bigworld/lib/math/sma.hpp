#ifndef _SMA_HPP
#define _SMA_HPP

#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class calculates the simple moving average of a variable, over a given
 *	period.
 *
 *	@ingroup Math
 */
template<class T> class SMA
{
public:
	SMA(int period);
	SMA( const SMA<T> & );
	~SMA();

	SMA&	operator=( const SMA<T> & );

	void 	append(T value);
	T 		average() const;
	T 		min() const;
	T 		max() const;

	/// The desired number of samples
	int		period() const			{ return period_; }

	/// The actual number of samples (may be less than period)
	int		count() const			{ return count_; }

	void 	clear();

private:
	int 	period_;
	T* 		samples_;
	T		total_;
	int 	count_;
	int		pos_;
};


/**
 *	This constructor creates an empty SMA.
 */
template<class T> SMA<T>::SMA( int period ) :
	period_(period),
	total_(0),
	count_(0),
	pos_(0)
{
	MF_ASSERT(period > 2 && period < 1000);

	samples_ = new T[period_];

	for (int i = 0; i < period_; i++)
		samples_[i] = 0;
}


/**
 *	Copy constructor.
 */
template<class T> SMA<T>::SMA(const SMA<T>& rOther) :
	period_(rOther.period_),
	count_(rOther.count_),
	total_(rOther.total_),
	pos_(rOther.pos_)
{
	samples_ = new T[period_];

	for(int i = 0; i < period_; i++)
		samples_[i] = rOther.samples_[i];
}

/**
 *	Destructor.
 */
template<class T> SMA<T>::~SMA()
{
	bw_safe_delete_array(samples_);
}

/**
 *	Assignment operator.
 */
template<class T> SMA<T>& SMA<T>::operator=(const SMA<T>& rOther)
{
	if (&rOther != this)
	{
		period_ = rOther.period_;
		count_ = rOther.count_;
		total_ = rOther.total_;
		pos_ = rOther.pos_;

		bw_safe_delete_array(samples_);
		samples_ = new T[period_];
	}

	return *this;
}


/**
 *	This method appends a value to the dataseries, and recalculates the total.
 */
template<class T> void SMA<T>::append(T value)
{
	total_ -= samples_[pos_];

	samples_[pos_] = value;
	total_ += value;
	pos_ = (pos_ + 1) % period_;

	if (count_ < period_)
		count_++;
}


/**
 *	This method calculates the current average.
 */
template<class T> T SMA<T>::average() const
{
	if (count_)
		return total_ / count_;
	else return 0;
}


/**
 *	This method calculates the min value in the series.
 */
template<class T> T SMA<T>::min() const
{
	T minVal = samples_[0];

	for (int i = 1; i < count_; i++)
		if (samples_[i] < minVal)
			minVal = samples_[i];

	return minVal;
}


/**
 *	This method calculates the max value in the series.
 */
template<class T> T SMA<T>::max() const
{
	T maxVal = samples_[0];

	for (int i = 1; i < count_; i++)
		if (samples_[i] > maxVal)
			maxVal = samples_[i];

	return maxVal;
}




/**
 * This method clears the dataseries.
 */
template<class T> void SMA<T>::clear()
{
	count_ = 0;
	pos_ = 0;
	total_ = 0;

	for (int i = 0; i < period_; i++)
		samples_[i] = 0;
}

BW_END_NAMESPACE

#endif // _SMA_HPP

