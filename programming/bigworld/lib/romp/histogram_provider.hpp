#ifndef HISTOGRAM_PROVIDER_HPP
#define HISTOGRAM_PROVIDER_HPP


BW_BEGIN_NAMESPACE

/**
 * TODO: to be documented.
 */
class HistogramProvider
{
public:
	enum DUMMY
	{
		HISTOGRAM_LEVEL = 256
	};
	/**
	 * TODO: to be documented.
	 */
	struct Histogram
	{
		unsigned int value_[ 256 ];
	};
	enum Type
	{
		HT_R,
		HT_G,
		HT_B,
		HT_LUMINANCE,
		HT_NUM
	};
	HistogramProvider();
	void addUser();
	void removeUser();
	bool inUse() const;
	Histogram get( Type type ) const;
	void update();

	static HistogramProvider& instance();

private:
	unsigned int usageCount_;
	Histogram histogram_[ HT_NUM ];
};

BW_END_NAMESPACE

#endif//HISTOGRAM_PROVIDER_HPP
