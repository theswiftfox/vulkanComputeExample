#ifndef VULKAN_COMPUTE_H
#define VULKAN_COMPUTE_H

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "BufferExtension.h"
#include "Helpers.h"

std::vector<const char*> getRequiredExtensions();
std::vector<char> readFile(const std::string& filename);
vk::ResultValue<uint32_t> findQueueFamilyIndex(vk::PhysicalDevice device);
bool checkValidationLayerSupport(const std::vector<const char*> &validationLayers);
vk::ShaderModule createShaderModule(const vk::Device &device, const std::vector<char>& code);
vk::ShaderModule createShaderModuleFromFile(const vk::Device &device, const std::string &file);
bool isDeviceSuitable(vk::PhysicalDevice device, std::vector<std::string> requiredExtensions);
bool checkDeviceExtensionSupport(vk::PhysicalDevice device, std::vector<std::string> requiredExtensions);
vk::ResultValue<uint32_t> findMemoryTypeIndex(vk::PhysicalDevice device, vk::DeviceSize size, vk::MemoryPropertyFlags flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);


class VulkanComputeApplication {
public:
	vk::Result init() {
		vk::Result res = vk::Result::eSuccess;
		res = initInstance();
		if (res != vk::Result::eSuccess) return res;
		res = createDevice();
		if (res != vk::Result::eSuccess) return res;
		res = createBuffers();
		if (res != vk::Result::eSuccess) return res;
		res = createPipeline();
		if (res != vk::Result::eSuccess) return res;
		res = createCommandBuffer();
		if (res == vk::Result::eSuccess) m_initialized = true;
		return res;
	}

	void run() {
		if (!m_initialized) {
			TRACE_FULL("VulkanComputeApplication not fully initialized. aborting.");
			return;
		}
		fillInputBuffersRandom();
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&m_commandBuffer);
		m_queue.submit(submitInfo, nullptr);
		m_queue.waitIdle();
	}

	std::vector<float> getResult();

	~VulkanComputeApplication(){
		cleanup();
	}

private:
	/* Members */	
	bool m_initialized;
	std::vector<const char*> m_validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	vk::Instance m_instance;
	vk::PhysicalDevice m_physicalDevice;
	vk::Device m_device;

	uint32_t m_queueFamIndex;
	vk::Queue m_queue;

	vk::Pipeline m_pipeline;
	vk::PipelineLayout m_pipelineLayout;

	vk::CommandPool m_commandPool;
	vk::CommandBuffer m_commandBuffer;

	vk::DescriptorPool m_descriptorPool;

	vk::DescriptorSet m_descriptorSet;
	vk::DescriptorSetLayout m_descriptorSetLayout;

	std::vector<std::string> m_requiredExtensions;

	vk::DeviceMemory m_valuesBufferMemory;
	vkExt::Buffer m_inputBufferA;
	vkExt::Buffer m_inputBufferB;
	vkExt::Buffer m_outputBuffer;
	uint32_t m_numElements = 1024*1024;
	uint32_t m_bufferSize = sizeof(float) * m_numElements;
	uint32_t m_memorySize = m_bufferSize * 3;
	uint32_t m_outputOffset = m_bufferSize * 2;
	uint32_t m_numElemsPushConstantSize = sizeof(glm::vec4);

	/* functions */
	vk::Result initInstance();
	vk::Result createDevice();
	vk::Result createBuffers();
	vk::Result createPipeline();
	vk::Result createCommandBuffer();

	void fillInputBuffersRandom();

	void cleanup();
};

#endif