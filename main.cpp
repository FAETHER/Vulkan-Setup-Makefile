#include "src/startup.h"

#define number_of_queues 2 // <- change this if more queues needed

int main(int argc, char *lpCmdLine[])
{
	static const char *extensions[] = {};
	static const char *device_extensions[] = {}; 
	uint32_t graphics_queue_family_index; // <- this is queue 1
	uint32_t compute_queue_family_index; // <- this is queue 2
	VkQueue GraphicsQueue;
	VkQueue ComputeQueue;
	struct QueueInfo QueueInfos[number_of_queues];
	float priority[] = {1.0f};
	
	
	if (!LoadVulkan() || 
		!LoadVulkanGlobalFuncs() || 
		!CheckInstanceExtensions() ||
		!CreateVulkanInstance(0,extensions)||
		!LoadInstanceFunctions() ||
		!CheckPhysicalDevices() ||
		!CheckPhysicalDeviceExtensions()||
		!CheckQueueProperties(VK_QUEUE_GRAPHICS_BIT, graphics_queue_family_index )||
		!CheckQueueProperties(VK_QUEUE_COMPUTE_BIT, compute_queue_family_index)||
		!SetQueue(QueueInfos, graphics_queue_family_index, priority, 0)||
		!SetQueue(QueueInfos, compute_queue_family_index, priority, 1)||
		!CreateLogicalDevice(QueueInfos, number_of_queues, 0, device_extensions)||
		!LoadDeviceLevelFunctions())
	{
		debug_pause();
		exit(1);
	}
	
	vkGetDeviceQueue(logical_device, graphics_queue_family_index, 0, &GraphicsQueue);
	vkGetDeviceQueue(logical_device, compute_queue_family_index, 0, &ComputeQueue);
	
	std::cout << "Vulkan Initialized Successfully!" << std::endl;
	
	ReleaseVulkanLoaderLibrary();
	
	debug_pause();
}
