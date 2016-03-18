#pragma once

#ifdef ASP_DLL_EXPORTS
#define ASP_LIBAPI extern "C" __declspec(dllexport)
#else
#define ASP_LIBAPI extern "C" __declspec(dllimport)
#endif