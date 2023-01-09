#ifndef SAFE_FIFO_HPP
#define SAFE_FIFO_HPP

BW_BEGIN_NAMESPACE

template <class T>
class SafeFifo
{
public:
	void push(T& t)
	{
		SimpleMutexHolder smh(mutex_);
		data_.push_back(t);
	}

	T pop()
	{
		SimpleMutexHolder smh(mutex_);
		T returned = *data_.begin();
		data_.pop_front();
		return returned;
	}

	uint count()
	{
		SimpleMutexHolder smh(mutex_);
		return data_.size();
	}

private:
	mutable SimpleMutex mutex_;
	BW::list<T> data_;
};

BW_END_NAMESPACE

#endif // SAFE_FIFO_HPP
