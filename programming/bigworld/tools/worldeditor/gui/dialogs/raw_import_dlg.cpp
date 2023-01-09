#include "pch.hpp"
#include "worldeditor/gui/dialogs/raw_import_dlg.hpp"
#include "common/format.hpp"
#include "resmgr/multi_file_system.hpp"

BW_BEGIN_NAMESPACE


BEGIN_MESSAGE_MAP(RawImportDlg, CDialog)
    ON_CBN_SELCHANGE(IDC_RAWIMP_SIZE        , OnUpdateImage)
    ON_BN_CLICKED   (IDC_RAWIMP_LITTLEENDIAN, OnUpdateImage)
    ON_BN_CLICKED   (IDC_RAWIMP_BIGENDIAN   , OnUpdateImage)
END_MESSAGE_MAP()


/*explicit*/ RawImportDlg::RawImportDlg(char const *filename):
	CDialog(IDD),
	filename_(filename),
	data_(NULL),
	dataSize_(0),
	selWidth_(0),
	selHeight_(0),
	littleEndian_(true)
{
	BW_GUARD;

    CWaitCursor waitCursor; // this may take a little while

    // Read the RAW file into a buffer:
    FILE *file = NULL;
    try
    {
        file = BWResource::instance().fileSystem()->posixFileOpen(filename_, "rb");
        if (file != NULL)
        {
            MF_VERIFY( fseek(file, 0, SEEK_END) == 0 );
            dataSize_ = ftell(file);
            MF_VERIFY( fseek(file, 0, SEEK_SET) == 0 );
            data_ = new uint8[dataSize_];
            fread(data_, sizeof(uint8), dataSize_, file);
            fclose(file);
        }
    }
    catch (...)
    {
        fclose(file); file = NULL;
        bw_safe_delete_array(data_);
        throw;
    }
}


/*virtual*/ RawImportDlg::~RawImportDlg()
{
	BW_GUARD;

    bw_safe_delete_array(data_);
}


void RawImportDlg::getResult
(
    unsigned int    &width, 
    unsigned int    &height, 
    bool            &littleEndian
) const
{
    width           = selWidth_;
    height          = selHeight_;
    littleEndian    = littleEndian_;
}


/*virtual*/ BOOL RawImportDlg::OnInitDialog()
{
	BW_GUARD;

    BOOL result = CDialog::OnInitDialog();
    if (result != FALSE)
    {
        filenameEdit_.SetWindowText( bw_utf8tow( filename_ ).c_str());

        // Add the prime factors of dataSize/2 into the size array:
        size_t numWords = dataSize_/2;
        for (size_t i = 1; i <= numWords; ++i)
        {
            if (numWords%i == 0)
            {
				BW::string sizeStr = 
                    sformat("{0} x {1}", i, numWords/i);
                int idx = sizeCB_.AddString( bw_utf8tow( sizeStr.c_str() ).c_str());
                // Store the width in the item data.  The height is easy to
                // calculate from this.
                sizeCB_.SetItemData(idx, i);
            }
        }
        sizeCB_.SetCurSel(sizeCB_.GetCount()/2); // roughly the sqrt posn.

        bmpImage_.subclass(IDC_RAWIMP_BMP, this);
        bmpImage_.borderPadding(2);

        littleEndianButton_.SetCheck(BST_CHECKED);

        OnUpdateImage();

        INIT_AUTO_TOOLTIP();
    }
    return result;
}


/*virtual*/ void RawImportDlg::DoDataExchange(CDataExchange *dx)
{
    DDX_Control(dx, IDC_RAWIMP_FILENAME    , filenameEdit_      );
    DDX_Control(dx, IDC_RAWIMP_FILENAME    , filenameEdit_      );
    DDX_Control(dx, IDC_RAWIMP_SIZE        , sizeCB_            );
    DDX_Control(dx, IDC_RAWIMP_LITTLEENDIAN, littleEndianButton_);
    DDX_Control(dx, IDC_RAWIMP_BIGENDIAN   , bigEndianButton_   );
}


/*virtual*/ void RawImportDlg::OnOK()
{
	BW_GUARD;

    int idx = sizeCB_.GetCurSel();

	DWORD_PTR itemData = sizeCB_.GetItemData( idx );
	MF_ASSERT( itemData <= UINT_MAX );
    selWidth_     = ( unsigned int ) itemData;
    selHeight_    = static_cast<uint>((dataSize_/2)/selWidth_);
    littleEndian_ = (littleEndianButton_.GetCheck() == BST_CHECKED);

    CDialog::OnOK();
}


void RawImportDlg::OnUpdateImage()
{
	BW_GUARD;

    CWaitCursor waitCursor; // this may take a little while

    int          idx    = sizeCB_.GetCurSel();

	DWORD_PTR itemData = sizeCB_.GetItemData( idx );
	MF_ASSERT( itemData <= UINT_MAX );
    unsigned int width  = ( unsigned int ) itemData;
    unsigned int height = static_cast<uint>((dataSize_/2)/width);
    controls::DibSection8  &image = bmpImage_.image();
    image.resize(width, height);
    if (littleEndianButton_.GetCheck() == BST_CHECKED)
    {
        // Find the range of the data:
        uint16 minv = 65535;
        uint16 maxv =     0;
        for (unsigned int i = 0; i < image.height(); ++i)
        {
            uint16 *q = (uint16 *)data_ + i*width;
            for (unsigned int j = 0; j < image.width(); ++j, ++q)
            {
                minv = std::min(minv, *q);
                maxv = std::max(maxv, *q);
            }
        }
        // Draw the normalised data:
        for (unsigned int i = 0; i < image.height(); ++i)
        {
            uint8    *p = image.getRow(i);
            uint16   *q = (uint16 *)data_ + i*width;
            for (unsigned int j = 0; j < image.width(); ++j, ++q, ++p)
            {
                *p = (uint8)Math::lerp(*q, minv, maxv, 0, 255);
            }
        }
    }
    else
    {
        // Find the range of the data:
        uint16 minv = 65535;
        uint16 maxv =     0;
        for (unsigned int i = 0; i < image.height(); ++i)
        {
            uint16 *q = (uint16 *)data_ + i*width;
            for (unsigned int j = 0; j < image.width(); ++j, ++q)
            {
                uint16 value = *q;
                value = ((value & 0xff) << 8) + (value >> 8);
                minv = std::min(minv, value);
                maxv = std::max(maxv, value);
            }
        }
        // Draw the normalised data:
        for (unsigned int i = 0; i < image.height(); ++i)
        {
            uint8    *p = image.getRow(i);
            uint16   *q = (uint16 *)data_ + i*width;
            for (unsigned int j = 0; j < image.width(); ++j, ++q, ++p)
            {
                uint16 value = *q;
                value = ((value & 0xff) << 8) + (value >> 8);
                uint8 grey = (uint8)Math::lerp(value, minv, maxv, 0, 255);
                *p = grey;
            }
        }
    }
    bmpImage_.Invalidate();
    bmpImage_.RedrawWindow();
}

BW_END_NAMESPACE

