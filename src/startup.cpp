#include "startup.h"

uint32_t extensions_count = 0;
uint32_t device_extensions_count = 0;
uint32_t device_count = 0;
uint32_t desired_count = 0; 
uint32_t queue_families_count = 0;
const char** desired_extensions = nullptr;

VkExtensionProperties* available_extensions = nullptr;
VkPhysicalDevice *available_devices = nullptr;
VkQueueFamilyProperties *queue_families = nullptr;
VkDeviceQueueCreateInfo *queue_create_infos = nullptr;

VkInstance instance;
VkPhysicalDevice target_device;
VkDevice logical_device;
VkPhysicalDeviceFeatures device_features;
VkPhysicalDeviceProperties device_properties;

VkInstanceCreateInfo instance_create_info;
VkApplicationInfo app_info = 
{
	VK_STRUCTURE_TYPE_APPLICATION_INFO,
	nullptr,
	"Vether",
	VK_MAKE_VERSION(1,0,0),
	"Vether",
	VK_MAKE_VERSION(1,0,0),
	VK_MAKE_VERSION(1,0,0)
};

#if defined _WIN32
HMODULE vulkan_lib;
#elif defined __linux
void* vulkan_lib
#endif

//---------------------------------------------------------------------

void debug_pause()
{
	std::cout<<"Press any key to continue... \n";
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	std::cin.get();
	exit(1);
}

bool LoadVulkan()
{
	#if defined _WIN32
	#define libtype HMODULE
	vulkan_lib = LoadLibrary("vulkan-1.dll");
	#define LoadFunction GetProcAddress
	#elif defined __linux
	#define libtype void*
	vulkan_lib = dlopen("libvulcan.so.1", RTLD_NOW);
	#define LoadFunction dlsym
	#endif
	if(vulkan_lib == nullptr)
	{
		std::cout<<"Failed to load the Vulkan Runtime Library! \n";
		return false;
	}
			
	#define EXPORTED_VULKAN_FUNCTION( name ) \
	name = (PFN_##name) LoadFunction(vulkan_lib, #name); \
	if(name == nullptr) \
	{ \
		std::cout<<"Could not load exported Vulkan function: "<< #name <<std::endl; \
		return false; \
	}else \
		std::cout<<"Exported Vulkan function: "<< #name <<std::endl; \
	
	#include "vulkan_functions_list.inl"
	
	return true;
}

bool LoadVulkanGlobalFuncs()
{
	#define GLOBAL_LEVEL_VULKAN_FUNCTION( name ) \
	name = (PFN_##name)vkGetInstanceProcAddr( nullptr, #name); \
	if(name == nullptr) \
	{ \
		std::cout<<"Could not load exported Vulkan function: "<< #name <<std::endl; \
		return false; \
	}else \
		std::cout<<"Loaded exported Vulkan function: "<< #name <<std::endl; \
	
	#include "vulkan_functions_list.inl"
	return true; 
}

//Instance functions take 1st parameter of type Vkdevice || VkQueue || VkCommandBuffer
bool LoadInstanceFunctions()
{
	#define INSTANCE_LEVEL_VULKAN_FUNCTION( name ) \
	name = (PFN_##name)vkGetInstanceProcAddr( instance, #name); \
	if(name == nullptr) \
	{ \
		std::cout<<"Could not load instance-level Vulkan function: "<< #name <<std::endl; \
		return false; \
	}else \
		std::cout<<"Loaded instance-level Vulkan function: "<< #name <<std::endl; \
	
	#include "vulkan_functions_list.inl"
	
    // Load instance-level functions from enabled extensions
	#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension )        \
    for(uint32_t i = 0; i<desired_count; i++) {                      \
      if( std::string( desired_extensions[i] ) == std::string( extension ) ) {      \
        name = (PFN_##name)vkGetInstanceProcAddr( instance, #name );            \
        if( name == nullptr ) {                                                 \
          std::cout << "Could not load instance-level Vulkan function named: "  \
            #name << std::endl;                                                 \
          return false;                                                         \
        }else                                                                       \
		std::cout<<"Loaded function from extension: "<<#name<<std::endl; \
      }                                                                         \
    }	
	
	#include "vulkan_functions_list.inl"
	
	return true;
}

bool CheckInstanceExtensions()
{
	VkResult result = VK_SUCCESS;
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);
	if(result != VK_SUCCESS || extensions_count == 0)
	{
		std::cout<<"Could not get the extension count!  \n";
		return false; 		
	}
	available_extensions = new VkExtensionProperties [extensions_count];
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, &available_extensions[0]);
	if(result != VK_SUCCESS || extensions_count == 0)
	{
		std::cout<<"Could not enumerate extensions!  \n";
		return false; 		
	}	
	return true;
}

bool IsExtensionSupported(const char* extension) 
{
	for(uint32_t i = 0; i<extensions_count; i++)
	{
		if( strstr( (char*) &available_extensions[i], extension ) ) 
		{
			return true;
		}		
	}
	std::cout<<"Available Extensions: \n";
	for(uint32_t i = 0; i<extensions_count; i++)
	{
		std::cout<<(char*) &available_extensions[i]<<std::endl; 
	}	
    return false;
}

bool CreateVulkanInstance(uint32_t count, const char** exts)
{
	desired_count = count;
	desired_extensions = exts;
	for(uint32_t i = 0; i < count; i++)
	{
		if(desired_extensions[i] != nullptr)
		{
			if(!IsExtensionSupported(desired_extensions[i]))
			{
				std::cout<<"Extension \n"<<desired_extensions[i]<<" is not supported!"<<std::endl;
				return false;
			}
		}
	}
	
	instance_create_info = 
	{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		nullptr,
		0,
		&app_info,
		0,
		nullptr,
		count,
		&desired_extensions[0]
	};
	VkResult result = VK_SUCCESS;
	result = vkCreateInstance(&instance_create_info, nullptr, &instance);
	if(result != VK_SUCCESS)
	{
		std::cout<<"Could not create Vulkan Instance!  \n";
		return false; 		
	}	
	
	//instance is created and we wont dont need to store instance extensions here anymore. 
	//next step will be to load device extensions in this array. 
	delete available_extensions;
	available_extensions = nullptr;
	
	return true;
}

bool CheckPhysicalDevices()
{
	VkResult result = VK_SUCCESS;
	result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if(result != VK_SUCCESS || device_count == 0)
	{
		std::cout<<"Could not get number of physical devices!  \n";
		return false; 		
	}	
	available_devices = new VkPhysicalDevice [device_count];
	result = vkEnumeratePhysicalDevices(instance, &device_count, &available_devices[0]);
	if(result != VK_SUCCESS || device_count == 0)
	{
		std::cout<<"Could not enumerate physical devices!  \n";
		return false; 		
	}		
	return true;
}

bool CheckPhysicalDeviceExtensions()
{
	VkResult result = VK_SUCCESS;
	for(uint32_t i = 0; i<device_count; i++)
	{
		result = vkEnumerateDeviceExtensionProperties(available_devices[i], nullptr, &device_extensions_count, nullptr);
		if(result != VK_SUCCESS || device_extensions_count == 0)
		{
			std::cout<<"Could not get number of physical device extensions!  \n";
			return false; 		
		}
		available_extensions = new VkExtensionProperties [device_extensions_count];
		result = vkEnumerateDeviceExtensionProperties(available_devices[i], nullptr, &device_extensions_count, &available_extensions[0]);
		if(result != VK_SUCCESS || device_extensions_count == 0)
		{
			std::cout<<"Could not enumerate device extensions!  \n";
			return false; 		
		}	
		
		vkGetPhysicalDeviceFeatures(available_devices[i], &device_features);
		vkGetPhysicalDeviceProperties(available_devices[i], &device_properties);
		
		//can add more checks later.
		if( !device_features.geometryShader ) 
		{
			continue;
		} else {
			device_features = {};
			device_features.geometryShader = VK_TRUE;
			target_device = available_devices[i];
			break;
		}
	}
	return true;
}

bool CheckQueueProperties(VkQueueFlags desired_capabilities,  uint32_t &queue_family_index )
{
	vkGetPhysicalDeviceQueueFamilyProperties(target_device, &queue_families_count, nullptr);
	if(queue_families_count == 0)
	{
		std::cout<<"Could not get number of family queues!  \n";
		return false;
	}
	VkQueueFamilyProperties queue_families[queue_families_count];
	vkGetPhysicalDeviceQueueFamilyProperties(target_device, &queue_families_count, &queue_families[0]);
	if(queue_families_count == 0)
	{
		std::cout<<"Could not get properties of family queues!  \n";
		return false;
	}
	for(uint32_t i = 0; i<queue_families_count; ++i)
	{
		if(queue_families[i].queueCount > 0 && (queue_families[i].queueFlags & desired_capabilities) == desired_capabilities)
		{
			queue_family_index = i;
			return true;
		}
	}
	std::cout<<"Some capabilities were of family queues not detected!  \n";
	return false;
}

bool SetQueue(QueueInfo *array, uint32_t family, float *_Priorities, int index)
{
	array[index].FamilyIndex = family;
	array[index].Priorities = _Priorities;
	return true;
}

bool CreateLogicalDevice(QueueInfo *array, int number_of_queues, uint32_t ext_count, const char** exts)
{
	desired_count = ext_count;
	desired_extensions = exts;
	for(uint32_t i = 0; i < ext_count; i++)
	{
		if(desired_extensions[i] != nullptr)
		{
			if(!IsExtensionSupported(desired_extensions[i]))
			{
				std::cout<<"Extension \n"<<desired_extensions[i]<<" is not supported by physical device!"<<std::endl;
				return false;
			}
		}
	}	
	VkDeviceQueueCreateInfo queue_create_infos[number_of_queues];
	for(int i = 0; i<number_of_queues; i++)
	{
		if(array[i].FamilyIndex == array[i+1].FamilyIndex)
		{
			number_of_queues = number_of_queues - 1;
		}
		queue_create_infos[i] = 
		{
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr,
			0,
			array[i].FamilyIndex,
			(uint32_t) sizeof(array[i].Priorities)/sizeof(array[i].Priorities[0]),
			array[i].Priorities
		};
	}
	
    VkDeviceCreateInfo device_create_info = 
	{
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,               // VkStructureType                  sType
      nullptr,                                            // const void                     * pNext
      0,                                                  // VkDeviceCreateFlags              flags
      (uint32_t) number_of_queues,                        // uint32_t                         queueCreateInfoCount
      queue_create_infos,                                 // const VkDeviceQueueCreateInfo  * pQueueCreateInfos
      0,                                                  // uint32_t                         enabledLayerCount
      nullptr,                                            // const char * const             * ppEnabledLayerNames
      ext_count,                                          // uint32_t                         enabledExtensionCount
      &desired_extensions[0],                             // const char * const             * ppEnabledExtensionNames
      &device_features                                    // const VkPhysicalDeviceFeatures * pEnabledFeatures
    };	
	
	VkResult result = vkCreateDevice(target_device, &device_create_info, nullptr, &logical_device);
    if(result != VK_SUCCESS || logical_device == VK_NULL_HANDLE) 
	{
      std::cout << "Could not create logical device." << std::endl;
      return false;
    }
	
	//delete available_extensions;
	
	return true;
}

bool LoadDeviceLevelFunctions()
{
    // Load core Vulkan API device-level functions
	#define DEVICE_LEVEL_VULKAN_FUNCTION( name )                                    \
    name = (PFN_##name)vkGetDeviceProcAddr( logical_device, #name );            \
    if( name == nullptr ) {                                                     \
      std::cout << "Could not load device-level Vulkan function named: "        \
        #name << std::endl;                                                     \
      return false;                                                             \
    }else                                                                       \
		std::cout<<"Loaded device-level function: "<<#name<<std::endl; 
	
	#include "vulkan_functions_list.inl"
	
    // Load device-level functions from enabled extensions
	#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension )          \
    for(uint32_t i = 0; i<desired_count; i++) {                      \
      if( std::string( desired_extensions[i] ) == std::string( extension ) ) {      \
        name = (PFN_##name)vkGetDeviceProcAddr( logical_device, #name );        \
        if( name == nullptr ) {                                                 \
          std::cout << "Could not load device-level Vulkan function from extension: "    \
            #name << std::endl;                                                 \
          return false;                                                         \
        }else                                                                       \
		std::cout<<"Loaded device-level function from extension: "<<#name<<std::endl;                                                                       \
      }                                                                         \
    }
	#include "vulkan_functions_list.inl"
    return true;	
}

void ReleaseVulkanLoaderLibrary()
{
	vkDestroyDevice( logical_device, nullptr );
	vkDestroyInstance( instance, nullptr );
    instance = VK_NULL_HANDLE;
	logical_device = VK_NULL_HANDLE;	
	
	if( nullptr != vulkan_lib ) 
	{
		#if defined _WIN32
		  FreeLibrary( vulkan_lib );
		#elif defined __linux
		  dlclose( vulkan_lib );
		#endif
		  vulkan_lib = nullptr;	
	}
}
