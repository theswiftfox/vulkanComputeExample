#ifndef BUFFER_EXTENSION_H
#define BUFFER_EXTENSION_H
#include <vulkan/vulkan.hpp>

namespace vkExt {
	struct SharedMemory {
		vk::DeviceMemory memory;
		vk::DeviceSize size;
		void* mapped;
		bool isMapped = false;
		bool isAlive = true;

		vk::Result map(vk::Device device, vk::DeviceSize offset, vk::MemoryMapFlags flags) {
			if (isMapped) return vk::Result::eSuccess;
			auto res = device.mapMemory(memory, offset, size, flags, &mapped);
			if (res == vk::Result::eSuccess) isMapped = true;
			return res;
		}
		void unmap(vk::Device device) {
			if (!isMapped) return;
			device.unmapMemory(memory);
			mapped = nullptr;
			isMapped = false;
		}
		void free(vk::Device device) {
			if (!isAlive) return;
			isAlive = false;
			device.freeMemory(memory);
		}
	};

	struct Buffer {
		vk::Device device;
		vk::Buffer buffer;
		vk::DescriptorBufferInfo descriptor;
		uint32_t memoryOffset;
		vkExt::SharedMemory* memory;
		
		void* mapped() {
			if (!memory->isMapped) {
				auto res = map();
				if (res != vk::Result::eSuccess) return nullptr;
			}
			return (void*)((uint32_t *)(memory->mapped) + memoryOffset);
		}

		// Flags
		vk::BufferUsageFlags usageFlags;

		vk::Result map() {
			return memory->map(device, descriptor.offset, vk::MemoryMapFlags());
		}

		void unmap() {
			memory->unmap(device);
		}

		void bind(vk::DeviceSize offset = 0) {
			device.bindBufferMemory(buffer, memory->memory, offset);
		}

		void setupDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
			descriptor.offset = offset;
			descriptor.buffer = buffer;
			descriptor.range = size;
		}

		void copyTo(void* data, vk::DeviceSize size) {
			assert(memory->mapped);
			memcpy(memory->mapped, data, (size_t)size);
		}

		vk::Result flush() {
			vk::MappedMemoryRange mappedRange = vk::MappedMemoryRange()
				.setMemory(memory->memory)
				.setOffset(descriptor.offset)
				.setSize(descriptor.range);
			return device.flushMappedMemoryRanges(1, &mappedRange);
		}

		vk::Result invalidate() {
			vk::MappedMemoryRange mappedRange = vk::MappedMemoryRange()
				.setMemory(memory->memory)
				.setOffset(descriptor.offset)
				.setSize(descriptor.range);
			return device.invalidateMappedMemoryRanges(1, &mappedRange);
		}

		void destroy(bool freeMem = false) {
			unmap();
			if (buffer) {
				device.destroyBuffer(buffer);
			}
			if (freeMem) {
				memory->free(device);
			}
		}
	};
}

#endif