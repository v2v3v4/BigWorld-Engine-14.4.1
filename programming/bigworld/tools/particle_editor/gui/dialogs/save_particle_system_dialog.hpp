#error File no longer in use.  Allan 04/03/03


// SaveParticleSystemDialog dialog

class SaveParticleSystemDialog : public CDialog
{
	DECLARE_DYNAMIC(SaveParticleSystemDialog)

public:
	SaveParticleSystemDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~SaveParticleSystemDialog();

// Dialog Data
	enum { IDD = IDD_SAVE_PARTICLE_SYSTEM_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
