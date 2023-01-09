#ifndef PY_GFXDISPLAYOBJECT_HPP
#define PY_GFXDISPLAYOBJECT_HPP

#include "math/matrix.hpp"
#include "py_gfx_value.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	class PyGFxDisplayInfo;

	/*
	* Derived from PyGFxValue, exposes Scaleform DisplayObject API methods to Python scripts.
	*/
	class PyGFxDisplayObject : public PyGFxValue
	{
		Py_Header(PyGFxDisplayObject, PyGFxValue);

	protected:
		PyGFxDisplayObject(PyTypePlus *pType = &s_type_);

	public:
		Matrix displayMatrix();
		void displayMatrix(const Matrix &m);
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(Matrix, displayMatrix, displayMatrix);

		PyObject *getDisplayInfo();
		void setDisplayInfo(PyObjectPtr info);
		PY_AUTO_METHOD_DECLARE(RETOWN, getDisplayInfo, END);
		PY_AUTO_METHOD_DECLARE(RETVOID, setDisplayInfo, ARG(PyObjectPtr, END));

		Matrix matrix3D();
		void matrix3D(const Matrix &m);
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(Matrix, matrix3D, matrix3D);

		Matrix worldMatrix();
		PY_RO_ATTRIBUTE_DECLARE(worldMatrix(), worldMatrix);

		BW::string text();
		void text(const BW::string &str);
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(BW::string, text, text);

		BW::string textHTML();
		void textHTML(const BW::string &str);
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(BW::string, textHTML, textHTML);

		BW::wstring textUnicode();
		void textUnicode(const BW::wstring &str);
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(BW::wstring, textUnicode, textUnicode);

		BW::wstring textHTMLUnicode();
		void textHTMLUnicode(const BW::wstring &str);
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(BW::wstring, textHTMLUnicode, textHTMLUnicode);

		friend class PyGFxValue;
	};


	//-------------------------------------------------------------------------

	/*
	* Wrapper class for GFx::Value::DisplayInfo
	*/
	class PyGFxDisplayInfo : public PyObjectPlus
	{
		Py_Header(PyGFxDisplayInfo, PyObjectPlus);
	
	public:
		PyGFxDisplayInfo(GFx::Value &value, PyTypePlus *pType = &s_type_);
		~PyGFxDisplayInfo() { }

		void apply(GFx::Value &value);

		void clear() { info_.Clear(); }
		PY_AUTO_METHOD_DECLARE(RETVOID, clear, END);

		Vector2 position() { return Vector2((float)info_.GetX(), (float)info_.GetY()); }
		void position(const Vector2 &pos) { info_.SetPosition(pos.x, pos.y); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(Vector2, position, position);

		Vector3 position3D() { return Vector3((float)info_.GetX(), (float)info_.GetY(), (float)info_.GetZ()); }
		void position3D(const Vector3 &pos) { info_.SetX(pos.x); info_.SetY(pos.y); info_.SetZ(pos.z); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(Vector3, position3D, position3D);

		Vector2 scale() { return Vector2((float)info_.GetXScale(), (float)info_.GetYScale()); }
		void scale(const Vector2 &sc) { info_.SetScale(sc.x, sc.y); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(Vector2, scale, scale);

		Vector3 scale3D() { return Vector3((float)info_.GetXScale(), (float)info_.GetYScale(), (float)info_.GetZScale()); }
		void scale3D(const Vector3 &sc) { info_.SetXScale(sc.x); info_.SetYScale(sc.y); info_.SetZScale(sc.z); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(Vector3, scale3D, scale3D);

		Matrix viewMatrix3D();
		void viewMatrix3D(const Matrix &m);
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(Matrix, viewMatrix3D, viewMatrix3D);

		Matrix projectionMatrix3D();
		void projectionMatrix3D(const Matrix &m);
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(Matrix, projectionMatrix3D, projectionMatrix3D);

		float rotationDegrees() { return (float)info_.GetRotation(); }
		void rotationDegrees(float val) { info_.SetRotation(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, rotationDegrees, rotationDegrees);

		float x() { return (float)info_.GetX(); }
		void x(float val) { info_.SetX(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, x, x);

		float y() { return (float)info_.GetY(); }
		void y(float val) { info_.SetY(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, y, y);

		float z() { return (float)info_.GetZ(); }
		void z(float val) { info_.SetZ(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, z, z);

		float xScale() { return (float)info_.GetXScale(); }
		void xScale(float val) { info_.SetXScale(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, xScale, xScale);

		float yScale() { return (float)info_.GetYScale(); }
		void yScale(float val) { info_.SetYScale(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, yScale, yScale);

		float zScale() { return (float)info_.GetZScale(); }
		void zScale(float val) { info_.SetZScale(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, zScale, zScale);

		float xRotation() { return (float)info_.GetXRotation(); }
		void xRotation(float val) { info_.SetXRotation(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, xRotation, xRotation);

		float yRotation() { return (float)info_.GetYRotation(); }
		void yRotation(float val) { info_.SetYRotation(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, yRotation, yRotation);

		float FOV() { return (float)info_.GetFOV(); }
		void FOV(float val) { info_.SetFOV(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, FOV, FOV);

		float alpha() { return (float)info_.GetAlpha(); }
		void alpha(float val) { info_.SetAlpha(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(float, alpha, alpha);

		bool visible() { return info_.GetVisible(); }
		void visible(bool val) { info_.SetVisible(val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(bool, visible, visible);

		uint edgeAAMode() { return (uint)info_.GetEdgeAAMode(); }
		void edgeAAMode(uint val) { info_.SetEdgeAAMode((SF::Render::EdgeAAMode)val); }
		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(uint, edgeAAMode, edgeAAMode);

		// Gfx::Value::DisplayInfo has 16 bit alignment requirements, thus need to do an aligned alloc
		static void * operator new(std::size_t size);
		static void operator delete(void * ptr);

	private:
		GFx::Value::DisplayInfo info_;
	};
}

BW_END_NAMESPACE

#endif
