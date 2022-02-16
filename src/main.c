#include <stdio.h>

#define NOMINMAX
#include <Windows.h>
#include <initguid.h>
#include <Hidclass.h>
#include <Setupapi.h>

#define MAX_CONTROLLER_NUM 16
#define SUCCESS 0

typedef enum ConnectionType
{
    UNDEFINED = 0,
    USB = 1,
    BT = 2
} ConnectionType;

typedef struct DeviceInfo
{
    char* path;
    ConnectionType type;
    
} DeviceInfo;

int enum_devices(DeviceInfo* infos, int max_infos_num)
{
    HANDLE hidDiHandle = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    
    return SUCCESS;
}

int main()
{
    DeviceInfo infos[MAX_CONTROLLER_NUM];
    if(enum_devices(infos, MAX_CONTROLLER_NUM) == SUCCESS)
        printf("we're all good\n");
    printf("Hello world!\n");
    return 0;
}