
#include "Modify.h"
#include "Logger.h"
#include <array>
#ifndef NDEBUG
#include <include/capi/cef_urlrequest_capi.h>
#endif
/*
*
* Black banner still show even libcef hooked.
*
*/

static Logger g_Logger;
//using _cef_urlrequest_create = cef_urlrequest_t * (*)(struct _cef_request_t* request,
//	struct _cef_urlrequest_client_t* client,
//	struct _cef_request_context_t* request_context);

using _cef_urlrequest_create = void* (*)(void* request,
	void* client,
	void* request_context);

using _cef_string_userfree_utf16_free = void* (*)(/*void* str*/void* addr);


static _cef_urlrequest_create cef_urlrequest_create_orig;

static _cef_string_userfree_utf16_free cef_string_userfree_utf16_free_orig;

static constexpr auto block_list = { L"/ads/", L"/ad-logic/", L"/gabo-receiver-service/" };
//static constexpr char search_str[] = {0x61,0x70,0x70,0x2D,0x64,0x65,0x76,0x65,0x6C,0x6F,0x70,0x65,0x72,0x09,0x01,0x30,0x78};

static bool xpui_found = false;
static DWORD ret_addr = 0;
static DWORD buff_addr = 0;
static DWORD buff_size = 0;

DWORD WINAPI get_url(DWORD pRequest)
{
	DWORD retval;
	__asm
	{
		mov eax, pRequest
		mov ecx, dword ptr ds : [eax + 0x18]
		mov edx, eax
		push edx
		call ecx
		mov retval, eax
	}
	return retval;
}


DWORD WINAPI get_str(DWORD pRequest)
{
	DWORD retval;
	__asm
	{
		// get url str
		mov ecx, pRequest
		mov eax, dword ptr ds : [ecx]
		mov retval, eax
	}
	return retval;
}
#ifndef NDEBUG
void* cef_urlrequest_create_hook(struct _cef_request_t* request, void* client, void* request_context)
#else
void* cef_urlrequest_create_hook(void* request, void* client, void* request_context)
#endif
{
#ifndef NDEBUG
	cef_string_utf16_t* url_utf16 = request->get_url(request);
	std::wstring url(url_utf16->str);
#else
	auto url_utf16 = get_url(reinterpret_cast<DWORD>(request));
	std::wstring url(reinterpret_cast<wchar_t*>(get_str(url_utf16)));
#endif
	cef_string_userfree_utf16_free_orig(reinterpret_cast<void*>(url_utf16));
	//cef_string_userfree_utf16_free(url_utf16);
	for (auto blockurl : block_list) {
		if (std::wstring_view::npos != url.find(blockurl)) {
			g_Logger.Log(L"blocked - " + url);

			return nullptr;
		}
	}

	g_Logger.Log(L"allow - " + url);
	return cef_urlrequest_create_orig(request, client, request_context);
}


// https://www.unknowncheats.me/forum/1064672-post23.html
bool DataCompare(BYTE* pData, BYTE* bSig, const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bSig)
	{
		if (*szMask == 'x' && *pData != *bSig)
			return false;
	}
	return (*szMask) == NULL;
}

BYTE* FindPattern(BYTE* dwAddress, const DWORD dwSize, BYTE* pbSig, const char* szMask)
{
	DWORD length = strlen(szMask);
	for (DWORD i = NULL; i < dwSize - length; i++)
	{
		__try
		{
			if (DataCompare(dwAddress + i, pbSig, szMask))
				return dwAddress + i;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	return 0;
}

DWORD WINAPI KillAds(LPVOID)
{
	constexpr auto libcef_str{ L"libcef.dll" };
	auto hModule = GetModuleHandle(libcef_str);
	if (!hModule)
		hModule = LoadLibrary(libcef_str);

	if (hModule)
	{
		cef_urlrequest_create_orig = /*cef_urlrequest_create;*/reinterpret_cast<_cef_urlrequest_create>(GetProcAddress(hModule, "cef_urlrequest_create"));

		cef_string_userfree_utf16_free_orig = /*_cef_string_userfree_utf16_free;*/reinterpret_cast<_cef_string_userfree_utf16_free>(GetProcAddress(hModule, "cef_string_userfree_utf16_free"));

		if (cef_urlrequest_create_orig && cef_string_userfree_utf16_free_orig) {
			auto result = Mhook_SetHook(reinterpret_cast<PVOID*>(&cef_urlrequest_create_orig), cef_urlrequest_create_hook);
			result ? g_Logger.Log(L"libcef.dll patch success!") : g_Logger.Log(L"libcef.dll patch failed!");
		}
	}
	return 0;
}

void WINAPI modify_buffer()
{
	if (false == xpui_found) {
		auto skipPod = FindPattern((uint8_t*)buff_addr, buff_size, (BYTE*)"{height:122px}}#", "xxxxxxxxxxxxxxxx");
		if (skipPod)
		{
			memset((char*)skipPod + 8, 0x30, 3); // 122 to 000
			xpui_found = true;
		}
	}
}

__declspec(naked) void hook_zip_buffer()
{
	__asm
	{
		add eax, edi;
		mov buff_addr, eax;
		push eax;
		call edx;
		mov buff_size, eax;
		//------------ preparation --------------------
		pushad;
		//------------ function call ------------------
		call modify_buffer;
		//------------ end call ------------------
		popad;
		//------------ finish -------------------------
		push ret_addr;
		retn;
	}
}

DWORD WINAPI Developer(LPVOID)
{
	const HMODULE hModule = GetModuleHandle(NULL);
	MODULEINFO mInfo = { 0 };
	if (true == GetModuleInformation(GetCurrentProcess(), hModule, &mInfo, sizeof(MODULEINFO))) {
		if (true == g_Logger.read(L"Config", L"Banner")) {
			auto skipPod = FindPattern((uint8_t*)hModule, mInfo.SizeOfImage, (BYTE*)"\x8B\x45\xEC\x03\xC7\x50\xFF\xD2\x03\xF8", "xxxxxxxxxx");
			if (skipPod)
			{
				const DWORD dwTmp = reinterpret_cast<DWORD>(skipPod) + 3;
				ret_addr = dwTmp + 5;
				DWORD oldProtect;
#pragma region Zip_buffer_hook
				VirtualProtect((LPVOID)dwTmp, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
				*(BYTE*)dwTmp = 0xE9; // jmp
				*(DWORD*)(dwTmp + 1) = (DWORD)hook_zip_buffer - dwTmp - 5;
				VirtualProtect((LPVOID)dwTmp, 5, oldProtect, &oldProtect);
#pragma endregion
				g_Logger.Log(L"Banner - patch success!");
			}
			else {
				g_Logger.Log(L"Banner - patch failed!");
			}

		}

		if (true == g_Logger.read(L"Config", L"Developer")) {
			auto skipPod = FindPattern((uint8_t*)hModule, mInfo.SizeOfImage, (BYTE*)"\x25\x01\xFF\xFF\xFF\x89\x85\xF8\xFB\xFF\xFF", "xxxxxx???xx");
			if (skipPod)
			{
				DWORD oldProtect;
				VirtualProtect((char*)skipPod + 0, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
				memset((char*)skipPod + 0, 0xB8, 1);
				VirtualProtect((char*)skipPod + 0, 1, oldProtect, &oldProtect);

				VirtualProtect((char*)skipPod + 1, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
				memset((char*)skipPod + 1, 0x03, 1);
				VirtualProtect((char*)skipPod + 1, 1, oldProtect, &oldProtect);

				VirtualProtect((char*)skipPod + 2, 3, PAGE_EXECUTE_READWRITE, &oldProtect);
				memset((char*)skipPod + 2, 0x00, 3);
				VirtualProtect((char*)skipPod + 2, 3, oldProtect, &oldProtect);
				g_Logger.Log(L"Developer - patch success!");
			}
			else {
				g_Logger.Log(L"Developer - patch failed!");
			}

		}
	}
	else {
		g_Logger.Log(L"GetModuleInformation - failed!");
	}
	return 0;
}
