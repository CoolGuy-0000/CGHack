#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

typedef int (*LauncherMain_t)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

typedef struct _PEB_LDR_DATA {
	UINT8 _PADDING_[12];
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _PEB {
#ifdef _WIN64
	UINT8 _PADDING_[24];
#else
	UINT8 _PADDING_[12];
#endif
	PEB_LDR_DATA* Ldr;
} PEB, * PPEB;

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	VOID* DllBase;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

typedef struct _UNLINKED_MODULE
{
	HMODULE hModule;
	PLIST_ENTRY RealInLoadOrderLinks;
	PLIST_ENTRY RealInMemoryOrderLinks;
	PLIST_ENTRY RealInInitializationOrderLinks;
	PLDR_DATA_TABLE_ENTRY Entry;
} UNLINKED_MODULE;

#define UNLINK(x)					\
	(x).Flink->Blink = (x).Blink;	\
	(x).Blink->Flink = (x).Flink;

#define RELINK(x, real)			\
	(x).Flink->Blink = (real);	\
	(x).Blink->Flink = (real);	\
	(real)->Blink = (x).Blink;	\
	(real)->Flink = (x).Flink;

std::vector<UNLINKED_MODULE> UnlinkedModules;

struct FindModuleHandle
{
	HMODULE m_hModule;
	FindModuleHandle(HMODULE hModule) : m_hModule(hModule)
	{
	}
	bool operator() (UNLINKED_MODULE const& Module) const
	{
		return (Module.hModule == m_hModule);
	}
};

void RelinkModuleToPEB(HMODULE hModule)
{
	std::vector<UNLINKED_MODULE>::iterator it = std::find_if(UnlinkedModules.begin(), UnlinkedModules.end(), FindModuleHandle(hModule));

	if (it == UnlinkedModules.end())
	{
		//DBGOUT(TEXT("Module Not Unlinked Yet!"));
		return;
	}

	RELINK((*it).Entry->InLoadOrderLinks, (*it).RealInLoadOrderLinks);
	RELINK((*it).Entry->InInitializationOrderLinks, (*it).RealInInitializationOrderLinks);
	RELINK((*it).Entry->InMemoryOrderLinks, (*it).RealInMemoryOrderLinks);
	UnlinkedModules.erase(it);
}

void UnlinkModuleFromPEB(HMODULE hModule)
{
	std::vector<UNLINKED_MODULE>::iterator it = std::find_if(UnlinkedModules.begin(), UnlinkedModules.end(), FindModuleHandle(hModule));
	if (it != UnlinkedModules.end())
	{
		//DBGOUT(TEXT("Module Already Unlinked!"));
		return;
	}

#ifdef _WIN64
	PPEB pPEB = (PPEB)__readgsqword(0x60);
#else
	PPEB pPEB = (PPEB)__readfsdword(0x30);
#endif

	PLIST_ENTRY CurrentEntry = pPEB->Ldr->InLoadOrderModuleList.Flink;
	PLDR_DATA_TABLE_ENTRY Current = NULL;

	while (CurrentEntry != &pPEB->Ldr->InLoadOrderModuleList && CurrentEntry != NULL)
	{
		Current = CONTAINING_RECORD(CurrentEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (Current->DllBase == hModule)
		{
			UNLINKED_MODULE CurrentModule = { 0 };
			CurrentModule.hModule = hModule;
			CurrentModule.RealInLoadOrderLinks = Current->InLoadOrderLinks.Blink->Flink;
			CurrentModule.RealInInitializationOrderLinks = Current->InInitializationOrderLinks.Blink->Flink;
			CurrentModule.RealInMemoryOrderLinks = Current->InMemoryOrderLinks.Blink->Flink;
			CurrentModule.Entry = Current;
			UnlinkedModules.push_back(CurrentModule);

			UNLINK(Current->InLoadOrderLinks);
			UNLINK(Current->InInitializationOrderLinks);
			UNLINK(Current->InMemoryOrderLinks);

			break;
		}

		CurrentEntry = CurrentEntry->Flink;
	}
}

static char* GetBaseDir(const char* pszBuffer)
{
	static char	basedir[MAX_PATH];
	char szBuffer[MAX_PATH];
	size_t j;
	char* pBuffer = NULL;

	strcpy(szBuffer, pszBuffer);

	pBuffer = strrchr(szBuffer, '\\');
	if (pBuffer)
	{
		*(pBuffer + 1) = '\0';
	}

	strcpy(basedir, szBuffer);

	j = strlen(basedir);
	if (j > 0)
	{
		if ((basedir[j - 1] == '\\') ||
			(basedir[j - 1] == '/'))
		{
			basedir[j - 1] = 0;
		}
	}

	return basedir;
}



DWORD WINAPI LauncherMain(HMODULE module) {
	HMODULE Game = GetModuleHandleA("hl2.exe");
	char* pPath = getenv("PATH");

	char moduleName[MAX_PATH];
	char szBuffer[4096];
	if (!GetModuleFileNameA(Game, moduleName, MAX_PATH))
	{
		MessageBoxA(0, "Failed calling GetModuleFileName", "Launcher Error", MB_OK);
		return 0;
	}

	char* pRootDir = GetBaseDir(moduleName);

	_snprintf(szBuffer, sizeof(szBuffer), "PATH=%s\\bin\\;%s", pRootDir, pPath);
	szBuffer[sizeof(szBuffer) - 1] = '\0';
	_putenv(szBuffer);

	_snprintf(szBuffer, sizeof(szBuffer), "%s\\bin\\launcher.dll", pRootDir);
	szBuffer[sizeof(szBuffer) - 1] = '\0';

	HINSTANCE launcher = LoadLibraryExA(szBuffer, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

	if (!launcher) {
		MessageBoxA(0, "Cannot Run!", "Launcher Error", MB_OK);
		return 0;
	}

	LauncherMain_t main = (LauncherMain_t)GetProcAddress(launcher, "LauncherMain");
	STARTUPINFOA si;
	GetStartupInfoA(&si);
	return main(Game, NULL, GetCommandLineA(), si.wShowWindow);
}

DWORD WINAPI ThreadCheatLoader(HMODULE module) {
	HMODULE final_dll = NULL;
	while (final_dll == NULL) final_dll = GetModuleHandleA("TextShaping.dll");

	HMODULE cheat_dll = LoadLibraryA("cheat.dll");
	if (cheat_dll) UnlinkModuleFromPEB(cheat_dll);
	
	FreeLibraryAndExitThread(module, 0);
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {

	if (fdwReason == DLL_PROCESS_ATTACH) {
		HANDLE MainThread = CreateThread(NULL, 0x1000, (LPTHREAD_START_ROUTINE)LauncherMain, hinstDLL, NULL, NULL);
		if(MainThread)
			SetThreadPriority(MainThread, THREAD_PRIORITY_HIGHEST);
		CreateThread(NULL, 0x1000, (LPTHREAD_START_ROUTINE)ThreadCheatLoader, hinstDLL, NULL, NULL);
	}
	
	return TRUE;
}
