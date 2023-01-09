#include "pch.hpp"
#include "py_gfx_display_object.hpp"
#include "cstdmf/string_utils.hpp"


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT(ScaleformBW::PyGFxDisplayObject)

PY_TYPEOBJECT(ScaleformBW::PyGFxDisplayInfo)

namespace ScaleformBW {

inline void convertFromMatrix2x4(const SF::Render::Matrix2x4<float> &m, Matrix &ret)
{
	ret._11 = m.M[0][0]; ret._12 = m.M[0][1];
	ret._21 = m.M[1][0]; ret._22 = m.M[1][1];
	ret._41 = m.M[0][3]; ret._42 = m.M[1][3];
}

inline SF::Render::Matrix2x4<float> convertToMatrix2x4(const Matrix &m)
{
	return SF::Render::Matrix2x4<float>(
		m._11, m._12, 0.0f, m._41,
		m._21, m._22, 0.0f, m._42);
}

inline void convertFromMatrix3x4(const SF::Render::Matrix3x4<float> &m, Matrix &ret)
{
	ret._11 = m.M[0][0]; ret._12 = m.M[0][1]; ret._12 = m.M[0][2];
	ret._21 = m.M[1][0]; ret._22 = m.M[1][1]; ret._23 = m.M[1][2];
	ret._31 = m.M[2][0]; ret._32 = m.M[2][1]; ret._33 = m.M[2][2];
	ret._41 = m.M[0][3]; ret._42 = m.M[1][3]; ret._42 = m.M[2][3];
}

inline SF::Render::Matrix3x4<float> convertToMatrix3x4(const Matrix &m)
{
	return SF::Render::Matrix3x4<float>(
		m._11, m._12, m._13, m._41,
		m._21, m._22, m._23, m._42,
		m._31, m._32, m._33, m._43);
}


//-------------------------------------------------------------------

PY_BEGIN_METHODS(PyGFxDisplayObject)
	PY_METHOD(getDisplayInfo)
	PY_METHOD(setDisplayInfo)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES(PyGFxDisplayObject)
	PY_ATTRIBUTE(displayMatrix)
	PY_ATTRIBUTE(matrix3D)
	PY_ATTRIBUTE(worldMatrix)
	PY_ATTRIBUTE(text)
	PY_ATTRIBUTE(textHTML)
	PY_ATTRIBUTE(textUnicode)
	PY_ATTRIBUTE(textHTMLUnicode)
PY_END_ATTRIBUTES()


PyGFxDisplayObject::PyGFxDisplayObject(PyTypePlus *pType) : PyGFxValue(pType)
{
}

Matrix PyGFxDisplayObject::displayMatrix()
{
	BW_GUARD;

	Matrix ret;
	ret.setIdentity();

	SF::Render::Matrix2x4<float> m;
	if (gfxValue_.GetDisplayMatrix(&m))
		convertFromMatrix2x4(m, ret);

	return ret;
}

void PyGFxDisplayObject::displayMatrix(const Matrix &m)
{
	BW_GUARD;

	gfxValue_.SetDisplayMatrix(convertToMatrix2x4(m));
}

Matrix PyGFxDisplayObject::matrix3D()
{
	BW_GUARD;

	Matrix ret;
	ret.setIdentity();

	SF::Render::Matrix3x4<float> m;
	if (gfxValue_.GetMatrix3D(&m))
		convertFromMatrix3x4(m, ret);

	return ret;
}

void PyGFxDisplayObject::matrix3D(const Matrix &m)
{
	BW_GUARD;

	gfxValue_.SetMatrix3D(convertToMatrix3x4(m));
}

Matrix PyGFxDisplayObject::worldMatrix()
{
	BW_GUARD;

	Matrix ret;
	ret.setIdentity();

	SF::Render::Matrix2x4<float> m;
	if (gfxValue_.GetWorldMatrix(&m))
		convertFromMatrix2x4(m, ret);

	return ret;
}

BW::string PyGFxDisplayObject::text()
{
	BW_GUARD;

	GFx::Value tmp;
	if (gfxValue_.GetText(&tmp))
	{
		if (tmp.GetType() == GFx::Value::VT_StringW)
			return bw_wtoutf8(tmp.GetStringW());
		else if (tmp.GetType() == GFx::Value::VT_String)
			return tmp.GetString();
	}

	return "";
}

void PyGFxDisplayObject::text(const BW::string &str)
{
	BW_GUARD;

	gfxValue_.SetText(str.c_str());
}

BW::string PyGFxDisplayObject::textHTML()
{
	BW_GUARD;

	GFx::Value tmp;
	if (gfxValue_.GetTextHTML(&tmp))
	{
		if (tmp.GetType() == GFx::Value::VT_StringW)
			return bw_wtoutf8(tmp.GetStringW());
		else if (tmp.GetType() == GFx::Value::VT_String)
			return tmp.GetString();
	}

	return "";
}

void PyGFxDisplayObject::textHTML(const BW::string &str)
{
	BW_GUARD;

	gfxValue_.SetTextHTML(str.c_str());
}

BW::wstring PyGFxDisplayObject::textUnicode()
{
	BW_GUARD;

	GFx::Value tmp;
	if (gfxValue_.GetText(&tmp))
	{
		if (tmp.GetType() == GFx::Value::VT_StringW)
			return tmp.GetStringW();
		else if (tmp.GetType() == GFx::Value::VT_String)
			return bw_utf8tow(tmp.GetString());
	}

	return L"";
}

void PyGFxDisplayObject::textUnicode(const BW::wstring &str)
{
	BW_GUARD;

	gfxValue_.SetText(str.c_str());
}

BW::wstring PyGFxDisplayObject::textHTMLUnicode()
{
	BW_GUARD;

	GFx::Value tmp;
	if (gfxValue_.GetTextHTML(&tmp))
	{
		if (tmp.GetType() == GFx::Value::VT_StringW)
			return tmp.GetStringW();
		else if (tmp.GetType() == GFx::Value::VT_String)
			return bw_utf8tow(tmp.GetString());
	}

	return L"";
}

void PyGFxDisplayObject::textHTMLUnicode(const BW::wstring &str)
{
	BW_GUARD;

	gfxValue_.SetTextHTML(str.c_str());
}

PyObject *PyGFxDisplayObject::getDisplayInfo()
{
	BW_GUARD;

	return new PyGFxDisplayInfo(gfxValue_);
}

void PyGFxDisplayObject::setDisplayInfo(PyObjectPtr info)
{
	BW_GUARD;

	if (info != NULL && info->ob_type == &PyGFxDisplayInfo::s_type_)
	{
		PyGFxDisplayInfo *pInfo = (PyGFxDisplayInfo*)info.get();
		pInfo->apply(gfxValue_);
	}
}


//---------------------------------------------------------------------------

PY_BEGIN_METHODS(PyGFxDisplayInfo)
	PY_METHOD(clear)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES(PyGFxDisplayInfo)
	PY_ATTRIBUTE(position)
	PY_ATTRIBUTE(position3D)
	PY_ATTRIBUTE(scale)
	PY_ATTRIBUTE(scale3D)
	PY_ATTRIBUTE(viewMatrix3D)
	PY_ATTRIBUTE(projectionMatrix3D)
	PY_ATTRIBUTE(x)
	PY_ATTRIBUTE(y)
	PY_ATTRIBUTE(z)
	PY_ATTRIBUTE(xScale)
	PY_ATTRIBUTE(yScale)
	PY_ATTRIBUTE(zScale)
	PY_ATTRIBUTE(xRotation)
	PY_ATTRIBUTE(yRotation)
	PY_ATTRIBUTE(FOV)
	PY_ATTRIBUTE(alpha)
	PY_ATTRIBUTE(visible)
	PY_ATTRIBUTE(edgeAAMode)
PY_END_ATTRIBUTES()

void * PyGFxDisplayInfo::operator new(std::size_t size)
{
	return bw_malloc_aligned(size, 16);
}

void PyGFxDisplayInfo::operator delete(void * ptr)
{
	bw_free_aligned(ptr);
}

PyGFxDisplayInfo::PyGFxDisplayInfo(GFx::Value &value, PyTypePlus *pType)
	: PyObjectPlus(pType)
{
	BW_GUARD;

	value.GetDisplayInfo(&info_);
}

void PyGFxDisplayInfo::apply(GFx::Value &value)
{
	BW_GUARD;

	value.SetDisplayInfo(info_);
}

Matrix PyGFxDisplayInfo::viewMatrix3D()
{
	BW_GUARD;

	Matrix ret;
	ret.setIdentity();

	const SF::Render::Matrix3x4<float> *m = info_.GetViewMatrix3D();
	if (m != NULL)
		convertFromMatrix3x4(*m, ret);

	return ret;
}

void PyGFxDisplayInfo::viewMatrix3D(const Matrix &m)
{
	BW_GUARD;

	const SF::Render::Matrix3x4<float> m34 = convertToMatrix3x4(m);
	info_.SetViewMatrix3D(&m34);
}

Matrix PyGFxDisplayInfo::projectionMatrix3D()
{
	BW_GUARD;

	Matrix ret;
	ret.setIdentity();

	const SF::Render::Matrix4x4<float> *m = info_.GetProjectionMatrix3D();
	if (m != NULL)
	{
		// TODO: redo this if will be used extensively, too ugly for now!
		ret._11 = m->M[0][0]; ret._12 = m->M[0][1]; ret._13 = m->M[0][2]; ret._14 = m->M[0][3];
		ret._21 = m->M[1][0]; ret._22 = m->M[1][1]; ret._23 = m->M[1][2]; ret._24 = m->M[1][3];
		ret._31 = m->M[2][0]; ret._32 = m->M[2][1]; ret._33 = m->M[2][2]; ret._34 = m->M[2][3];
		ret._41 = m->M[3][0]; ret._42 = m->M[3][1]; ret._43 = m->M[3][2]; ret._44 = m->M[3][3];
	}

	return ret;
}

void PyGFxDisplayInfo::projectionMatrix3D(const Matrix &m)
{
	BW_GUARD;

	SF::Render::Matrix4x4<float> matr;
	matr.Set(&m._11, 16);

	info_.SetProjectionMatrix3D(&matr);
}

}

BW_END_NAMESPACE
