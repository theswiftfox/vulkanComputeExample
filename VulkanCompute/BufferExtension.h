#ifndef BUFFER_EXTENSION_H
#define BUFFER_EXTENSION_H
#include <vulkan/vulkan.hpp>

namespace vkExt {

	struct Buffer {
		vk::Device device;
		vk::Buffer buffer;
		vk::DeviceMemory memory;
		vk::DescriptorBufferInfo descriptor;
		void* mapped;
		bool isMapped = false;

		// Flags
		vk::BufferUsageFlags usageFlags;

		vk::Result map() {
			auto res = device.mapMemory(memory, descriptor.offset, descriptor.range, vk::MemoryMapFlags(), &mapped);
			if (res == vk::Result::eSuccess) isMapped = true;
			return res;
		}

		void unmap() {
			if (isMapped) {
				device.unmapMemory(memory);
				mapped = nullptr;
				isMapped = false;
			}
		}

		void bind(vk::DeviceSize offset = 0) {
			device.bindBufferMemory(buffer, memory, offset);
		}

		void setupDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
			descriptor.offset = offset;
			descriptor.buffer = buffer;
			descriptor.range = size;
		}

		void copyTo(void* data, vk::DeviceSize size) {
			assert(mapped);
			memcpy(mapped, data, (size_t)size);
		}

		vk::Result flush() {
			vk::MappedMemoryRange mappedRange = vk::MappedMemoryRange()
				.setMemory(memory)
				.setOffset(descriptor.offset)
				.setSize(descriptor.range);
			return device.flushMappedMemoryRanges(1, &mappedRange);
		}

		vk::Result invalidate() {
			vk::MappedMemoryRange mappedRange = vk::MappedMemoryRange()
				.setMemory(memory)
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
				device.freeMemory(memory);
			}
		}
	};
}

#endif