#include "VulkanCompute.h"

#include <set>
#include <fstream>

#include <random>

vk::Result VulkanComputeApplication::initInstance() {
	// Use validation layers if this is a debug build
	std::vector<const char*> layers;
#if defined(_DEBUG)
	if (!checkValidationLayerSupport(m_validationLayers)) {
		TRACE_FULL("validation layers requested, but not present!");
		return vk::Result::eErrorInitializationFailed;
	}
	layers = m_validationLayers;
#endif
	auto extensions = getRequiredExtensions();

	// VkApplicationInfo allows the programmer to specifiy some basic information about the
	// program, which can be useful for layers and tools to provide more debug information.
	vk::ApplicationInfo appInfo = vk::ApplicationInfo()
		.setPApplicationName("Vulkan C++ Program Template")
		.setApplicationVersion(1)
		.setPEngineName("LunarG SDK")
		.setEngineVersion(1)
		.setApiVersion(VK_API_VERSION_1_0);

	// VkInstanceCreateInfo is where the programmer specifies the layers and/or extensions that
	// are needed. For now, none are enabled.
	vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
		.setFlags(vk::InstanceCreateFlags())
		.setPApplicationInfo(&appInfo)
		.setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
		.setPpEnabledExtensionNames(extensions.data())
		.setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
		.setPpEnabledLayerNames(layers.data());

	// Create the Vulkan instance.
	vk::Instance instance;
	try {
		m_instance = vk::createInstance(instInfo);
	}
	catch (const std::exception& e) {
		TRACE_FULL(e.what());
		return vk::Result::eErrorInitializationFailed;
	}
	return vk::Result::eSuccess;
}

void VulkanComputeApplication::cleanup() {
	if (!m_initialized) return; // todo: maybe check each component if it is initialized
	m_device.freeCommandBuffers(m_commandPool, 1, &m_commandBuffer);
	m_device.destroyCommandPool(m_commandPool);
	m_device.destroyDescriptorPool(m_descriptorPool);
	m_device.destroyPipeline(m_pipeline);
	m_device.destroyPipelineLayout(m_pipelineLayout);
	m_device.destroyDescriptorSetLayout(m_descriptorSetLayout);
	m_inputBufferA.destroy(false);
	m_inputBufferB.destroy(false);
	m_outputBuffer.destroy(true);
	m_queue = nullptr;
	m_device.destroy();
	m_instance.destroy();
}

vk::Result VulkanComputeApplication::createDevice() {
	std::vector<vk::PhysicalDevice> devices = m_instance.enumeratePhysicalDevices();
	for (const auto& device : devices) {
		if (isDeviceSuitable(device, m_requiredExtensions)) {
			m_physicalDevice = device;
			break;
		}
	}
	if (!m_physicalDevice) {
		TRACE_FULL("unable to find suitable device");
		return vk::Result::eErrorInitializationFailed;
	}
	m_queueFamIndex = findQueueFamilyIndex(m_physicalDevice).value; // we can be sure that the function reports a correct value as we already check this in isDeviceSuitable

	const float defaultQueuePriority = 1.0f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(m_queueFamIndex)
		.setQueueCount(1)
		.setPQueuePriorities(&defaultQueuePriority);

	vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount(1)
		.setPQueueCreateInfos(&deviceQueueCreateInfo);

	m_device = m_physicalDevice.createDevice(deviceCreateInfo);
	if (!m_device) {
		TRACE_FULL("unable to create logical device");
		return vk::Result::eErrorInitializationFailed;
	}
	m_queue = m_device.getQueue(m_queueFamIndex, 0);
	if (!m_device) {
		TRACE_FULL("unable to create device queue");
		return vk::Result::eErrorInitializationFailed;
	}
	return vk::Result::eSuccess;
}

vk::Result VulkanComputeApplication::createBuffers() {
	auto res = findMemoryTypeIndex(m_physicalDevice, m_memorySize);
	if (res.result != vk::Result::eSuccess) {
		TRACE_FULL("unable to find memory type for values buffers");
		return vk::Result::eErrorInitializationFailed;
	}
	vk::MemoryAllocateInfo memAllocInfo = vk::MemoryAllocateInfo()
		.setAllocationSize(m_memorySize)
		.setMemoryTypeIndex(res.value);

	m_valuesBufferMemory = m_device.allocateMemory(memAllocInfo);
	if (!m_valuesBufferMemory) {
		TRACE_FULL("unable to allocate memory for buffers");
		return vk::Result::eErrorInitializationFailed;
	}

	vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
		.setSize(m_bufferSize)
		.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setQueueFamilyIndexCount(1)
		.setPQueueFamilyIndices(&m_queueFamIndex);

	m_inputBufferA.buffer = m_device.createBuffer(bufferCreateInfo);
	if (!m_inputBufferA.buffer) {
		TRACE_FULL("unable to create input buffer A");
		return vk::Result::eErrorInitializationFailed;
	}
	m_device.bindBufferMemory(m_inputBufferA.buffer, m_valuesBufferMemory, 0);
	m_inputBufferA.device = m_device;
	m_inputBufferA.memory = m_valuesBufferMemory;
	m_inputBufferA.usageFlags = bufferCreateInfo.usage;
	m_inputBufferA.setupDescriptor(m_bufferSize);

	m_inputBufferB.buffer = m_device.createBuffer(bufferCreateInfo);
	if (!m_inputBufferB.buffer) {
		TRACE_FULL("unable to create input buffer B");
		return vk::Result::eErrorInitializationFailed;
	}
	m_device.bindBufferMemory(m_inputBufferB.buffer, m_valuesBufferMemory, m_bufferSize);
	m_inputBufferB.device = m_device;
	m_inputBufferB.memory = m_valuesBufferMemory;
	m_inputBufferB.usageFlags = bufferCreateInfo.usage;
	m_inputBufferB.setupDescriptor(m_bufferSize);

	m_outputBuffer.buffer = m_device.createBuffer(bufferCreateInfo);
	if (!m_outputBuffer.buffer) {
		TRACE_FULL("unable to create output buffer");
		return vk::Result::eErrorInitializationFailed;
	}
	m_device.bindBufferMemory(m_outputBuffer.buffer, m_valuesBufferMemory, m_outputOffset);
	m_outputBuffer.device = m_device;
	m_outputBuffer.memory = m_valuesBufferMemory;
	m_outputBuffer.usageFlags = bufferCreateInfo.usage;
	m_outputBuffer.setupDescriptor(m_bufferSize);

	return vk::Result::eSuccess;
}

vk::Result VulkanComputeApplication::createPipeline() {
	vk::ShaderModule shader_module;
	try {
		shader_module = createShaderModuleFromFile(m_device, "shaders/comp.spv");
	}
	catch (const std::runtime_error& e) {
		TRACE_FULL(e.what());
		return vk::Result::eIncomplete;
	}

	vk::DescriptorSetLayoutBinding layoutBindings[3] = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
	};

	vk::DescriptorSetLayoutCreateInfo descLayoutCI = vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(3)
		.setPBindings(layoutBindings);

	m_descriptorSetLayout = m_device.createDescriptorSetLayout(descLayoutCI);
	if (!m_descriptorSetLayout) {
		TRACE_FULL("unable to create compute descriptor set layout");
		return vk::Result::eErrorInitializationFailed;
	}

	vk::PushConstantRange pushConstantRange = vk::PushConstantRange()
		.setStageFlags(vk::ShaderStageFlagBits::eCompute)
		.setSize(m_numElemsPushConstantSize)
		.setOffset(0);

	vk::PipelineLayoutCreateInfo pipeLayoutCI = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(1)
		.setPSetLayouts(&m_descriptorSetLayout)
		.setPushConstantRangeCount(1)
		.setPPushConstantRanges(&pushConstantRange);

	m_pipelineLayout = m_device.createPipelineLayout(pipeLayoutCI);
	if (!m_pipelineLayout) {
		TRACE_FULL("unable to create compute pipeline layout");
		return vk::Result::eErrorInitializationFailed;
	}

	vk::ComputePipelineCreateInfo computePipeCI = vk::ComputePipelineCreateInfo()
		.setStage(vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eCompute,
			shader_module,
			"main",
			nullptr
		))
		.setLayout(m_pipelineLayout);
	m_pipeline = m_device.createComputePipeline(nullptr, computePipeCI);
	if (!m_pipeline) {
		TRACE_FULL("unable to create compute pipeline");
		return vk::Result::eErrorInitializationFailed;
	}

	return vk::Result::eSuccess;
}

vk::Result VulkanComputeApplication::createCommandBuffer() {
	vk::CommandPoolCreateInfo comandPoolCI = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(m_queueFamIndex);
	vk::DescriptorPoolSize descriptorPoolSizeStoreBuffs = vk::DescriptorPoolSize()
		.setDescriptorCount(3)
		.setType(vk::DescriptorType::eStorageBuffer);


	vk::DescriptorPoolCreateInfo descriptorPoolCI = vk::DescriptorPoolCreateInfo()
		.setPoolSizeCount(1)
		.setPPoolSizes(&descriptorPoolSizeStoreBuffs)
		.setMaxSets(1);

	m_descriptorPool = m_device.createDescriptorPool(descriptorPoolCI);
	if (!m_descriptorPool) {
		TRACE_FULL("unable to create descriptor pool");
		return vk::Result::eErrorInitializationFailed;
	}

	vk::DescriptorSetAllocateInfo descriptorSetAllocInfo = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(m_descriptorPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&m_descriptorSetLayout);

	m_descriptorSet = m_device.allocateDescriptorSets(descriptorSetAllocInfo).front();
	if (!m_descriptorSet) {
		TRACE_FULL("unable to create descriptor sets");
		return vk::Result::eErrorInitializationFailed;
	}

	vk::WriteDescriptorSet writeDescriptorSets[3] = {
		vk::WriteDescriptorSet(m_descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &m_inputBufferA.descriptor, nullptr),
		vk::WriteDescriptorSet(m_descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &m_inputBufferB.descriptor, nullptr),
		vk::WriteDescriptorSet(m_descriptorSet, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &m_outputBuffer.descriptor, nullptr)
	};

	m_device.updateDescriptorSets(3, writeDescriptorSets, 0, nullptr);

	m_commandPool = m_device.createCommandPool(comandPoolCI);
	if (!m_commandPool) {
		TRACE_FULL("unable to create command pool");
		return vk::Result::eErrorInitializationFailed;
	}

	vk::CommandBufferAllocateInfo commandBufferAllocInfo = vk::CommandBufferAllocateInfo()
		.setCommandPool(m_commandPool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(1);

	m_commandBuffer = m_device.allocateCommandBuffers(commandBufferAllocInfo).front();
	if (!m_commandBuffer) {
		TRACE_FULL("unable to allocate command buffer");
		return vk::Result::eErrorInitializationFailed;
	}

	vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	glm::vec4 numElems(m_numElements, 0, 0, 0);
	
	m_commandBuffer.begin(commandBufferBeginInfo);
	m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);
	m_commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
	m_commandBuffer.pushConstants(m_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, m_numElemsPushConstantSize, &numElems);
	m_commandBuffer.dispatch(m_bufferSize / sizeof(int32_t), 1, 1);
	m_commandBuffer.end();

	return vk::Result::eSuccess;
}

void VulkanComputeApplication::fillInputBuffersRandom() {
	auto res = m_inputBufferA.map();
	if (res != vk::Result::eSuccess) {
		TRACE_FULL("unable to map buffer");
		return;
	}
	res = m_inputBufferB.map();
	if (res != vk::Result::eSuccess) {
		TRACE_FULL("unable to map buffer");
		return;
	}
	float* mappedA = (float *)m_inputBufferA.mapped;
	float* mappedB = (float *)m_inputBufferB.mapped;
	
	std::random_device rand;
	std::mt19937 gen(rand());
	std::uniform_real_distribution<float> distribution(1.0f, 10.0f); //random floats from 1.0 to 10.0)
	for (uint32_t i = 0; i < m_numElements; i++) {
		mappedA[i] = distribution(gen);
		mappedB[i] = distribution(gen);
	}
	m_inputBufferA.unmap();
	m_inputBufferB.unmap();
}

std::vector<float> VulkanComputeApplication::getResult() {
	if (!m_initialized) {
		TRACE_FULL("VulkanComputeApplication not fully initialized. aborting.");
		return std::vector<float>();
	}
	auto res = m_outputBuffer.map();
	if (res != vk::Result::eSuccess) {
		TRACE_FULL("unable to map buffer");
		return std::vector<float>();
	}
	float* mappedOut = (float *)m_outputBuffer.mapped;
	std::vector<float> result(m_numElements);
	for (uint32_t i = 0; i < m_numElements; i++) {
		result[i] = mappedOut[i];
	}
	m_outputBuffer.unmap();
	return result;
}

#pragma region helperfunctions
static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error(ERRORSTRING("failed to open file!"));
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

vk::ShaderModule createShaderModuleFromFile(const vk::Device &device, const std::string &file) {
	return createShaderModule(device, readFile(file));
}

vk::ShaderModule createShaderModule(const vk::Device &device, const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(code.size())
		.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

	return device.createShaderModule(createInfo);
}

vk::ResultValue<uint32_t> findMemoryTypeIndex(vk::PhysicalDevice device, vk::DeviceSize size, vk::MemoryPropertyFlags flags) {
	vk::PhysicalDeviceMemoryProperties props = device.getMemoryProperties();
	for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
		const vk::MemoryType memType = props.memoryTypes[i];
		if (memType.propertyFlags & flags && props.memoryHeaps[memType.heapIndex].size > size) {
			return vk::ResultValue<uint32_t>(vk::Result::eSuccess, i);
		}
	}
	uint32_t ret = 0;
	return vk::ResultValue<uint32_t>(vk::Result::eIncomplete, ret);
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device, std::vector<std::string> requiredExtensions) {
	std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

	std::set<std::string> setRequiredExtensions(requiredExtensions.begin(), requiredExtensions.end());
	for (const auto& ext : availableExtensions) {
		setRequiredExtensions.erase(ext.extensionName);
	}
	return setRequiredExtensions.empty();
}

vk::ResultValue<uint32_t> findQueueFamilyIndex(vk::PhysicalDevice device) {
	uint32_t index = 0;
	vk::Result res = vk::Result::eIncomplete;
	std::vector<vk::QueueFamilyProperties> props = device.getQueueFamilyProperties();

	uint32_t i = 0;
	for (const auto& queueFamily : props) {
		if (queueFamily.queueCount > 0 && 
			queueFamily.queueFlags & vk::QueueFlagBits::eCompute /*&& 
			!((queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) || (queueFamily.queueFlags & vk::QueueFlagBits::eSparseBinding))*/) {
			res = vk::Result::eSuccess;
			index = i;
			break;
		}
		i++;
	}
	
	return vk::ResultValue<uint32_t>(res, index);
}

bool isDeviceSuitable(vk::PhysicalDevice device, std::vector<std::string> requiredExtensions) {
	auto res = findQueueFamilyIndex(device);
	if (res.result != vk::Result::eSuccess) {
		return false;
	}
	return checkDeviceExtensionSupport(device, requiredExtensions);
}

std::vector<const char*> getRequiredExtensions() {
	std::vector<const char*> extensions;
#ifdef _DEBUG
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
	return extensions;
}

bool checkValidationLayerSupport(const std::vector<const char*> &validationLayers) {
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}
#pragma endregion endhelperfunctions