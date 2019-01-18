#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>
#include <tchar.h>
#include <string>

#if defined _WIN32

#include <windows.h>

#endif

#define VK_NO_PROTOTYPES
#include <vulkan.h>

//------------------
#pragma once
#include "vulkan_decl.h"

#define DEBUG

//--------------------

extern VkDevice logical_device;
extern VkInstance instance;

struct QueueInfo
{
	uint32_t FamilyIndex;
	float *Priorities;
};

//--------------------

//-------------------- funcs
void debug_pause();
void ReleaseVulkanLoaderLibrary();
bool LoadVulkan();
bool LoadVulkanGlobalFuncs();
bool LoadInstanceFunctions();
bool LoadDeviceLevelFunctions();
bool CheckInstanceExtensions();
bool CheckPhysicalDeviceExtensions();
bool CheckPhysicalDevices();
bool CheckQueueProperties(VkQueueFlags desired_capabilities,  uint32_t &queue_family_index);
bool IsExtensionSupported(const char* extension);
bool SetQueue(QueueInfo *array, uint32_t family, float *_Priorities, int index);
bool CreateVulkanInstance(uint32_t count, const char** exts); 
bool CreateLogicalDevice(QueueInfo *array, int number_of_queues, uint32_t ext_count, const char** exts);

//--------------------


