#include <stdio.h>

#if defined(__WIIU__)
#include <coreinit/debug.h>

#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/proc.h>
#include <whb/gfx.h>
#endif

#if defined(HX_WINDOWS) && !defined(HXCPP_DEBUGGER)
#include <windows.h>
#endif

extern "C" const char *hxRunLibrary ();
extern "C" void hxcpp_set_top_of_stack ();

extern "C" int zlib_register_prims ();
extern "C" int lime_cairo_register_prims ();
extern "C" int lime_openal_register_prims ();
::foreach ndlls::::if (registerStatics)::
extern "C" int ::nameSafe::_register_prims ();
::end:: ::end::

#if defined(HX_WINDOWS) && !defined(HXCPP_DEBUGGER)
int __stdcall WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
extern "C" int main(int argc, char *argv[]) {
#endif
#if defined(__WIIU__)
	WHBLogCafeInit();
	WHBLogUdpInit();
	// WHBGfxInit();
	WHBProcInit();
#endif

	hxcpp_set_top_of_stack ();
	
	zlib_register_prims ();
	lime_cairo_register_prims ();
	lime_openal_register_prims ();
	::foreach ndlls::::if (registerStatics)::
	::nameSafe::_register_prims ();::end::::end::
	
	const char *err = NULL;
 	err = hxRunLibrary ();
	
	if (err) {
		printf("[Main.cpp - hxRunLibrary()] ERROR: %s\n", err);

		#if defined(__WIIU__)
		char errorMsg[256];
		snprintf(errorMsg, sizeof(errorMsg), "[Main.cpp - hxRunLibrary()] ERROR:\n%s\n", err);
		OSReportInfo(errorMsg);
		OSFatal(errorMsg);
		return -1;
		#endif
	}

	return 0;
}
