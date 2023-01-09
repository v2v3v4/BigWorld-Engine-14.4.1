#ifndef EDITOR_CHUNK_LOCK_VISUALIZER_HPP
#define EDITOR_CHUNK_LOCK_VISUALIZER_HPP

#include "moo/device_callback.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkLockVisualizer: public Moo::DeviceCallback
{
	struct GPUBox
	{
		Vector4 m_dir;
		Vector4 m_pos;
		Vector4 m_up;
		Vector4 m_scale;
	};
	
	BW::vector<GPUBox>	m_Vertices;
	Moo::EffectMaterialPtr				m_materials[2]; // stencil and main
	Moo::VisualPtr						m_cube;
	Moo::VertexBuffer					m_instancedVB;
private:
	GPUBox fromBoundBox2GPUBox(const BoundingBox & bbox, const Matrix & transform, DWORD color);
	void fillVB();
public:
	virtual void	createManagedObjects();
	virtual void	deleteManagedObjects();
	virtual void	createUnmanagedObjects();
	virtual void	deleteUnmanagedObjects();

	void addBox(const BoundingBox& bbox, const Matrix & transform, DWORD color);

	void draw();
};

BW_END_NAMESPACE

#endif
