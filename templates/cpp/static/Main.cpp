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
#include <coreinit/memheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <nn/erreula.h>
#include <vpad/input.h>

using namespace nn::erreula;

static uint32_t probeLargestFree()
{
	uint32_t lo = 0, hi = 256u * 1024 * 1024; // 256 MB
	while (hi - lo > 64 * 1024)
	{
		uint32_t mid = lo + (hi - lo) / 2;
		void *p = MEMAllocFromDefaultHeapEx(mid, 4);
		if (p)
		{
			MEMFreeToDefaultHeap(p);
			lo = mid;
		}
		else
		{
			hi = mid;
		}
	}
	return lo;
}

static void logMem(const char *tag)
{
	uint32_t largest = probeLargestFree();
	OSReport("[MEM] %s: largest_alloc=%u KB\n", tag, largest / 1024);
	WHBLogPrintf("[MEM] %s: largest_alloc=%u KB", tag, largest / 1024);
}

static OSThread sMemThread;
alignas(16) static uint8_t sMemThreadStack[16 * 1024];

static int memThreadEntry(int argc, const char **argv)
{
	for (;;)
	{
		logMem("tick");
		OSSleepTicks(OSMillisecondsToTicks(1000));
	}
	return 0;
}
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

	logMem("before hxRunLibrary");

	OSCreateThread(&sMemThread, memThreadEntry, 0, NULL,
		sMemThreadStack + sizeof(sMemThreadStack), sizeof(sMemThreadStack),
		16, OS_THREAD_ATTRIB_AFFINITY_ANY);
	OSSetThreadName(&sMemThread, "MemLogger");
	OSResumeThread(&sMemThread);
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
		char log_message[2048];
		snprintf(log_message, sizeof(log_message), "[Main.cpp - hxRunLibrary()] ERROR: %s\n", err);
		printf("%s", log_message);

#if defined(__WIIU__)
		OSReport("%s", log_message);
		WHBLogPrintf("%s", log_message);

		OSSuspendThread(&sMemThread);

		char display_message[4096];
		snprintf(display_message, sizeof(display_message),
			"%s\n\n"
			"(press exit or press A on the Wii U Gamepad to return to Wii U Menu)",
			err);

		char16_t u16_error[4096];
		size_t i = 0;
		for (; i < (sizeof(u16_error) / sizeof(u16_error[0]) - 1) && display_message[i] != '\0'; ++i)
		{
			u16_error[i] = (char16_t)(uint8_t)display_message[i];
		}
		u16_error[i] = u'\0';

		FSClient *fsClient = (FSClient *)MEMAllocFromDefaultHeapEx(sizeof(FSClient), 0x40);
		if (fsClient)
		{
			FSAddClient(fsClient, FS_ERROR_FLAG_NONE);
		}

		uint32_t workMemSize = GetWorkMemorySize();
		void *workMem = MEMAllocFromDefaultHeapEx(workMemSize, 0x40);

		if (workMem != nullptr && fsClient != nullptr)
		{
			CreateArg createArg;
			createArg.workMemory = workMem;
			createArg.region     = RegionType::USA;
			createArg.language   = LangType::English;
			createArg.fsClient   = fsClient;

			if (Create(createArg))
			{
				AppearArg appearArg;
				appearArg.errorArg.errorType      = ErrorType::Message1Button;
				appearArg.errorArg.renderTarget   = RenderTarget::Both;
				appearArg.errorArg.controllerType = ControllerType::DrcGamepad;
				appearArg.errorArg.errorTitle     = u" Haxe Error";
				appearArg.errorArg.errorMessage   = u16_error;
				appearArg.errorArg.button1Label   = u"Exit";

				AppearErrorViewer(appearArg);

				bool dialogActive = true;
				uint32_t frameCounter = 0;

				while (dialogActive)
				{
					VPADStatus vpad;
					VPADReadError vpadErr;
					VPADRead(VPAD_CHAN_0, &vpad, 1, &vpadErr);

					if (vpadErr == VPAD_READ_SUCCESS)
					{
						VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpad.tpFiltered1, &vpad.tpNormal);
						VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpad.tpNormal, &vpad.tpNormal);
					}

					ControllerInfo ctrlInfo;
					ctrlInfo.vpad = (vpadErr == VPAD_READ_SUCCESS) ? &vpad : nullptr;
					Calc(ctrlInfo);

					if (IsDecideSelectButtonError())
					{
						dialogActive = false;
					}

					if (vpadErr == VPAD_READ_SUCCESS && frameCounter > 20)
					{
						if ((vpad.trigger & (VPAD_BUTTON_A)))
						{
							dialogActive = false;
						}
					}

					frameCounter++;

					WHBGfxBeginRender();
					WHBGfxClearColor(0.0f, 0.0f, 0.0f, 1.0f);

					WHBGfxBeginRenderTV();
					DrawTV();
					WHBGfxFinishRenderTV();

					WHBGfxBeginRenderDRC();
					DrawDRC();
					WHBGfxFinishRenderDRC();

					WHBGfxFinishRender();
				}

				DisappearErrorViewer();
				Destroy();
			}
			MEMFreeToDefaultHeap(workMem);
		}

		if (fsClient)
		{
			FSDelClient(fsClient, FS_ERROR_FLAG_NONE);
			MEMFreeToDefaultHeap(fsClient);
		}

		WHBGfxShutdown();
		WHBUnmountSdCard();
		WHBLogUdpDeinit();
		WHBProcShutdown();
#endif
		return -1;
	}

#if defined(__WIIU__)
	OSCancelThread(&sMemThread);
	WHBGfxShutdown();
	WHBUnmountSdCard();
	WHBLogUdpDeinit();
	WHBProcShutdown();
#endif

	return 0;
}