#ifndef PARTICLE_EDITOR_DOC_HPP
#define PARTICLE_EDITOR_DOC_HPP

BW_BEGIN_NAMESPACE

class ParticleEditorDoc : public CDocument
{
protected:
    ParticleEditorDoc();

    DECLARE_DYNCREATE(ParticleEditorDoc)

public:
    /*virtual*/ ~ParticleEditorDoc();

    /*virtual*/ BOOL OnNewDocument();

    void SetActionProperty(int actionSelect);

    static ParticleEditorDoc &instance();

protected:
    DECLARE_MESSAGE_MAP()

private:
    static ParticleEditorDoc    *s_instance_;
    int                         m_actionSelect;
    bool                        m_check;
};

BW_END_NAMESPACE

#endif // PARTICLE_EDITOR_DOC_HPP
