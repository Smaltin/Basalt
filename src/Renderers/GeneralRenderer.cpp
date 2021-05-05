#include "GeneralRenderer.h"


GeneralRenderer::GeneralRenderer(vn::Device* mainDevice, VkRenderPass* rpass, VkDescriptorSetLayout desclayout) //: m_descriptorbuffer(vn::vk::BufferDescription{})
{
	p_device = mainDevice;
	vn::vk::createCommandPool(*p_device, m_pool);
	m_renderpass = rpass;

	if (img.loadFromFile("res/container.jpg"))
	{
		std::cout << "Image creation success \n";
	}

	vn::vk::Texture texture(p_device);
	texture.loadFromImage(img);
	vn::asset_manager.addTexture(texture, "container");

	m_renderlist.resize(1);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandBufferCount = m_renderlist.size();

	if (vkAllocateCommandBuffers(p_device->getDevice(), &allocInfo, m_renderlist.data()) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // Optional


	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	
	// Set to Null because the finish render function has the target frame buffer to render to
	inheritanceInfo.framebuffer = VK_NULL_HANDLE;

	beginInfo.pInheritanceInfo = &inheritanceInfo;

	// Mesh
	vn::Mesh m = vn::loadMeshFromObj("res/Models/sphere.obj");\
	auto* model = new vn::vk::Model(m, p_device);
	vn::asset_manager.addModel(*model, "sphere");

	// Pipelines
	vn::vk::createPipeline(*p_device, gfx, *m_renderpass, playout, desclayout);
}

void GeneralRenderer::addInstance(vn::GameObject& entity)
{
	//Hack/Workaround to avoid cyclic dependency/init of the renderpass
	if (!firstrun)
	{
		inheritanceInfo.renderPass = *m_renderpass; //Renderpass
		firstrun = true;
	}

	m_queue.emplace_back(entity);
}

void GeneralRenderer::render(Camera& cam)
{

	PushConstantsStruct pushconst = {};
	pushconst.proj = cam.getProjMatrix();
	pushconst.view = cam.getViewMatrix();
	
	vn::Transform t;
	t.pos.x = 0.0f;
	t.pos.y = 0.0f;
	t.pos.z = 0.0f;

	t.rescale(t, vn::vec3(0.5f, 0.5f, 0.5f));

	//pushconst.model = vn::makeModelMatrix(t);
	

	for (uint8_t i = 0; i < m_queue.size(); ++i)
	{
		if (vkBeginCommandBuffer(m_renderlist.at(i), &beginInfo) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		vkCmdBindPipeline(m_renderlist.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, gfx);

		vkCmdBindDescriptorSets(m_renderlist.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1, 
								vn::asset_manager.pDescsetglobal, 0, nullptr);

		vkCmdPushConstants(m_renderlist.at(i), playout,
			VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsStruct), &pushconst);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(m_renderlist.at(i), 0, 1, &vn::asset_manager.getModel(std::move(m_queue.at(i).model_id)).getVertexBuffer()->getAPIResource(), &offset);
		
		vkCmdBindIndexBuffer(m_renderlist.at(i), vn::asset_manager.getModel(std::move(m_queue.at(i).model_id)).getIndexBuffer()->getAPIResource(), offset, VK_INDEX_TYPE_UINT32);
		

		vkCmdDrawIndexed(m_renderlist.at(i), vn::asset_manager.getModel(std::move(m_queue.at(i).model_id)).getIndexBuffer()->getNumElements(), 1, 0, 0, 0);

		if (vkEndCommandBuffer(m_renderlist.at(i)) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void GeneralRenderer::clearQueue()
{
	m_queue.clear();
	vkResetCommandPool(p_device->getDevice(), m_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

// Get list of secondary cmd buffers
std::vector<VkCommandBuffer>& GeneralRenderer::getRenderlists()
{
	return m_renderlist;
}

GeneralRenderer::~GeneralRenderer()
{
	/*for (auto* model : m_models)
	{
		delete model;
	}*/
}
