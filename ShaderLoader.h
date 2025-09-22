#pragma once

#include <string>
#include <filesystem>
#include "webgpu/webgpu.hpp"
#include <optional>

class ShaderLoader {

  public:
    ShaderLoader(std::filesystem::path const& shaderFile); // NOLINT: Conversion from const char to path intended
    [[nodiscard]] std::string const& getSource(); // TODO: Make me const
    [[nodiscard]] wgpu::ShaderModule& getShaderModule(wgpu::Device&);

    private:
      std::string shaderFileName;
      std::string source;
      std::optional<wgpu::ShaderModule> shaderModule;
};
