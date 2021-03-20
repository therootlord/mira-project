#include "SystemApi.hpp"
#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>

#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

bool SystemApi::ReadProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_Data, uint32_t p_DataLength)
{
    // Open mira device
    auto s_Device = open("/dev/mira", O_RDWR);
    if (s_Device == -1)
        return false;
    
    // Calculate how large the allocation is, it needs to be the header + size of the data
    uint64_t s_AllocationSize = sizeof(MiraReadProcessMemory) + p_DataLength;
    auto s_Allocation = new uint8_t[s_AllocationSize];
    if (s_Allocation == nullptr)
    {
        close(s_Device);
        return false;
    }
    
    // Zero out the entire buffer to prevent garbage from being read back
    memset(s_Allocation, 0, s_AllocationSize);

    // Cast the header and set the needed values
    MiraReadProcessMemory* s_Request = reinterpret_cast<MiraReadProcessMemory*>(s_Allocation);
    s_Request->StructureSize = s_AllocationSize;
    s_Request->ProcessId = p_ProcessId;
    s_Request->Address = p_ProcessAddress;

    // TODO: Verify that the _IOC macro hasn't changed between Linux<->FreeBSD

    // Send the RPM ioctl, if everything works correctly then s_Ret should be 0
    auto s_Ret = ioctl(s_Device, MIRA_READ_PROCESS_MEMORY, s_Request);
    if (s_Ret != 0)
    {
        delete [] s_Allocation;
        close(s_Device);
        return false;
    }

    // Copy out the buffer
    memcpy(p_Data, s_Request->Data, p_DataLength);

    // Free the memory used for the allocation
    delete [] s_Allocation;
    close(s_Device);

    return true;
}

bool SystemApi::WriteProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_Data, uint32_t p_DataLength)
{
    auto s_Device = open("/dev/mira", O_RDWR);
    if (s_Device == -1)
        return false;
    
    uint64_t s_AllocationSize = sizeof(MiraWriteProcessMemory) + p_DataLength;
    uint8_t* s_Allocation = new uint8_t[s_AllocationSize];
    if (s_Allocation == nullptr)
    {
        close(s_Device);
        return false;
    }
    
    memset(s_Allocation, 0, s_AllocationSize);

    auto s_Request = reinterpret_cast<MiraWriteProcessMemory*>(s_Allocation);
    s_Request->StructureSize = s_AllocationSize;
    s_Request->ProcessId = p_ProcessId;
    s_Request->Address = p_ProcessAddress;

    // Copy the data to our request
    memcpy(s_Request->Data, p_Data, p_DataLength);

    auto s_Ret = ioctl(s_Device, MIRA_WRITE_PROCESS_MEMORY, s_Request);
    if (s_Ret != 0)
    {
        delete [] s_Allocation;
        close(s_Device);
        return false;
    }

    delete [] s_Allocation;
    close(s_Device);

    return true;
}