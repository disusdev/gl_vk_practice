#include <defines.h>

#if defined(_WIN32) && defined(__TINYC__)
  #ifdef __int64
    #undef __int64
  #endif
  #define __int64 long
  #define _ftelli64 ftell
#endif

#define CGLTF_IMPLEMENTATION
#define CGLTF_VALIDATE_ENABLE_ASSERTS 1
#include <cgltf.h>

#if defined(_WIN32) && defined(__TINYC__)
  #undef __int64
  #define __int64 long long
  #undef _ftelli64
#endif

#include <math/mathm.c>

#define CVEC_IMPLEMENTATION
#define CVEC_STDLIB
#include <containers/cvec.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define CBITMAP_STDLIB
#define CBITMAP_IMPLEMENTATION
#include <resource/cbitmap.h>

#define TINYOBJ_MALLOC malloc
#define TINYOBJ_CALLOC calloc
#define TINYOBJ_REALLOC realloc
#define TINYOBJ_FREE free

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <tinyobj_loader_c.h>

#include <tools.h>

#include <resource/model_loader.h>

#include <scene/vtx_data.h>

t_mesh_data g_mesh_data;

u32 g_index_offset;
u32 g_vertex_offset;

const u32 NUM_ELEMENTS_TO_STORE = 3 + 3 + 2;

f32 g_mesh_scale = 0.01;
b8 g_calc_lods = false;

t_mesh_v
convert_mesh(t_mesh mesh, const mat4* transform)
{
  const b8 hasTexCoords = true;// m->HasTextureCoords(0);
	const u32 streamElementSize = (u32)(NUM_ELEMENTS_TO_STORE * sizeof(f32));

  t_mesh_v out_mesh =
  {
		.stream_count = 1,
		.index_offset = g_index_offset,
		.vertex_offset = g_vertex_offset,
		.vertex_count = mesh.vertexCount,
		.stream_offset = { g_vertex_offset * streamElementSize },
		.stream_element_size = { streamElementSize }
	};

	// Original data for LOD calculation
	// std::vector<f32> srcVertices;

	mesh.vn_vertices = cvec_ncreate(VertexData, mesh.vertexCount);

	for (u64 i = 0; i < mesh.vertexCount; i++)
	{
		vec3 pos = vec3_zero();
		pos.x = mesh.v_vertices[i * 3];
		pos.y = mesh.v_vertices[i * 3 + 1];
		pos.z = mesh.v_vertices[i * 3 + 2];

		if (transform)
		{
			// transform vertices
			pos = mat4_mul_point(*transform, pos);
		}

		mesh.vn_vertices[i].pos = pos;

		mesh.vn_vertices[i].n.x = mesh.v_normals[i * 3];
		mesh.vn_vertices[i].n.y = mesh.v_normals[i * 3 + 1];
		mesh.vn_vertices[i].n.z = mesh.v_normals[i * 3 + 2];

		mesh.vn_vertices[i].tc.x = mesh.v_texcoords[i * 2];
		mesh.vn_vertices[i].tc.y = mesh.v_texcoords[i * 2 + 1];
	}

	
	for (u64 i = 0; i < mesh.vertexCount; i++)
	{
		f32* v_ptr = &mesh.vn_vertices[i];
		cvec_push(g_mesh_data.vertex_data, v_ptr[0]);
		cvec_push(g_mesh_data.vertex_data, v_ptr[1]);
		cvec_push(g_mesh_data.vertex_data, v_ptr[2]);
		cvec_push(g_mesh_data.vertex_data, v_ptr[3]);
		cvec_push(g_mesh_data.vertex_data, v_ptr[4]);
		cvec_push(g_mesh_data.vertex_data, v_ptr[5]);
		cvec_push(g_mesh_data.vertex_data, v_ptr[6]);
		cvec_push(g_mesh_data.vertex_data, v_ptr[7]);
	}

	cvec(u32) srcIndices = cvec_ncreate_copy(u32, mesh.triangleCount * 3, mesh.v_indices, mesh.triangleCount * 3);

	u32 numIndices = 0;

	if (!g_mesh_data.index_data)
		g_mesh_data.index_data = cvec_create(u32);

  for (u64 i = 0; i < cvec_size(srcIndices); i++)
  {
    cvec_push(g_mesh_data.index_data, srcIndices[i]);
  }

  out_mesh.lod_count = 1;
  out_mesh.lod_offset[0] = 0;
  numIndices += (int)cvec_size(srcIndices);
	out_mesh.lod_offset[1] = (int)cvec_size(srcIndices);

	// for (u64 l = 0 ; l < outLods.size() ; l++)
	// {
	// 	for (u64 i = 0 ; i < outLods[l].size() ; i++)
	// 		g_meshData.indexData_.push_back(outLods[l][i]);

	// 	result.lodOffset[l] = numIndices;
	// 	numIndices += (int)outLods[l].size();
	// }

	g_index_offset += numIndices;
	g_vertex_offset += mesh.vertexCount;

  return out_mesh;
}

void load_file(const char* file_path)
{
  // t_model* model = LoadOBJ(file_path);
  t_model* model = LoadGLTF(file_path);

	g_mesh_data.vertex_data = cvec_create(f32);
  g_mesh_data.meshes = cvec_ncreate(t_mesh_v, model->meshCount);
  g_mesh_data.boxes = cvec_ncreate(bbox, model->meshCount);

  for (u32 i = 0; i != model->meshCount; i++)
	{
		g_mesh_data.meshes[i] = convert_mesh(model->meshes[i], &model->transforms[i]);
	}

  recalc_bbox(&g_mesh_data);
}

int main()
{
  // load_file("deps/src/bistro/Exterior/exterior.obj");
  load_file("data/bistro_5_2/gltf/bistro.gltf");
  // load_file("data/DragonAttenuation/glTF/DragonAttenuation.gltf");
  //load_file("data/city_grid_block/scene.gltf");

  cvec(t_draw_data) grid = cvec_create(t_draw_data);
	g_vertex_offset = 0;

  for (u64 i = 0; i < cvec_size(g_mesh_data.meshes); i++)
	{
    t_draw_data draw_data =
    {
			.mesh_index = (u32)i,
			.material_index = 0,
			.lod = 0,
			.index_offset = g_mesh_data.meshes[i].index_offset,
			.vertex_offset = g_vertex_offset,
			.transform_index = 0
		};
    cvec_push(grid, draw_data);

		g_vertex_offset += g_mesh_data.meshes[i].vertex_count;
	}

  save_mesh_data("data/meshes/test.meshes", &g_mesh_data);

	FILE* f = fopen("data/meshes/test.meshes.drawdata", "wb");
	fwrite(grid, cvec_size(grid), sizeof(t_draw_data), f);
	fclose(f);

  return 0;
}