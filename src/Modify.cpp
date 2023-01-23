
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

static constexpr std::array<std::wstring_view, 3> block_list = { L"/ads/", L"/ad-logic/", L"/gabo-receiver-service/" };


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
	auto url_utf16 = get_url((DWORD)request);
	std::wstring url(reinterpret_cast<wchar_t*>(get_str(url_utf16)));
#endif
	for (const auto& blockurl : block_list) {
		if (std::wstring_view::npos != url.find (blockurl)) {
			g_Logger.Log(L"blocked - " + url);
			//cef_string_userfree_utf16_free(url_utf16);
			cef_string_userfree_utf16_free_orig((void*)url_utf16);
			return nullptr;
		}
	}
	//cef_string_userfree_utf16_free(url_utf16);
	cef_string_userfree_utf16_free_orig((void*)url_utf16);
	g_Logger.Log(L"allow - " + url);
	return cef_urlrequest_create_orig (request, client, request_context);
}


// https://www.unknowncheats.me/forum/1064672-post23.html
bool DataCompare (BYTE* pData, BYTE* bSig,const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bSig)
	{
		if (*szMask == 'x' && *pData != *bSig)
			return false;
	}
	return (*szMask) == NULL;
}

BYTE* FindPattern (BYTE* dwAddress, const DWORD dwSize, BYTE* pbSig,const char* szMask)
{
	DWORD length = strlen (szMask);
	for (DWORD i = NULL; i < dwSize - length; i++)
	{
		__try
		{
			if (DataCompare (dwAddress + i, pbSig, szMask))
				return dwAddress + i;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	return 0;
}


DWORD WINAPI KillBanner (LPVOID)
{
	auto hModule = GetModuleHandle (L"libcef.dll");
	if (!hModule)
		hModule = LoadLibrary (L"libcef.dll");

	if (hModule)
	{
		cef_urlrequest_create_orig = /*cef_urlrequest_create;*/(_cef_urlrequest_create)GetProcAddress (hModule, "cef_urlrequest_create");
		
		cef_string_userfree_utf16_free_orig = /*cef_urlrequest_create;*/(_cef_string_userfree_utf16_free)GetProcAddress (hModule, "cef_string_userfree_utf16_free");

		if (cef_urlrequest_create_orig && cef_string_userfree_utf16_free_orig) {
			auto result = Mhook_SetHook ((PVOID*)&cef_urlrequest_create_orig, cef_urlrequest_create_hook);
			result ? g_Logger.Log(L"main process - patch success!") : g_Logger.Log(L"main process - patch failed!");
		}
	}
	
	return 0;
}
