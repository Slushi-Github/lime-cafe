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
#include <coreinit/launch.h>
#include <coreinit/exception.h>
#include <coreinit/context.h>
#include <coreinit/memheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <nn/erreula.h>
#include <vpad/input.h>
#include <coreinit/core.h>
#include <coreinit/memorymap.h>

using namespace nn::erreula;

void logMsg(const char *msg)
{
	OSReport("%s\n", msg);
	WHBLogPrintf("%s\n", msg);
}

static BOOL onSystemCrash(OSContext *context)
{
	uint32_t coreId = OSGetCoreId();
	OSThread *currentThread = OSGetCurrentThread();
	const char *threadName = currentThread ? OSGetThreadName(currentThread) : "Unknown";
	uint32_t threadId = currentThread ? currentThread->id : 0;

	char crashSymbol[128] = "Unknown Symbol";
	char callerSymbol[128] = "Unknown Symbol";

	if (context->srr0 != 0)
	{
		OSGetSymbolName((uint32_t)context->srr0, crashSymbol, sizeof(crashSymbol));
	}
	if (context->lr != 0)
	{
		OSGetSymbolName((uint32_t)context->lr, callerSymbol, sizeof(callerSymbol));
	}

	char backtraceStr[512] = "";
	uint32_t *frame = (uint32_t *)context->gpr[1];
	int depth = 0;

	while (frame != nullptr && OSIsAddressValid((uint32_t)frame) && depth < 5)
	{
		uint32_t savedLr = frame[1];
		if (savedLr != 0 && OSIsAddressValid(savedLr))
		{
			char fnName[128] = "";
			OSGetSymbolName(savedLr, fnName, sizeof(fnName));

			char line[160];
			snprintf(line, sizeof(line), "  #%d [0x%08X] %s\n", depth, savedLr, fnName[0] ? fnName : "???");
			strncat(backtraceStr, line, sizeof(backtraceStr) - strlen(backtraceStr) - 1);
			depth++;
		}
		frame = (uint32_t *)frame[0];
	}

	char crashLog[1536];
	snprintf(crashLog, sizeof(crashLog),
		"=== Wii U Crash Handler ===\nCRASH ON THREAD: [%s] (ID: %u, Core: %u)\n\n"
		"Crash caused by:\n"
		"  %s\n"
		"  PC: 0x%08X | Memory (DAR): 0x%08X\n\n"
		"Called from:\n"
		"  %s (LR: 0x%08X)\n\n"
		"Stack Trace:\n%s"
		"===============================================",
		threadName ? threadName : "unnamed",
		threadId,
		coreId,
		crashSymbol,
		(uint32_t)context->srr0,
		(uint32_t)context->dar,
		callerSymbol,
		(uint32_t)context->lr,
		backtraceStr[0] ? backtraceStr : "  (No available)\n");

	logMsg(crashLog);

	WHBUnmountSdCard();
	WHBLogUdpDeinit();

	OSFatal(crashLog);

	return TRUE;
}

static MEMHeapHandle getActualDefaultHeap()
{
	MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
	if (heap) return heap;

	heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);
	if (heap) return heap;

	void *sample = MEMAllocFromDefaultHeapEx(16, 4);
	if (sample)
	{
		heap = (MEMHeapHandle)MEMFindContainHeap(sample);
		MEMFreeToDefaultHeap(sample);
		return heap;
	}

	return NULL;
}

static uint32_t probeLargestFree(uint32_t *outTotalFree = NULL)
{
	MEMHeapHandle heap = getActualDefaultHeap();
	uint32_t nativeAllocatable = 0;

	if (heap)
	{
		if (outTotalFree)
		{
			*outTotalFree = MEMGetTotalFreeSizeForExpHeap(heap);
		}
		nativeAllocatable = MEMGetAllocatableSizeForExpHeapEx(heap, 4);
	}

	if (nativeAllocatable > 0)
	{
		void *p = MEMAllocFromDefaultHeapEx(nativeAllocatable, 4);
		if (p)
		{
			MEMFreeToDefaultHeap(p);
			return nativeAllocatable;
		}
	}

	uint32_t hi = nativeAllocatable;
	if (hi == 0)
	{
		uint32_t mem2Addr = 0, mem2Size = 0;
		if (OSGetMemBound(OS_MEM2, &mem2Addr, &mem2Size) != 0 || mem2Size == 0)
		{
			mem2Size = 1024u * 1024 * 1024; // 1 GB fallback
		}
		hi = mem2Size;
	}

	uint32_t lo = 0;
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
	uint32_t totalFree = 0;
	uint32_t largest = probeLargestFree(&totalFree);
	char msg[160];

	if (totalFree > 0)
	{
		snprintf(msg, sizeof(msg), "[MEM] %s: largest_alloc=%u KB | total_free=%u KB",
		         tag, largest / 1024, totalFree / 1024);
	}
	else
	{
		snprintf(msg, sizeof(msg), "[MEM] %s: largest_alloc=%u KB",
		         tag, largest / 1024);
	}

	logMsg(msg);
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

	OSSetExceptionCallbackEx(OS_EXCEPTION_MODE_GLOBAL_ALL_CORES, OS_EXCEPTION_TYPE_DSI, onSystemCrash);
	OSSetExceptionCallbackEx(OS_EXCEPTION_MODE_GLOBAL_ALL_CORES, OS_EXCEPTION_TYPE_ISI, onSystemCrash);
	OSSetExceptionCallbackEx(OS_EXCEPTION_MODE_GLOBAL_ALL_CORES, OS_EXCEPTION_TYPE_PROGRAM, onSystemCrash);
	OSSetExceptionCallbackEx(OS_EXCEPTION_MODE_GLOBAL_ALL_CORES, OS_EXCEPTION_TYPE_ALIGNMENT, onSystemCrash);
	logMsg("[Main.cpp] Wii U native crash Handler prepared");

	logMsg("[Main.cpp] Running on WiiU! Initializing rest of HXCPP and Lime...");

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
		logMsg(log_message);

		OSSuspendThread(&sMemThread);

		char display_message[4096];
		snprintf(display_message, sizeof(display_message),
			"%s\n\n"
			"(Press \"Reboot\" button or press A on the Wii U Gamepad to restart console)",
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
				appearArg.errorArg.errorTitle     = u"HaxeError!";
				appearArg.errorArg.errorMessage   = u16_error;
				appearArg.errorArg.button1Label   = u"Reboot console";

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
						OSLaunchTitlel(OS_TITLE_ID_REBOOT, 0);
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