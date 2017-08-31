# Small Example on how to use Vulkan Compute Shaders  

Notice: This is just a basic Implementation of a simple Compute Shader with Vulkan and GLSL I created when first working with Compute Shaders in Vulkan.

### Building  
Build tested on Windows 10 with Visual Studio 2017 and [LunarG SDK 1.0.57.0](https://vulkan.lunarg.com/sdk/home)  

Open the Solution and Build x64 Debug or Release Configuration. 32-Bit builds are neither tested nor supported.

### Shaders  
You can edit the Shader if you wish. It is located in shaders/kernel.comp  
Make sure to run glslangValidator.exe -V kernel.comp after your changes!

