#include <Windows.h>

typedef void* ADDRESS;

void GetCodeSection(ADDRESS buffer, IMAGE_SECTION_HEADER*& section_header, ADDRESS& section) {

	IMAGE_NT_HEADERS32* NTHeader = (IMAGE_NT_HEADERS32*)((DWORD)buffer + (DWORD) * (DWORD*)((DWORD)buffer + 0x3C));
	IMAGE_SECTION_HEADER* _section = (IMAGE_SECTION_HEADER*)((DWORD)NTHeader + sizeof(IMAGE_NT_HEADERS32));


	for (int i = 0; i < NTHeader->FileHeader.NumberOfSections; i++) {

		if (_section->VirtualAddress == NTHeader->OptionalHeader.BaseOfCode) {
			section_header = _section;
			section = (ADDRESS)((DWORD)buffer + _section->PointerToRawData);
			return;
		}

		_section = (IMAGE_SECTION_HEADER*)((DWORD)_section + sizeof(IMAGE_SECTION_HEADER));
	}
}

void SetRelocToNull(ADDRESS buffer) {
	IMAGE_NT_HEADERS32* NTHeader = (IMAGE_NT_HEADERS32*)((DWORD)buffer + (DWORD) * (DWORD*)((DWORD)buffer + 0x3C));
	NTHeader->OptionalHeader.DataDirectory[5].VirtualAddress = 0;
	NTHeader->OptionalHeader.DataDirectory[5].Size = 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	
	HANDLE hGameFile;
	HANDLE hBinFile;
	ADDRESS GameBuffer;
	ADDRESS BinBuffer;
	DWORD GameFileSize;
	DWORD BinFileSize;
	IMAGE_DOS_HEADER* GameMZHeader;
	IMAGE_DOS_HEADER* BinMZHeader;
	IMAGE_NT_HEADERS32* GameNTHeader;
	IMAGE_NT_HEADERS32* BinNTHeader;
	IMAGE_SECTION_HEADER* GameCodeSectionHeader;
	IMAGE_SECTION_HEADER* BinCodeSectionHeader;
	ADDRESS GameCodeSection;
	ADDRESS BinCodeSection;
	
	hGameFile = CreateFile("hl2.exe", GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hGameFile == INVALID_HANDLE_VALUE) {
		MessageBox(0, "cannot open hl2.exe!", "ERROR!", MB_OK);
		return -1;
	}
	hBinFile = CreateFile(TEXT("cheat_bin.dll"), GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hBinFile == INVALID_HANDLE_VALUE) {
		MessageBox(0, "cannot open cheat_bin.dll!", "ERROR!", MB_OK);
		return -1;
	}

	GameFileSize = GetFileSize(hGameFile, NULL);
	BinFileSize = GetFileSize(hBinFile, NULL);


	GameBuffer = VirtualAlloc(NULL, GameFileSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	BinBuffer = VirtualAlloc(NULL, BinFileSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (GameBuffer == NULL || BinBuffer == NULL) {
		MessageBox(0, "Cannot allocate buffer!", "ERROR!", MB_OK);
		return -1;
	}

	if (!ReadFile(hGameFile, GameBuffer, GameFileSize, NULL, NULL)) {
		MessageBox(0, "cannot read hl2.exe!", "ERROR!", MB_OK);
		return -1;
	}
	if (!ReadFile(hBinFile, BinBuffer, BinFileSize, NULL, NULL)) {
		MessageBox(0, "cannot read cheat_bin.dll!", "ERROR!", MB_OK);
		return -1;
	}

	GameMZHeader = (IMAGE_DOS_HEADER*)GameBuffer;
	BinMZHeader = (IMAGE_DOS_HEADER*)BinBuffer;

	if (GameMZHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		MessageBox(0, "hl2.exe is not a PE!, What?", "ERROR!", MB_OK);
		return -1;
	}
	if (BinMZHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		MessageBox(0, "cheat_bin.dll is not a PE!, What?", "ERROR!", MB_OK);
		return -1;
	}

	GameNTHeader = (IMAGE_NT_HEADERS32*)((DWORD)GameBuffer + (DWORD)*(DWORD*)((DWORD)GameBuffer + 0x3C));
	BinNTHeader = (IMAGE_NT_HEADERS32*)((DWORD)BinBuffer + (DWORD)*(DWORD*)((DWORD)BinBuffer + 0x3C));

	SetRelocToNull(GameBuffer);

	GetCodeSection(GameBuffer, GameCodeSectionHeader, GameCodeSection);
	GetCodeSection(BinBuffer, BinCodeSectionHeader, BinCodeSection);

	GameNTHeader->OptionalHeader.AddressOfEntryPoint = GameCodeSectionHeader->VirtualAddress;
	GameCodeSectionHeader->Characteristics |= BinCodeSectionHeader->Characteristics;

	memcpy(GameCodeSection, BinCodeSection, *(DWORD*)((DWORD)BinCodeSection + 2));

	SetFilePointer(hGameFile, 0, NULL, FILE_BEGIN);
	WriteFile(hGameFile, GameBuffer, GameFileSize, NULL, NULL);

	MessageBox(0, "Success to write PE on hl2.exe!", "Success!", MB_OK);

	CloseHandle(hGameFile);
	CloseHandle(hBinFile);
	return 0;
}

