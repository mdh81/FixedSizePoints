#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <iostream>
#include <print>
#include <cassert>
#include <vector>

#include "ShaderLoader.h"
#include <random>

using namespace wgpu;

class Application {
public:
	void Initialize();

	void Terminate();

	void MainLoop();

	bool IsRunning() const;

private:
	TextureView GetNextSurfaceTextureView();

	void drawPointCloud(CommandEncoder&, RenderPassDescriptor const&);

	void setupPointCloudPipeline();

	void generatePointCloud();

	GLFWwindow *window{nullptr};
	Device device;
	Queue queue;
	Surface surface;
	std::unique_ptr<ErrorCallback> uncapturedErrorCallbackHandle;
	TextureFormat surfaceFormat{TextureFormat::Undefined};

	std::optional<RenderPipelineDescriptor> pointCloudRenderPipelineDesc;
	std::optional<RenderPipeline> pointCloudPipeline;

	std::vector<float> pointData;
	Buffer vertexBuffer;

	std::array<VertexAttribute, 2> vertexAttributes;
};

int main() {
	Application app;

	app.Initialize();

	while (app.IsRunning()) {
		app.MainLoop();
	}

	app.Terminate();

	return EXIT_SUCCESS;
}

void Application::generatePointCloud() {
	if (pointData.empty()) {
		std::random_device rd;
		std::mt19937 generator(rd());
		std::uniform_real_distribution<float> dist(-1.0, +1.0);
		auto constexpr NumPoints{10000U};
		pointData.reserve(NumPoints * 6);
		for (auto i = 0U; i < NumPoints; ++i) {
			for (int j = 0; j < 6; ++j) {
				pointData.push_back(dist(generator));
			}
		}
	}
}

void Application::setupPointCloudPipeline() {
	if (!pointCloudRenderPipelineDesc) {
		VertexBufferLayout vertexBufferLayout;
		vertexBufferLayout.attributeCount = 2;
		auto& positionAttribute = vertexAttributes[0];
		auto& colorAttribute = vertexAttributes[1];
		vertexBufferLayout.attributes = vertexAttributes.data();
		vertexBufferLayout.stepMode = VertexStepMode::Vertex;
		vertexBufferLayout.arrayStride = 6 * sizeof(float);
		positionAttribute.shaderLocation = 0;
		positionAttribute.format = VertexFormat::Float32x3;
		positionAttribute.offset = 0;
		colorAttribute.shaderLocation = 1;
		colorAttribute.format = VertexFormat::Float32x3;
		colorAttribute.offset = 3 * sizeof(float);

		pointCloudRenderPipelineDesc = std::make_optional<RenderPipelineDescriptor>();
		pointCloudRenderPipelineDesc->vertex.bufferCount = 1;
		pointCloudRenderPipelineDesc->vertex.buffers = &vertexBufferLayout;

		ShaderLoader pointCloudShaderLoader("PointCloud.wgsl");
		pointCloudRenderPipelineDesc->vertex.module = pointCloudShaderLoader.getShaderModule(device);
		pointCloudRenderPipelineDesc->vertex.entryPoint = "vs_main";
		pointCloudRenderPipelineDesc->vertex.constantCount = 0;
		pointCloudRenderPipelineDesc->vertex.constants = nullptr;

		pointCloudRenderPipelineDesc->primitive.topology = PrimitiveTopology::PointList;
		pointCloudRenderPipelineDesc->primitive.stripIndexFormat = IndexFormat::Undefined;
		pointCloudRenderPipelineDesc->primitive.frontFace = FrontFace::CCW;
		pointCloudRenderPipelineDesc->primitive.cullMode = CullMode::None;

		FragmentState fragmentState;
		fragmentState.module = pointCloudShaderLoader.getShaderModule(device);
		fragmentState.entryPoint = "fs_main";
		fragmentState.constantCount = 0;
		fragmentState.constants = nullptr;

		BlendState blendState;
		blendState.color.srcFactor = BlendFactor::SrcAlpha;
		blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
		blendState.color.operation = BlendOperation::Add;
		blendState.alpha.srcFactor = BlendFactor::Zero;
		blendState.alpha.dstFactor = BlendFactor::One;
		blendState.alpha.operation = BlendOperation::Add;

		ColorTargetState colorTarget;
		colorTarget.format = surfaceFormat;
		colorTarget.blend = &blendState;
		colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.

		// We have only one target because our render pass has only one output color
		// attachment.
		fragmentState.targetCount = 1;
		fragmentState.targets = &colorTarget;
		pointCloudRenderPipelineDesc->fragment = &fragmentState;

		pointCloudRenderPipelineDesc->depthStencil = nullptr;

		pointCloudRenderPipelineDesc->multisample.count = 1;
		pointCloudRenderPipelineDesc->multisample.mask = ~0u;
		pointCloudRenderPipelineDesc->multisample.alphaToCoverageEnabled = false;
		pointCloudRenderPipelineDesc->layout = nullptr;

		pointCloudPipeline = device.createRenderPipeline(*pointCloudRenderPipelineDesc);

		pointCloudShaderLoader.getShaderModule(device).release();

		BufferDescriptor vertexBufferDescriptor;
		generatePointCloud();
		vertexBufferDescriptor.size = pointData.size() * sizeof(float);
		vertexBufferDescriptor.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
		vertexBufferDescriptor.mappedAtCreation = false;
		vertexBuffer = device.createBuffer(vertexBufferDescriptor);
		queue.writeBuffer(vertexBuffer, 0, pointData.data(), vertexBufferDescriptor.size);
	}
}

void Application::drawPointCloud(CommandEncoder& encoder, RenderPassDescriptor const& renderPassDesc) {
	setupPointCloudPipeline();
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
	renderPass.setPipeline(*pointCloudPipeline);
	renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexBuffer.getSize());
	renderPass.draw(pointData.size() / 6, 1, 0, 0);
	renderPass.end();
	renderPass.release();
}

void Application::Initialize() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(640, 480, "WebGPU Point Cloud", nullptr, nullptr);

	Instance instance = wgpuCreateInstance(nullptr);
	surface = glfwGetWGPUSurface(instance, window);
    std::println("Requesting adapter...");
	surface = glfwGetWGPUSurface(instance, window);
	RequestAdapterOptions adapterOpts;
	adapterOpts.compatibleSurface = surface;
	Adapter adapter = instance.requestAdapter(adapterOpts);
	instance.release();

    std::println("Requesting device...");
	DeviceDescriptor deviceDesc;
	deviceDesc.label = "WebGpu Device";
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "Default Queue";
	deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
		std::println("Webgpu device lost. Reason: {} Message: {}", static_cast<int>(reason), message);
	};
	device = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << device << std::endl;
	
	uncapturedErrorCallbackHandle = device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
		std::println("Uncaptured device error: Type {} Message {}", static_cast<int>(type), message);
	});

	queue = device.getQueue();

	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
		std::println("Queued work finished with status: {}", static_cast<int>(status));
	};
	wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);

	SurfaceConfiguration config;

	config.width = 640;
	config.height = 480;
	config.usage = TextureUsage::RenderAttachment;
	surfaceFormat = surface.getPreferredFormat(adapter);
	config.format = surfaceFormat;

	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = PresentMode::Fifo;
	config.alphaMode = CompositeAlphaMode::Auto;

	surface.configure(config);

	adapter.release();

}

void Application::Terminate() {
	vertexBuffer.release();
	surface.unconfigure();
	queue.release();
	surface.release();
	device.release();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::MainLoop() {
	glfwPollEvents();

	TextureView targetView = GetNextSurfaceTextureView();
	if (!targetView) {
        return;
    }

	CommandEncoderDescriptor encoderDesc;
	encoderDesc.label = "Render command encoder";
	CommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	RenderPassDescriptor renderPassDesc;
	RenderPassColorAttachment renderPassColorAttachment;
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = LoadOp::Clear;
	renderPassColorAttachment.storeOp = StoreOp::Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.5, 0.5, 0.5, 1.0 };

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

	drawPointCloud(encoder, renderPassDesc);

	// Finally encode and submit the render pass
	CommandBufferDescriptor cmdBufferDescriptor;
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();

	queue.submit(1, &command);
	command.release();

	targetView.release();
	surface.present();

	device.poll(false);
}

bool Application::IsRunning() const {
	return !glfwWindowShouldClose(window);
}

TextureView Application::GetNextSurfaceTextureView() {
	SurfaceTexture surfaceTexture;
	surface.getCurrentTexture(&surfaceTexture);
	if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
		return nullptr;
	}
	Texture texture = surfaceTexture.texture;

	// Create a view for this surface texture
	TextureViewDescriptor viewDescriptor;
	viewDescriptor.label = "Surface texture view";
	viewDescriptor.format = texture.getFormat();
	viewDescriptor.dimension = TextureViewDimension::_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = TextureAspect::All;

	return texture.createView(viewDescriptor);
}
