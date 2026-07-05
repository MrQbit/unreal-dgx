#include <vulkan/vulkan.h>
#include <stdio.h>
#include <string.h>
int main(){
    VkApplicationInfo app={0}; app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName="vktest"; app.apiVersion=VK_API_VERSION_1_2;
    VkInstanceCreateInfo ci={0}; ci.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; ci.pApplicationInfo=&app;
    VkInstance inst;
    VkResult r=vkCreateInstance(&ci,0,&inst);
    if(r!=VK_SUCCESS){ printf("vkCreateInstance FAILED: %d\n", r); return 1; }
    printf("vkCreateInstance OK\n");
    uint32_t n=0; vkEnumeratePhysicalDevices(inst,&n,0);
    printf("Physical devices: %u\n", n);
    if(n==0){ printf("NO VULKAN DEVICES\n"); return 2; }
    VkPhysicalDevice devs[8]; if(n>8)n=8; vkEnumeratePhysicalDevices(inst,&n,devs);
    for(uint32_t i=0;i<n;i++){
        VkPhysicalDeviceProperties p; vkGetPhysicalDeviceProperties(devs[i],&p);
        const char* t = p.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU?"DISCRETE":
                        p.deviceType==VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU?"INTEGRATED":"OTHER";
        printf("  [%u] %s | Vulkan %u.%u.%u | type=%s | driverVer=%u\n", i, p.deviceName,
            VK_VERSION_MAJOR(p.apiVersion),VK_VERSION_MINOR(p.apiVersion),VK_VERSION_PATCH(p.apiVersion), t, p.driverVersion);
        /* check queue families / compute+graphics */
        uint32_t qn=0; vkGetPhysicalDeviceQueueFamilyProperties(devs[i],&qn,0);
        VkQueueFamilyProperties qf[16]; if(qn>16)qn=16; vkGetPhysicalDeviceQueueFamilyProperties(devs[i],&qn,qf);
        for(uint32_t q=0;q<qn;q++) if(qf[q].queueFlags & VK_QUEUE_GRAPHICS_BIT){ printf("      -> has GRAPHICS queue family (%u queues)\n", qf[q].queueCount); break; }
    }
    return 0;
}
