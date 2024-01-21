#include <defines.h>

#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include <math/mathm.c>

#define CVEC_IMPLEMENTATION
#define CVEC_STDLIB
#include <containers/cvec.h>

#define CQUE_IMPLEMENTATION
#define CQUE_STDLIB
#include <containers/cque.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define TINYOBJ_MALLOC malloc
#define TINYOBJ_CALLOC calloc
#define TINYOBJ_REALLOC realloc
#define TINYOBJ_FREE free

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <tinyobj_loader_c.h>


#include <tools.h>
#include <vk/vk_shader_compiler.h>
#include <resource/model_loader.h>
#include <vk/vk_core.h>

#include <vk/vk_app.h>

// #include <gli/gli.hpp>
// #include <gli/texture2d.hpp>
// #include <gli/load_ktx.hpp>
#include <gli/gli_c_wrapper.h>

#include <vk/renderers/vk_compute_base.c>

#define brdfW 256
#define brdfH 256

// const u32 bufferSize = 2 * sizeof(f32) * brdfW * brdfH;
#define bufferSize (2 * sizeof(f32) * brdfW * brdfH)

f32 lutData[bufferSize];

t_vk_app vk_app;
vulkan_instance vk;
vulkan_render_device vk_dev;

void
calculate_lut(f32* out_data)
{
  t_compute_base cb = compute_base_create(&vk_dev,
                                          "data/shaders/chapter06/VK01_BRDF_LUT.comp",
                                          sizeof(f32),
                                          bufferSize);

	if (!compute_base_execute(&cb, brdfW, brdfH, 1))
		exit(EXIT_FAILURE);

  compute_base_download_output(&cb, 0, (u8*)out_data, bufferSize);

	compute_base_destroy(&cb);
}

void// texture
convert_lut_to_texture(f32* data)
{
	gli_texture* texture = gli_texture2d_create(GLI_FORMAT_RG16_SFLOAT_PACK16, brdfW, brdfH);
	
	for (int y = 0; y < brdfH; y++)
	{
		for (int x = 0; x < brdfW; x++)
		{
			const int ofs = y * brdfW + x;
			const vec2 value = { data[ofs * 2 + 0], data[ofs * 2 + 1] };
			gli_texture2d_store(texture, x, y, packHalf2x16(value));
		}
	}

	gli_texture2d_save_ktx(texture, "data/brdfLUT_test.ktx");
}

int main()
{
  vk_app = vk_app_create(brdfW, brdfH, NULL);

  vulkan_create_instance(&vk);

  if (!vulkan_setup_debug_callbacks(vk.instance, &vk.messenger, &vk.report_callback))
		exit(EXIT_FAILURE);

	if (glfwCreateWindowSurface(vk.instance, vk_app.window, NULL, &vk.surface))
		exit(EXIT_FAILURE);

  VkPhysicalDeviceFeatures features = {0};

  if (!vulkan_init_render_device_compute(&vk,
																	       &vk_dev,
																	       brdfW,
																	       brdfH,
																	       features))
	{
		exit(EXIT_FAILURE);
	}

  printf("Calculating LUT texture...\n");
	calculate_lut(lutData);

  printf("Saving LUT texture...\n");
	/*gli::texture lutTexture =*/ convert_lut_to_texture(lutData);

  // use Pico Pixel to view https://pixelandpolygon.com/ 
	// gli::save_ktx(lutTexture, "data/brdfLUT.ktx");

	vulkan_destroy_render_device(&vk_dev);
  vulkan_destroy_instance(&vk);

	glfwTerminate();
	// glslang_finalize_process();

  return 0;
}