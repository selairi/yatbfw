#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <iostream>

/*! A set of macros to debug messages.
 */


#define debug if(m_debug) std::cerr << "[" << __PRETTY_FUNCTION__ << "] Line:" << __LINE__ << "\t"

#define debug_get_func std::string("[") + std::string(__PRETTY_FUNCTION__) + std::string("]")

#define debug_error std::cerr << "ERROR: [" << __PRETTY_FUNCTION__ << "] Line:" << __LINE__ << "\t"

void debug_printstacktrace();

extern bool m_debug;

#endif
