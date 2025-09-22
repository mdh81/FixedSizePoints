#include "ShaderLoader.h"
#include <fstream>
#include <iostream>
#include <webgpu/webgpu.hpp>

ShaderLoader::ShaderLoader(std::filesystem::path const &shaderFile) {
    if (std::ifstream ifs(shaderFile, std::ios::binary); ifs) {
        ifs.seekg(0U, std::ios::end);
        auto const numBytes = ifs.tellg();
        ifs.seekg(0U, std::ios::beg);
        source.resize(numBytes);
        ifs.read(source.data(), numBytes);
    } else {
        std::println(std::cerr, "Failed to open file {}", shaderFile.string());
    }

    shaderFileName = shaderFile.string();
}

std::string const &ShaderLoader::getSource() {
    return source;
}

wgpu::ShaderModule &ShaderLoader::getShaderModule(wgpu::Device &device) {
    if (!shaderModule) {
        wgpu::ShaderModuleDescriptor shaderDesc;
        shaderDesc.label = std::format("Shader generated from {}", shaderFileName).c_str();
        wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
        shaderCodeDesc.chain.next = nullptr;
        shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
        shaderCodeDesc.code = getSource().c_str();
        shaderDesc.nextInChain = &shaderCodeDesc.chain;
        shaderModule = std::make_optional(device.createShaderModule(shaderDesc));
    }
    return *shaderModule;
}
