
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

using _cef_string_userfree_utf16_free = void * (*)(/*void* str*/void* addr);


static _cef_urlrequest_create cef_urlrequest_create_orig;

static _cef_string_userfree_utf16_free cef_string_userfree_utf16_free_orig;

static constexpr auto block_list = { L"/ads/", L"/ad-logic/", L"/gabo-receiver-service/" };
//static constexpr char search_str[] = {0x61,0x70,0x70,0x2D,0x64,0x65,0x76,0x65,0x6C,0x6F,0x70,0x65,0x72,0x09,0x01,0x30,0x78};

DWORD WINAPI get_url(DWORD pRequest)
{
	DWORD retval;
	__asm
	{
		mov eax, pRequest
		mov ecx, dword ptr ds:[eax+0x18]
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
void* cef_urlrequest_create_hook (struct _cef_request_t* request, void* client, void* request_context)
#else
void* cef_urlrequest_create_hook(void* request, void* client, void* request_context)
#endif
{
#ifndef NDEBUG
	cef_string_utf16_t*  url_utf16 = request->get_url (request);
	std::wstring url(url_utf16->str);
#else
	auto url_utf16 = get_url(reinterpret_cast<DWORD>(request));
	std::wstring url(reinterpret_cast<wchar_t*>(get_str(url_utf16)));
#endif
	cef_string_userfree_utf16_free_orig(reinterpret_cast<void*>(url_utf16));
	//cef_string_userfree_utf16_free(url_utf16);
	for (auto blockurl : block_list) {
		if (std::wstring_view::npos != url.find (blockurl)) {
			g_Logger.Log(L"blocked - " + url);
		
			return nullptr;
		}
	}
	
	g_Logger.Log(L"allow - " + url);
	return cef_urlrequest_create_orig (request, client, request_context);
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

int FindPattern2(BYTE* dwAddress, const DWORD dwSize, BYTE* pbSig, const char* szMask)
{
	DWORD length = strlen(szMask);
	for (DWORD i = NULL; i < dwSize - length; i++)
	{
		__try
		{
			if (DataCompare(dwAddress + i, pbSig, szMask))
				return i;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return 0;
		}
	}
	return 0;
}

DWORD WINAPI KillBanner (LPVOID)
{
	constexpr auto libcef_str{ L"libcef.dll" };
	auto hModule = GetModuleHandle(libcef_str);
	if (!hModule)
		hModule = LoadLibrary(libcef_str);

	if (hModule)
	{
		cef_urlrequest_create_orig = /*cef_urlrequest_create;*/reinterpret_cast<_cef_urlrequest_create>(GetProcAddress (hModule, "cef_urlrequest_create"));
		
		cef_string_userfree_utf16_free_orig = /*_cef_string_userfree_utf16_free;*/reinterpret_cast<_cef_string_userfree_utf16_free>(GetProcAddress (hModule, "cef_string_userfree_utf16_free"));

		if (cef_urlrequest_create_orig && cef_string_userfree_utf16_free_orig) {
			auto result = Mhook_SetHook (reinterpret_cast<PVOID*>(&cef_urlrequest_create_orig), cef_urlrequest_create_hook);
			result ? g_Logger.Log(L"main process - patch success!") : g_Logger.Log(L"main process - patch failed!");
		}
	}
	return 0;
}

DWORD WINAPI Developer(LPVOID)
{
	if (true == g_Logger.read(L"Config", L"Developer")) {
		auto appdata = std::getenv("APPDATA");
		std::filesystem::path file{ appdata };
		file = file.parent_path();
		file /= "Local\\Spotify\\offline.bnk";
		std::fstream offline_bnk;
		offline_bnk.open(file, std::ios::in | std::ios::out | std::ios::binary);
		if (true == offline_bnk.is_open()) {
			const auto length = std::filesystem::file_size(file);
			if (length > 0)
			{
				char* temp_buffer = new char[length];
				offline_bnk.rdbuf()->sgetn(temp_buffer, length);
				auto developer = FindPattern2((uint8_t*)temp_buffer, length, 
					(BYTE*)"\x61\x70\x70\x2D\x64\x65\x76\x65\x6C\x6F\x70\x65\x72\x09\x01\x30\x78", 
					"xxxxxxxxxxxxxxxxx");
				//app-developer \x09 \x01 \x30 \x78 -> app-developer(\t\x01)00x
				if (developer) {
					offline_bnk.seekp(developer + 15);
					offline_bnk << char(0x32);//00x -> 02x
					g_Logger.Log(L"offline.bnk modified!");
				}
				else {
					g_Logger.Log(L"offline.bnk not modified!");
				}
				delete[] temp_buffer;
			}
		}
		else {
			g_Logger.Log(L"offline.bnk not exists/access denied!");
		}
	}
	return 0;
}