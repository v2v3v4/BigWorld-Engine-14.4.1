#include "pch.hpp"
#include "gui_utilities.hpp"

BW_BEGIN_NAMESPACE

float StringToFloat(CString stringValue)
{
	BW_GUARD;

	return (float)(_wtof(stringValue.GetString()));
}

int StringToInt(CString stringValue)
{
	BW_GUARD;

	return (int)(_wtoi(stringValue.GetString()));
}

CString FloatToString(float value)
{
	BW_GUARD;

	wchar_t tempString[20];
	bw_snwprintf(tempString, ARRAY_SIZE(tempString), L"%.2f", value);
	return CString(tempString);
}

CString IntToString(int value)
{
	BW_GUARD;

	wchar_t tempString[20];
	bw_snwprintf(tempString, ARRAY_SIZE(tempString), L"%d", value);
	return CString(tempString);
}

COLORREF RGBtoCOLORREF(const Vector4 & color)
{
	return RGB((BYTE)(color.x*255), (BYTE)(color.y*255), (BYTE)(color.z*255));
}

Vector4 COLORREFtoRGB(const COLORREF & colorREF)
{
	return 
        Vector4
        (
            GetRValue(colorREF)/255.f, 
            GetGValue(colorREF)/255.f, 
            GetBValue(colorREF)/255.f, 
            1.0f
        );
}

void 
GetFilenameAndDirectory
(
    CString     const &longFilename, 
    CString     &filename, 
    CString     &directory
)
{
	BW_GUARD;

	filename  = "";
	directory = "";

	if (longFilename == "")
		return;

	char directorySeperator = '/';
	int lastSeperator = longFilename.ReverseFind(directorySeperator);
	int stringLength  = longFilename.GetLength();
	
	if (lastSeperator == stringLength)
	{
		// this is a directory
		directory = longFilename;
	}
	else if (lastSeperator == -1)
	{
		// this is a filename
		filename = longFilename;
	}
	else
	{
		filename  = longFilename.Right(stringLength - lastSeperator - 1);
		directory = longFilename.Left(lastSeperator + 1);

		// make sure only one directory seperator
		directory.TrimRight(L"/");
		directory += L"/";
	}
}

void 
PopulateComboBoxWithFilenames
(
    CComboBox               &theBox, 
    BW::string             const &dir, 
    PopulateTestFunction    test
)
{
	BW_GUARD;

    theBox.ResetContent();
    theBox.ShowWindow(SW_HIDE);
    DataSectionPtr dataSection = BWResource::openSection(dir);    
    if (dataSection)
    {
        theBox.InitStorage(dataSection->countChildren(), 32);

        int idx = 0;
        for
        (
            DataSectionIterator it = dataSection->begin();
            it != dataSection->end();
            ++it
        )
        {
            BW::string filename = (*it)->sectionName();
			BW::string fullname = dir + filename;
            if (test == NULL || test( fullname ))
            {
                theBox.InsertString( idx, bw_utf8tow( filename ).c_str() );
                ++idx;
            }
        }
    }
    theBox.ShowWindow(SW_SHOW);
}
BW_END_NAMESPACE

