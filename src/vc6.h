#ifndef __VC6_H
#define __VC6_H

// VC6 Compiler issues
#if defined(_MSC_VER)
#   pragma warning(disable:4503) // Disable "decorated name length exceeded, name was trancated" compiler warning
#   pragma warning(disable:4786) // Disable "symbol truncated to 255 characters" compiler warning
#   pragma warning(disable:4800) // Disable "conversion from int to bool" compiler performance warning

// VC8:
#   pragma warning(disable:4996) // Disable "'stricmp' was declared deprecated" compiler warning
#endif

#endif
