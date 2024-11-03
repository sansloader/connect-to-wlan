#include <iostream>
#include <Windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "Ole32.lib")

static void wlanNotificationCallback(PWLAN_NOTIFICATION_DATA wlanNotificationData, PVOID)
{
	std::cout << "wlanNotificationCallback: " << wlanNotificationData->NotificationCode << '\n';
}

static void connectToWifi(const std::wstring& ssid, const std::wstring& password)
{
	DWORD clientVersion{ 2 };
	DWORD negotiatedVersion{};
	HANDLE clientHandle{};
	DWORD result{};

	result = WlanOpenHandle(clientVersion, NULL, &negotiatedVersion, &clientHandle);
	if (result != ERROR_SUCCESS)
	{
		std::cout << "ERROR WlanOpenHandle: " << result << std::endl;
		return;
	}

	result = WlanRegisterNotification(clientHandle, WLAN_NOTIFICATION_SOURCE_ACM, true, wlanNotificationCallback, NULL, NULL, NULL);
	if (result != ERROR_SUCCESS)
	{
		std::cout << "ERROR WlanRegisterNotification: " << result << std::endl;
		WlanCloseHandle(clientHandle, NULL);
		return;
	}

	PWLAN_INTERFACE_INFO_LIST interfaceList{};
	result = WlanEnumInterfaces(clientHandle, NULL, &interfaceList);
	if (result != ERROR_SUCCESS)
	{
		std::cout << "ERROR WlanEnumInterfaces: " << result << std::endl;
		// Unregisters notifications on all wireless interfaces
		WlanRegisterNotification(clientHandle, WLAN_NOTIFICATION_SOURCE_NONE, true, NULL, NULL, NULL, NULL);
		WlanCloseHandle(clientHandle, NULL);
		return;
	}

	if (interfaceList->dwNumberOfItems > 0)
	{
		PWLAN_INTERFACE_INFO interfaceInfo = &interfaceList->InterfaceInfo[0];

		WLAN_CONNECTION_PARAMETERS connectionParameters{};
		// A temporary profile will be used to make the connection.
		connectionParameters.wlanConnectionMode = wlan_connection_mode_temporary_profile;
		// When set to NULL, all SSIDs in the profile will be tried.
		connectionParameters.pDot11Ssid = NULL;
		connectionParameters.pDesiredBssidList = NULL;
		connectionParameters.dot11BssType = dot11_BSS_type_any;
		connectionParameters.dwFlags = 0;
		// If wlanConnectionMode is set to wlan_connection_mode_temporary_profile,
		// then strProfile specifies the XML representation of the profile used for the connection. 
		std::wstring strProfile
		{
			L"<?xml version=\"1.0\"?>"
			L"<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">"
			L"<name>" + ssid + L"</name>"
			L"<SSIDConfig><SSID><name>" + ssid + L"</name></SSID></SSIDConfig>"
			L"<connectionType>ESS</connectionType><connectionMode>manual</connectionMode>"
			L"<MSM><security><authEncryption>"
			L"<authentication>WPA2PSK</authentication><encryption>AES</encryption>"
			L"<useOneX>false</useOneX></authEncryption>"
			L"<sharedKey><keyType>passPhrase</keyType><protected>false</protected>"
			L"<keyMaterial>" + password + L"</keyMaterial></sharedKey></security></MSM></WLANProfile>"
		};
		connectionParameters.strProfile = strProfile.c_str();

		result = WlanConnect(clientHandle, &interfaceInfo->InterfaceGuid, &connectionParameters, NULL);
		if (result != ERROR_SUCCESS)
		{
			// Unregisters notifications on all wireless interfaces
			WlanRegisterNotification(clientHandle, WLAN_NOTIFICATION_SOURCE_NONE, true, NULL, NULL, NULL, NULL);
			WlanCloseHandle(clientHandle, NULL);
			std::cout << "ERROR WlanConnect: " << result << std::endl;
		}
	}

	if (interfaceList)
	{
		WlanFreeMemory(interfaceList);
	}

	// Keep program running to observe notifications
	std::cout << "Press Enter to exit..." << '\n';
	std::cin.get();

	// Unregisters notifications on all wireless interfaces
	WlanRegisterNotification(clientHandle, WLAN_NOTIFICATION_SOURCE_NONE, true, NULL, NULL, NULL, NULL);
	WlanCloseHandle(clientHandle, NULL);
}

int main()
{
	std::wstring ssid{ L"<SSID>" };
	std::wstring password{ L"<PASSWORD>" };
	connectToWifi(ssid, password);
	return 0;
}