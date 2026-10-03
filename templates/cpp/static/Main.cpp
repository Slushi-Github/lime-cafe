#include <stdio.h>
#include <hx/Debug.h>

#if defined(__WIIU__)
#include <whb/proc.h>
#include <whb/gfx.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#include <whb/sdcard.h>
#include <whb/crash.h>
#include <unistd.h>
#include <coreinit/debug.h>
#endif

#if defined(HX_WINDOWS) && !defined(HXCPP_DEBUGGER)
#include <windows.h>
#endif

extern "C" const char *hxRunLibrary();
extern "C" void hxcpp_set_top_of_stack();

extern "C" int zlib_register_prims();
extern "C" int lime_cairo_register_prims();
extern "C" int lime_openal_register_prims();
::foreach ndlls:: ::if (registerStatics)::
	extern "C" int ::nameSafe::_register_prims();
::end:: ::end::

#if defined(HX_WINDOWS) && !defined(HXCPP_DEBUGGER)
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#else
	extern "C" int main(int argc, char *argv[])
{
#endif
#if defined(__WIIU__)
	WHBProcInit();
	WHBInitCrashHandler();
	WHBLogUdpInit();
	WHBMountSdCard();
	WHBGfxInit();
	OSReport("[Main.cpp] Running on WiiU! Initializing rest of HXCPP and Lime...\n");
	WHBLogPrintf("[Main.cpp] Running on WiiU! Initializing rest of HXCPP and Lime...\n");

		int guardCounter = 0;
	while (!WHBProcIsRunning())
	{
		guardCounter++;
		if (guardCounter > 100000)
		{
			OSReport("[Main.cpp] Timed out waiting for ProcUI foreground, aborting.\n");
			WHBLogPrintf("[Main.cpp] Timed out waiting for ProcUI foreground, aborting.\n");
			WHBProcShutdown();
			return -1;
		}
	}
	OSReport("[Main.cpp] ProcUI foreground acquired.\n");
	WHBLogPrintf("[Main.cpp] ProcUI foreground acquired.\n");

#endif

	hxcpp_set_top_of_stack();

	zlib_register_prims();
	lime_cairo_register_prims();
	lime_openal_register_prims();
	::foreach ndlls:: ::if (registerStatics)::
		::nameSafe::_register_prims();
	::end:: ::end::

		const char *err = NULL;
	err = hxRunLibrary();

	if (err)
	{
		char error_message[512];
		snprintf(error_message, sizeof(error_message), "[Main.cpp - hxRunLibrary()] ERROR: %s\n", err);

		printf("%s", error_message);
		#if defined(__WIIU__)
		OSReport(error_message);
		WHBLogPrintf(error_message);
		OSFatal(error_message);

		WHBUnmountSdCard();
		WHBLogUdpDeinit();
		WHBProcShutdown();
		#endif
		return -1;
	}

#if defined(__WIIU__)
	WHBUnmountSdCard();
	WHBLogUdpDeinit();
	WHBProcShutdown();
#endif

	return 0;
}
