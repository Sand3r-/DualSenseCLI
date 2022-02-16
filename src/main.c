#include <stdio.h>

#define NOMINMAX
#include <Windows.h>
#include <initguid.h>
#include <Hidclass.h>
#include <Setupapi.h>
#include <hidsdi.h>
#include <malloc.h>

// Error codes
#define DSC_SUCCESS             0
#define DSC_INVALID_COLLECTIONS 1
#define DSC_INVALID_PATH_SIZE   2
#define DSC_STACK_OVERFLOW      3
#define DSC_OUT_OF_BOUNDS       4
#define DSC_INVALID_ARGS		5
#define DSC_DEVICE_DISCONNECTED	5

// HID constants
#define SONY_VENDOR_ID          0x054C
#define SONY_DUALSENSE_DEV_ID   0x0CE6

#define MAX_PATH_LENGTH         260
#define BT_REPORT_LENGTH        547

// Helper macros
#define LOG_E(msg) fprintf(stderr, #msg)

#define MAX_CONTROLLER_NUM 16

typedef enum ConnectionType
{
    UNDEFINED = 0,
    USB = 1,
    BT = 2
} ConnectionType;

typedef struct DeviceInfo
{
    wchar_t path[MAX_PATH_LENGTH];
    ConnectionType type;
    
} DeviceInfo;

typedef struct Device
{
    wchar_t path[MAX_PATH_LENGTH];
    void* handle;
    ConnectionType connectionType;
    char connected;
    unsigned char hidBuffer[BT_REPORT_LENGTH];
} Device;

int EnumDevices(DeviceInfo* infos, int maxInfosNum, unsigned int* devicesFoundNum)
{
    // Obtain handle to a device information set
    HANDLE hidCollectionsHandle = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hidCollectionsHandle == 0 || (hidCollectionsHandle == INVALID_HANDLE_VALUE))
    {
        LOG_E("HID collections handle was invalid");
		return DSC_INVALID_COLLECTIONS;
	}

    unsigned int infoIndex = 0;

    DWORD deviceIndex = 0;
    SP_DEVINFO_DATA deviceInfo;
    deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);
    // Enumerate through all device interfaces info
    while (SetupDiEnumDeviceInfo(hidCollectionsHandle, deviceIndex, &deviceInfo))
    {
        DWORD interfaceIndex = 0;
        SP_DEVICE_INTERFACE_DATA interfaceInfo;
		interfaceInfo.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        while(SetupDiEnumDeviceInterfaces(hidCollectionsHandle, &deviceInfo, &GUID_DEVINTERFACE_HID, interfaceIndex, &interfaceInfo))
        {
            // Query device path size
			DWORD pathSize = 0;
			SetupDiGetDeviceInterfaceDetailW(hidCollectionsHandle, &interfaceInfo, NULL, 0, &pathSize, NULL);

			// Check size
			if (pathSize > (260 * sizeof(wchar_t))) {
				SetupDiDestroyDeviceInfoList(hidCollectionsHandle);
				return DSC_INVALID_PATH_SIZE;
			}

			// Allocate buffer for a path on the stack
			SP_DEVICE_INTERFACE_DETAIL_DATA_W* devicePath = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)_malloca(pathSize);
			if (!devicePath) {
				SetupDiDestroyDeviceInfoList(hidCollectionsHandle);
				return DSC_STACK_OVERFLOW;
			}

            // Obtain a path to device
			devicePath->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
			SetupDiGetDeviceInterfaceDetailW(hidCollectionsHandle, &interfaceInfo, devicePath, pathSize, NULL, NULL);

            // Check if device is reachable
            HANDLE deviceHandle = CreateFileW(devicePath->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, (DWORD)0, NULL);

			if (deviceHandle && (deviceHandle != INVALID_HANDLE_VALUE)) {
				// Get vendor and product id
				unsigned int vendorId = 0;
				unsigned int productId = 0;
				HIDD_ATTRIBUTES deviceAttributes;
				if (HidD_GetAttributes(deviceHandle, &deviceAttributes)) {
					vendorId = deviceAttributes.VendorID;
					productId = deviceAttributes.ProductID;
				}

				// Check if ids match
				if (vendorId == SONY_VENDOR_ID && productId == SONY_DUALSENSE_DEV_ID) {
					// Get pointer to target
					DeviceInfo* info = infos;

					// Copy path
					if (info) {
						wcscpy_s(info->path, MAX_PATH_LENGTH, (const wchar_t*)devicePath->DevicePath);
					}

					// Get preparsed data
					PHIDP_PREPARSED_DATA preparsedData;
					if (HidD_GetPreparsedData(deviceHandle, &preparsedData)) {
						// Get device capabillities
						HIDP_CAPS deviceCaps;
						if (HidP_GetCaps(preparsedData, &deviceCaps) == HIDP_STATUS_SUCCESS) {
							// Check for device connection type
							if (info) {
								// Check if controller matches USB specifications
								if (deviceCaps.InputReportByteLength == 64) {
									info->type = USB;

									// Device found and valid -> Inrement index
									infoIndex++;
								}
								// Check if controler matches BT specifications
								else if(deviceCaps.InputReportByteLength == 78) {
									info->type = BT;

									// Device found and valid -> Inrement index
									infoIndex++;
								}
							}
						}

						// Free preparsed data
						HidD_FreePreparsedData(preparsedData);
					}
				}

				// Close device
				CloseHandle(deviceHandle);
			}

			// Increment index
			interfaceIndex++;

			// Free device from stack
			_freea(devicePath);
        }
        // Increment index
		deviceIndex++;
    }

    // Close device enum list
	SetupDiDestroyDeviceInfoList(hidCollectionsHandle);

    if (devicesFoundNum)
        *devicesFoundNum = infoIndex;

    // Check if array was suficient
	if (infoIndex > maxInfosNum)
		return DSC_OUT_OF_BOUNDS;
    
    return DSC_SUCCESS;
}

int InitDevice(Device* device, DeviceInfo* info)
{
	// Check if pointers are valid
	if (!device || !info) {
		return DSC_INVALID_ARGS;
	}

	// Check len
	if (wcslen(info->path) == 0) {
		return DSC_INVALID_ARGS;
	}

	// Connect to device
	HANDLE deviceHandle = CreateFileW(info->path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, (DWORD)0, NULL);
	if (!deviceHandle || (deviceHandle == INVALID_HANDLE_VALUE)) {
		return DSC_DEVICE_DISCONNECTED;
	}

	// Write to conext
	device->connected = TRUE;
	device->connectionType = info->type;
	device->handle = deviceHandle;
	wcscpy_s(device->path, MAX_PATH_LENGTH, info->path);
}

int main()
{
    DeviceInfo infos[MAX_CONTROLLER_NUM];
    unsigned int devicesNum = 0;
    if (EnumDevices(infos, MAX_CONTROLLER_NUM, &devicesNum) == DSC_SUCCESS)
        printf("Num of found devices: %u\n", devicesNum);

	if (devicesNum == 0)
	{
		printf("No devices found");
		return 0;
	}

	Device device;
	if (InitDevice(&device, &infos[0]) == DSC_SUCCESS)
	{
		printf("Device was successfully initialised");
	}

    return 0;
}