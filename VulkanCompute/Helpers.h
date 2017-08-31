#ifndef HELPERS_H
#define HELPERS_H
#include <sstream>

#ifdef _DEBUG
#  ifdef _MSC_VER
#	ifdef UNICODE
#		define C_STR(x) std::wstring(x.begin(), x.end()).c_str()
#	else
#		define C_STR(x) x.c_str()
#	endif
#    include <windows.h>
#    define TRACE(x) OutputDebugString(C_STR(x));
#    define TRACE_FULL(x)                           \
     do {  std::stringstream s;  s << __FILE__ << ":" << __LINE__ << ": " << (x);   \
		   std::string str = s.str();												\
           OutputDebugString(C_STR(str)); \
        } while(0)
#  else
#    include <iostream>
#    define TRACE(x)  std::clog << (x)
#    define TRACE_FULL(x) std::clog << __FILE__ << ":" << __LINE__ << ": " << (x)
#  endif       
#else
#  define TRACE(x)
#  define TRACE_FULL(x)
#endif

static const std::string errorString(std::string file, int line, std::string err) {
	std::stringstream s;
	s << file << ":" << line << ": " << err;
	return s.str();
}

#define ERRORSTRING(x) errorString(__FILE__, __LINE__, (x))


#endif