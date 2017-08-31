/*
 * Vulkan Compute Program
 * 
 * Created by Patrick Gantner
 *
 */

#include "VulkanCompute.h"

#include <fstream>

int main()
{
	auto app = VulkanComputeApplication();
	app.init();
	app.run();

	auto result = app.getResult();
	std::fstream out;
	out.open("result.txt", std::fstream::out);
	if (out.is_open()) {
		for (const auto &f : result) {
			out << f << std::endl;
		}
		out.close();
	}
    return 0;
}
