#include "BindlessVk/Texture/Loaders/KtxLoader.hpp"


namespace BINDLESSVK_NAMESPACE {

KtxLoader::KtxLoader(
    VkContext const *const vk_context,
    MemoryAllocator const *const memory_allocator,
    Buffer *const staging_buffer
)
    : device(vk_context->get_device())
    , memory_allocator(memory_allocator)
    , staging_buffer(staging_buffer)
{
	texture.device = vk_context->get_device();
	texture.memory_allocator = memory_allocator;
}

Texture KtxLoader::load(
    str_view const path,
    Texture::Type const type,
    vk::ImageLayout const final_layout,
    str_view const debug_name
)
{
	texture.debug_name = debug_name;
	load_ktx_texture(path);

	create_image();
	create_image_view();
	create_sampler();

	stage_texture_data();
	write_texture_data_to_gpu(final_layout);

	destroy_ktx_texture();

	return std::move(texture);
}

void KtxLoader::load_ktx_texture(str_view const path)
{
	assert_false(
	    ktxTexture_CreateFromNamedFile(
	        path.data(),
	        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
	        &ktx_texture
	    ),
	    "Failed to load ktx file: \nname: {}\npath: {}",
	    texture.debug_name,
	    path
	);

	texture.size = { ktx_texture->baseWidth, ktx_texture->baseHeight };
	texture.format = vk::Format::eB8G8R8A8Srgb;
	texture.mip_levels = ktx_texture->numLevels;
	texture.device_size = ktxTexture_GetSize(ktx_texture);
	texture.current_layout = vk::ImageLayout::eUndefined;
}

void KtxLoader::destroy_ktx_texture()
{
	ktxTexture_Destroy(ktx_texture);
}

void KtxLoader::create_image()
{
	auto const [width, height] = texture.size;

	texture.image = Image {
		memory_allocator,

		vk::ImageCreateInfo {
		    vk::ImageCreateFlagBits::eCubeCompatible,
		    vk::ImageType::e2D,
		    texture.format,
		    vk::Extent3D {
		        width,
		        height,
		        1u,
		    },
		    texture.mip_levels,
		    6u,
		    vk::SampleCountFlagBits::e1,
		    vk::ImageTiling::eOptimal,
		    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		    vk::SharingMode::eExclusive,
		    0u,
		    nullptr,
		    vk::ImageLayout::eUndefined,
		},

		vma::AllocationCreateInfo {
		    {},
		    vma::MemoryUsage::eGpuOnly,
		    vk::MemoryPropertyFlagBits::eDeviceLocal,
		},
	};
}

void KtxLoader::create_image_view()
{
	texture.image_view = device->vk().createImageView(vk::ImageViewCreateInfo {
	    {},
	    texture.image.vk(),
	    vk::ImageViewType::eCube,
	    texture.format,
	    vk::ComponentMapping {
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	    },
	    vk::ImageSubresourceRange {
	        vk::ImageAspectFlagBits::eColor,
	        0u,
	        texture.mip_levels,
	        0u,
	        6u,
	    },
	});
	texture.descriptor_info.imageView = texture.image_view;
}

void KtxLoader::create_sampler()
{
	texture.sampler = device->vk().createSampler(vk::SamplerCreateInfo {
	    {},
	    vk::Filter::eLinear,
	    vk::Filter::eLinear,
	    vk::SamplerMipmapMode::eLinear,
	    vk::SamplerAddressMode::eClampToEdge,
	    vk::SamplerAddressMode::eClampToEdge,
	    vk::SamplerAddressMode::eClampToEdge,
	    0.0f,
	    VK_FALSE, // @todo anisotropy
	    {},
	    VK_FALSE,
	    vk::CompareOp::eNever,
	    0.0f,
	    static_cast<float>(texture.mip_levels),
	    vk::BorderColor::eFloatOpaqueWhite,
	    VK_FALSE,
	});
	texture.descriptor_info.sampler = texture.sampler;
}

void KtxLoader::stage_texture_data()
{
	auto const *const src = ktxTexture_GetData(ktx_texture);
	auto *const dst = staging_buffer->map_block(0);
	memcpy(dst, reinterpret_cast<void const *>(src), texture.device_size);
	staging_buffer->unmap();
}

void KtxLoader::write_texture_data_to_gpu(vk::ImageLayout const final_layout)
{
	auto const buffer_copies = create_texture_face_buffer_copies();

	device->immediate_submit([&](vk::CommandBuffer &&cmd) {
		texture.transition_layout(
		    cmd,
		    0u,
		    texture.mip_levels,
		    6u,
		    vk::ImageLayout::eTransferDstOptimal
		);

		cmd.copyBufferToImage(
		    *staging_buffer->vk(),
		    texture.image.vk(),
		    vk::ImageLayout::eTransferDstOptimal,
		    static_cast<u32>(buffer_copies.size()),
		    buffer_copies.data()
		);

		texture.transition_layout(cmd, 0u, texture.mip_levels, 6u, final_layout);
	});

	texture.descriptor_info.imageLayout = texture.current_layout;
}

vec<vk::BufferImageCopy> KtxLoader::create_texture_face_buffer_copies()
{
	auto const [width, height] = texture.size;

	auto buffer_copies = vec<vk::BufferImageCopy> {};

	for (u32 face = 0u; face < 6u; ++face)
	{
		for (u32 level = 0u; level < texture.mip_levels; ++level)
		{
			usize offset;
			assert_false(
			    ktxTexture_GetImageOffset(ktx_texture, level, 0u, face, &offset),
			    "Failed to get ktx image offset"
			);

			buffer_copies.emplace_back(vk::BufferImageCopy {
			    offset,
			    {},
			    {},
			    {
			        vk::ImageAspectFlagBits::eColor,
			        level,
			        face,
			        1u,
			    },
			    {},
			    {
			        width >> level,
			        height >> level,
			        1u,
			    },
			});
		}
	}

	return buffer_copies;
}

} // namespace BINDLESSVK_NAMESPACE
