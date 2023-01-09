/**
 *  String formatting functions (type-safe versions of snprintf).
 *
 *  These functions are like the C# string.format function.  For example you
 *  can do something like:
 *
 *      string name = "Bill";
 *      string s    = sformat("Hello {0}, how are you {0}?", name);
 *
 *  which would generate the string "Hello Bill, how are you Bill?".
 *
 *  To use these functions we require that operator<< is defined for all
 *  objects that are printed.  We also limit the number of objects to ten.
 *  If you want to print more then extend the templates below in the obvious
 *  fashion.
 */


#ifndef FORMAT_HPP
#define FORMAT_HPP

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *  Convert an object to a string.  This assumes that operator<<
 *  is defined for the object.
 *
 *  @param object   The object to convert to a string.
 *  @returns        The object in string form.
 *
 *  We specialise this for string, wstring,
 *  combinations to go via fromStringToWString etc.
 *
 *  Note that this does NOT work with char * and wchar_t *.
 */
template<typename CHAR_TYPE, typename ALLOC_TYPE, typename T>
std::basic_string< CHAR_TYPE, std::char_traits< CHAR_TYPE >, ALLOC_TYPE >
toString(T const &object);

/**
 *  Convert an object from a string.  This assumes that operator>>
 *  is defined for the object, as is the default constructor.
 *
 *  @param str      The string to convert to an object.
 *  @returns        The object constructed from the string.
 */
template<typename CHAR_TYPE, typename T>
T fromString(std::basic_string<CHAR_TYPE> const &str);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param         str  The format string.
 *   @param         obj1 object1.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param         str  The format string.
 *   @param         obj1 object1.
 *   @param         obj2 object2.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @param          obj7 object7.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @param          obj7 object7.
 *   @param          obj8 object8.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @param          obj7 object7.
 *   @param          obj8 object8.
 *   @param          obj9 object9.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @param          obj7 object7.
 *   @param          obj8 object8.
 *   @param          obj9 object9.
 *   @param          obj10 object10.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9,
    typename CLASS10
>
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9,
    CLASS10         const &obj10
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @param          obj7 object7.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @param          obj7 object7.
 *   @param          obj8 object8.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @param          obj7 object7.
 *   @param          obj8 object8.
 *   @param          obj9 object9.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param          str  The format string.
 *   @param          obj1 object1.
 *   @param          obj2 object2.
 *   @param          obj3 object3.
 *   @param          obj4 object4.
 *   @param          obj5 object5.
 *   @param          obj6 object6.
 *   @param          obj7 object7.
 *   @param          obj8 object8.
 *   @param          obj9 object9.
 *   @param          obj10 object10.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9,
    typename CLASS10
>
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9,
    CLASS10         const &obj10
);


#ifdef _MFC_VER

/**
 *  Load a resource string.
 *
 *  @param resid    The resource id of the string to load.
 *  @returns        The loaded string.
 */
BW::string loadString(UINT resid);

/**
 *  Load a resource string.
 *
 *  @param resid    The resource id of the string to load.
 *  @returns        The loaded string.
 */
BW::wstring wloadString(UINT resid);

#endif // _MFC_VER

#ifdef WIN32

/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @param obj7    object7.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @param obj7    object7.
 *   @param obj8    object8.
 *   @returns        The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @param obj7    object7.
 *   @param obj8    object8.
 *   @param obj9    object9.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @param obj7    object7.
 *   @param obj8    object8.
 *   @param obj9    object9.
 *   @param obj10   object10.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9,
    typename CLASS10
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9,
    CLASS10         const &obj10
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @param obj7    object7.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @param obj7    object7.
 *   @param obj8    object8.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @param obj7    object7.
 *   @param obj8    object8.
 *   @param obj9    object9.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9
);


/**
 *  Print to a string.
 *
 *  Print format string replacing {0} etc with the first
 *  object and so on.
 *
 *   @param resid   The resource id of the string to load.
 *   @param obj1    object1.
 *   @param obj2    object2.
 *   @param obj3    object3.
 *   @param obj4    object4.
 *   @param obj5    object5.
 *   @param obj6    object6.
 *   @param obj7    object7.
 *   @param obj8    object8.
 *   @param obj9    object9.
 *   @param obj10   object10.
 *   @returns       The formatted string.
 */
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9,
    typename CLASS10
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9,
    CLASS10         const &obj10
);
#endif // WIN32


/**
 *  Private details
 */
namespace details
{
    bool
    sformatNextObj
    (
        BW::string         &result,
        BW::string         const &str,
        size_t              &pos,
        size_t              &idx
    );

    bool
    wformatNextObj
    (
        BW::wstring        &result,
        BW::wstring        const &str,
        size_t              &pos,
        size_t              &idx
    );
}

//
// Implementation
//


template<typename CHAR_TYPE, typename ALLOC_TYPE, typename T>
inline
std::basic_string< CHAR_TYPE, std::char_traits< CHAR_TYPE >, ALLOC_TYPE >
toString(T const &object)
{
    std::basic_ostringstream< CHAR_TYPE,
		std::char_traits< CHAR_TYPE >,
		ALLOC_TYPE > output;
    output << object;
    return output.str();
}


template<>
inline
BW::string
toString<char, BW::char_allocator, BW::string>(BW::string const &object)
{
    return object;
}


template<>
inline
std::basic_string< wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >
toString(wchar_t * const &object)
{
    return object;
}


template<typename CHAR_TYPE, typename T>
inline
T
fromString(std::basic_string<CHAR_TYPE> const &str)
{
    T result;
    std::basic_istringstream<CHAR_TYPE> input(str);
    input >> result;
    return result;
}


template
<
    typename CLASS1
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%1)
        {
		case 0: result += toString<char, BW::char_allocator>(obj1); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%2)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1); break;
        case 1: result += toString<char, BW::char_allocator>(obj2); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%3)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1); break;
        case 1: result += toString<char, BW::char_allocator>(obj2); break;
        case 2: result += toString<char, BW::char_allocator>(obj3); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%4)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1); break;
        case 1: result += toString<char, BW::char_allocator>(obj2); break;
        case 2: result += toString<char, BW::char_allocator>(obj3); break;
        case 3: result += toString<char, BW::char_allocator>(obj4); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%5)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1); break;
        case 1: result += toString<char, BW::char_allocator>(obj2); break;
        case 2: result += toString<char, BW::char_allocator>(obj3); break;
        case 3: result += toString<char, BW::char_allocator>(obj4); break;
        case 4: result += toString<char, BW::char_allocator>(obj5); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%6)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1); break;
        case 1: result += toString<char, BW::char_allocator>(obj2); break;
        case 2: result += toString<char, BW::char_allocator>(obj3); break;
        case 3: result += toString<char, BW::char_allocator>(obj4); break;
        case 4: result += toString<char, BW::char_allocator>(obj5); break;
        case 5: result += toString<char, BW::char_allocator>(obj6); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%7)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1); break;
        case 1: result += toString<char, BW::char_allocator>(obj2); break;
        case 2: result += toString<char, BW::char_allocator>(obj3); break;
        case 3: result += toString<char, BW::char_allocator>(obj4); break;
        case 4: result += toString<char, BW::char_allocator>(obj5); break;
        case 5: result += toString<char, BW::char_allocator>(obj6); break;
        case 6: result += toString<char, BW::char_allocator>(obj7); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%8)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1); break;
        case 1: result += toString<char, BW::char_allocator>(obj2); break;
        case 2: result += toString<char, BW::char_allocator>(obj3); break;
        case 3: result += toString<char, BW::char_allocator>(obj4); break;
        case 4: result += toString<char, BW::char_allocator>(obj5); break;
        case 5: result += toString<char, BW::char_allocator>(obj6); break;
        case 6: result += toString<char, BW::char_allocator>(obj7); break;
        case 7: result += toString<char, BW::char_allocator>(obj8); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%9)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1); break;
        case 1: result += toString<char, BW::char_allocator>(obj2); break;
        case 2: result += toString<char, BW::char_allocator>(obj3); break;
        case 3: result += toString<char, BW::char_allocator>(obj4); break;
        case 4: result += toString<char, BW::char_allocator>(obj5); break;
        case 5: result += toString<char, BW::char_allocator>(obj6); break;
        case 6: result += toString<char, BW::char_allocator>(obj7); break;
        case 7: result += toString<char, BW::char_allocator>(obj8); break;
        case 8: result += toString<char, BW::char_allocator>(obj9); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9,
    typename CLASS10
>
inline
BW::string
sformat
(
    BW::string     const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9,
    CLASS10         const &obj10
)
{
    BW::string result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::sformatNextObj(result, str, pos, idx))
    {
        switch (idx%10)
        {
        case 0: result += toString<char, BW::char_allocator>(obj1);  break;
        case 1: result += toString<char, BW::char_allocator>(obj2);  break;
        case 2: result += toString<char, BW::char_allocator>(obj3);  break;
        case 3: result += toString<char, BW::char_allocator>(obj4);  break;
        case 4: result += toString<char, BW::char_allocator>(obj5);  break;
        case 5: result += toString<char, BW::char_allocator>(obj6);  break;
        case 6: result += toString<char, BW::char_allocator>(obj7);  break;
        case 7: result += toString<char, BW::char_allocator>(obj8);  break;
        case 8: result += toString<char, BW::char_allocator>(obj9);  break;
        case 9: result += toString<char, BW::char_allocator>(obj10); break;
        }
    }
    return result;
}


template
<
    typename CLASS1
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%1)
        {
		case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%2)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        case 1: result += toString<wchar_t, std::allocator>(obj2); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%3)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        case 1: result += toString<wchar_t, std::allocator>(obj2); break;
        case 2: result += toString<wchar_t, std::allocator>(obj3); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4
)
{
    BW::wstring result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%4)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        case 1: result += toString<wchar_t, std::allocator>(obj2); break;
        case 2: result += toString<wchar_t, std::allocator>(obj3); break;
        case 3: result += toString<wchar_t, std::allocator>(obj4); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%5)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        case 1: result += toString<wchar_t, std::allocator>(obj2); break;
        case 2: result += toString<wchar_t, std::allocator>(obj3); break;
        case 3: result += toString<wchar_t, std::allocator>(obj4); break;
        case 4: result += toString<wchar_t, std::allocator>(obj5); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%6)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        case 1: result += toString<wchar_t, std::allocator>(obj2); break;
        case 2: result += toString<wchar_t, std::allocator>(obj3); break;
        case 3: result += toString<wchar_t, std::allocator>(obj4); break;
        case 4: result += toString<wchar_t, std::allocator>(obj5); break;
        case 5: result += toString<wchar_t, std::allocator>(obj6); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%7)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        case 1: result += toString<wchar_t, std::allocator>(obj2); break;
        case 2: result += toString<wchar_t, std::allocator>(obj3); break;
        case 3: result += toString<wchar_t, std::allocator>(obj4); break;
        case 4: result += toString<wchar_t, std::allocator>(obj5); break;
        case 5: result += toString<wchar_t, std::allocator>(obj6); break;
        case 6: result += toString<wchar_t, std::allocator>(obj7); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%8)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        case 1: result += toString<wchar_t, std::allocator>(obj2); break;
        case 2: result += toString<wchar_t, std::allocator>(obj3); break;
        case 3: result += toString<wchar_t, std::allocator>(obj4); break;
        case 4: result += toString<wchar_t, std::allocator>(obj5); break;
        case 5: result += toString<wchar_t, std::allocator>(obj6); break;
        case 6: result += toString<wchar_t, std::allocator>(obj7); break;
        case 7: result += toString<wchar_t, std::allocator>(obj8); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%9)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1); break;
        case 1: result += toString<wchar_t, std::allocator>(obj2); break;
        case 2: result += toString<wchar_t, std::allocator>(obj3); break;
        case 3: result += toString<wchar_t, std::allocator>(obj4); break;
        case 4: result += toString<wchar_t, std::allocator>(obj5); break;
        case 5: result += toString<wchar_t, std::allocator>(obj6); break;
        case 6: result += toString<wchar_t, std::allocator>(obj7); break;
        case 7: result += toString<wchar_t, std::allocator>(obj8); break;
        case 8: result += toString<wchar_t, std::allocator>(obj9); break;
        }
    }
    return result;
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9,
    typename CLASS10
>
inline
BW::wstring
wformat
(
    BW::wstring    const &str,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9,
    CLASS10         const &obj10
)
{
    BW::wstring  result;
    size_t pos = 0;
    size_t idx = 0;
    while (details::wformatNextObj(result, str, pos, idx))
    {
        switch (idx%10)
        {
        case 0: result += toString<wchar_t, std::allocator>(obj1);  break;
        case 1: result += toString<wchar_t, std::allocator>(obj2);  break;
        case 2: result += toString<wchar_t, std::allocator>(obj3);  break;
        case 3: result += toString<wchar_t, std::allocator>(obj4);  break;
        case 4: result += toString<wchar_t, std::allocator>(obj5);  break;
        case 5: result += toString<wchar_t, std::allocator>(obj6);  break;
        case 6: result += toString<wchar_t, std::allocator>(obj7);  break;
        case 7: result += toString<wchar_t, std::allocator>(obj8);  break;
        case 8: result += toString<wchar_t, std::allocator>(obj9);  break;
        case 9: result += toString<wchar_t, std::allocator>(obj10); break;
        }
    }
    return result;
}


#ifdef WIN32

template
<
    typename CLASS1
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1);
}


template
<
    typename CLASS1,
    typename CLASS2
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1, obj2);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1, obj2, obj3);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1, obj2, obj3, obj4);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1, obj2, obj3, obj4, obj5);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1, obj2, obj3, obj4, obj5, obj6);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1, obj2, obj3, obj4, obj5, obj6, obj7);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1, obj2, obj3, obj4, obj5, obj6, obj7, obj8);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9
)
{
    BW::string s = loadString(resid);
    return sformat(s, obj1, obj2, obj3, obj4, obj5, obj6, obj7, obj8, obj9);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9,
    typename CLASS10
>
BW::string
sformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9,
    CLASS10         const &obj10
)
{
    BW::string s = loadString(resid);
    return
        sformat(s, obj1, obj2, obj3, obj4, obj5, obj6, obj7, obj8, obj9, obj10);
}


template
<
    typename CLASS1
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1);
}


template
<
    typename CLASS1,
    typename CLASS2
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1, obj2);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1, obj2, obj3);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1, obj2, obj3, obj4);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1, obj2, obj3, obj4, obj5);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1, obj2, obj3, obj4, obj5, obj6);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1, obj2, obj3, obj4, obj5, obj6, obj7);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1, obj2, obj3, obj4, obj5, obj6, obj7, obj8);
}

 
template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9
)
{
    BW::wstring s = wloadString(resid);
    return wformat(s, obj1, obj2, obj3, obj4, obj5, obj6, obj7, obj8, obj9);
}


template
<
    typename CLASS1,
    typename CLASS2,
    typename CLASS3,
    typename CLASS4,
    typename CLASS5,
    typename CLASS6,
    typename CLASS7,
    typename CLASS8,
    typename CLASS9,
    typename CLASS10
>
BW::wstring
wformat
(
    UINT            resid,
    CLASS1          const &obj1,
    CLASS2          const &obj2,
    CLASS3          const &obj3,
    CLASS4          const &obj4,
    CLASS5          const &obj5,
    CLASS6          const &obj6,
    CLASS7          const &obj7,
    CLASS8          const &obj8,
    CLASS9          const &obj9,
    CLASS10         const &obj10
)
{
    BW::wstring s = wloadString(resid);
    return
        wformat(s, obj1, obj2, obj3, obj4, obj5, obj6, obj7, obj8, obj9, obj10);
}


#endif // WIN32

BW_END_NAMESPACE

#endif // FORMAT_HPP
