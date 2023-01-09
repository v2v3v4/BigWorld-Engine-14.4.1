#ifndef IMPORT_IMAGE_HPP
#define IMPORT_IMAGE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/import/import_codec.hpp"
#include "moo/image.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is a two-dimensional image of elevation or mask data.
 */
class ImportImage : public Moo::Image<uint16>
{
public:
	typedef Moo::Image<uint16>		Base;

    ImportImage();
    ImportImage(unsigned int w, unsigned int h);
    ImportImage(ImportImage const &other);
    ~ImportImage();

    ImportImage &operator=(ImportImage const &other);

    ImportCodec::LoadResult 
    load
    (
        BW::string     const & filename,
        float           *left,
        float           *top,
        float           *right,
        float           *bottom,
		bool			*absolute,
        bool            configDialog
    );

    bool 
    save
    (
        BW::string     const & filename,
        float           *left,
        float           *top,
        float           *right,
        float           *bottom,
		bool			*absolute,
		float			*minHeight,
		float			*maxHeight
    );

    void flipHeight();

	float toHeight(uint16 v) const;
	float toHeight(float v) const;
	double toHeight(double v) const;

    float scaleAdd() const;
	float scaleMul() const;
	void setScale(float minV, float maxV, uint16 minSrc = 0, uint16 maxSrc = 65535);
	void getScale(float &minV, float &maxV) const;
	void normalise();

	void rangeRaw(uint16 &minV, uint16 &maxV, bool recalc = false) const;
	void rangeHeight(float &minV, float &maxV, bool recalc = false) const;

protected:
    void copy(ImportImage const &other);

    void destroy();

private:
	float			scaleAdd_;
	float			scaleMul_;
	mutable uint16	minRaw_;
	mutable uint16	maxRaw_;
};


/**
 *	This converts from a raw value to a height/mask value.
 *
 *	@param v		The raw value to convert.
 *	@returns		The height/mask value given the raw value.
 */
inline float ImportImage::toHeight(uint16 v) const
{
	return v*scaleMul_ + scaleAdd_;
}


/**
 *	This converts from a raw value to a height/mask value.
 *
 *	@param v		The raw value to convert.
 *	@returns		The height/mask value given the raw value.
 */
inline FLOAT ImportImage::toHeight(FLOAT v) const
{
	return v*scaleMul_ + scaleAdd_;
}


/**
 *	This converts from a raw value to a height/mask value.
 *
 *	@param v		The raw value to convert.
 *	@returns		The height/mask value given the raw value.
 */
inline double ImportImage::toHeight(double v) const
{
	return v*scaleMul_ + scaleAdd_;
}


typedef SmartPointer<ImportImage>	ImportImagePtr;

BW_END_NAMESPACE

#endif // IMPORT_IMAGE_HPP
