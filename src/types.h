#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

// VC6 Compiler issues
#if defined(_MSC_VER)
#   pragma warning(disable:4786) // Disable "symbol truncated to 255 characters" compiler warning
#   pragma warning(disable:4800) // Disable "conversion from int to bool" compiler performance warning
#endif

#include <algorithm>
#include <list>
#include <string>
#include <map>
#include <vector>

class Object;
class MovingObject;

#define YesNo(i)    ((i) ? "Yes" : "No")
#define Loading(i)  "\n====== %s loading ======\n", (i)

#define xu4_list                    std::list       
#define xu4_vector                  std::vector     
#define xu4_set                     std::set
#define xu4_multiset                std::multiset
#define xu4_map                     std::map        // assosciative array
#define xu4_multimap                std::multimap
namespace xu4 {
    typedef std::string string;    
}

typedef xu4_list<Object*> ObjectList;

using namespace xu4;

#endif
