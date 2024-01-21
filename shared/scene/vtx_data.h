#ifndef __VTX_DATA_H__
#define __VTX_DATA_H__

#include <defines.h>

#define MAX_LODS 8
#define MAX_STREAMS 8

typedef struct
t_mesh_v
{
  u32 lod_count;
  u32 stream_count;
  u32 index_offset;
  u32 vertex_offset;
  u32 vertex_count;
  u32 lod_offset[MAX_LODS];
  u32 stream_offset[MAX_STREAMS];
  u32 stream_element_size[MAX_STREAMS];
}
t_mesh_v;

t_mesh_v
mesh_create()
{
  t_mesh_v mesh = {0};
  mesh.lod_count = 1;

  return mesh;
}

u32
get_lod_indices_count(t_mesh_v* mesh, u32 lod)
{
  return mesh->lod_offset[lod + 1] - mesh->lod_offset[lod];
}

typedef struct
t_mesh_file_header
{
  u32 magic_value;
  u32 mesh_count;
  u32 data_block_start_offset;
  u32 index_data_size;
  u32 vertex_data_size;
}
t_mesh_file_header;

typedef struct
t_draw_data
{
  u32 mesh_index;
  u32 material_index;
  u32 lod;
  u32 index_offset;
  u32 vertex_offset;
  u32 transform_index;
}
t_draw_data;

typedef struct
t_mesh_data
{
  cvec(u32) index_data;
  cvec(f32) vertex_data;
  cvec(t_mesh_v) meshes;
  cvec(bbox) boxes;
}
t_mesh_data;

t_mesh_file_header
load_mesh_data(const char* mesh_file,
               t_mesh_data* out_mesh_data)
{
  t_mesh_file_header header;

	FILE* f = fopen(mesh_file, "rb");

	assert(f); // Did you forget to run "Ch5_Tool05_MeshConvert"?

	if (!f)
	{
		printf("Cannot open %s. Did you forget to run \"Ch5_Tool05_MeshConvert\"?\n", mesh_file);
		exit(EXIT_FAILURE);
	}

	if (fread(&header, 1, sizeof(header), f) != sizeof(header))
	{
		printf("Unable to read mesh file header\n");
		exit(EXIT_FAILURE);
	}

  out_mesh_data->meshes = cvec_ncreate(t_mesh_v, header.mesh_count);

	if (fread(out_mesh_data->meshes, sizeof(t_mesh_v), header.mesh_count, f) != header.mesh_count)
	{
		printf("Could not read mesh descriptors\n");
		exit(EXIT_FAILURE);
	}

  out_mesh_data->boxes = cvec_ncreate(bbox, header.mesh_count);

	if (fread(out_mesh_data->boxes, sizeof(bbox), header.mesh_count, f) != header.mesh_count)
	{
		printf("Could not read bounding boxes\n");
		exit(255);
	}

  out_mesh_data->index_data = cvec_ncreate(u32, header.index_data_size / sizeof(u32));
  out_mesh_data->vertex_data = cvec_ncreate(f32, header.vertex_data_size / sizeof(f32));

	if ((fread(out_mesh_data->index_data, 1, header.index_data_size, f) != header.index_data_size) ||
		  (fread(out_mesh_data->vertex_data, 1, header.vertex_data_size, f) != header.vertex_data_size))
	{
		printf("Unable to read index/vertex data\n");
		exit(255);
	}

	fclose(f);

	return header;
}

void
save_mesh_data(const char* mesh_file,
               const t_mesh_data* mesh_data)
{
  FILE *f = fopen(mesh_file, "wb");

	const t_mesh_file_header header =
  {
		.magic_value = 0x12345678,
		.mesh_count = (u32)cvec_size(mesh_data->meshes),
		.data_block_start_offset = (u32)(sizeof(t_mesh_file_header) + cvec_size(mesh_data->meshes) * sizeof(t_mesh_v)),
		.index_data_size = (u32)(cvec_size(mesh_data->index_data) * sizeof(u32)),
		.vertex_data_size = (u32)(cvec_size(mesh_data->vertex_data) * sizeof(f32))
	};

	fwrite(&header, 1, sizeof(header), f);
	fwrite(mesh_data->meshes, sizeof(t_mesh_v), header.mesh_count, f);
	fwrite(mesh_data->boxes, sizeof(bbox), header.mesh_count, f);
	fwrite(mesh_data->index_data, 1, header.index_data_size, f);
	fwrite(mesh_data->vertex_data, 1, header.vertex_data_size, f);

	fclose(f);
}

void
recalc_bbox(t_mesh_data* mesh_data)
{
  cvec_clear(mesh_data->boxes);

	u64 size = cvec_size(mesh_data->meshes);

	u64 vd_size = cvec_size(mesh_data->vertex_data);

  for (u64 index = 0; index < size; index++)
	{
		const u32 numIndices = get_lod_indices_count(&mesh_data->meshes[index], 0);

		vec3 vmin = { FLT_MAX, FLT_MAX, FLT_MAX };
	  vec3 vmax = { FLT_MIN, FLT_MIN, FLT_MIN };

    for (u64 i = 0; i != numIndices; i++)
		{
			i32 vtxOffset = mesh_data->index_data[mesh_data->meshes[index].index_offset + i] + mesh_data->meshes[index].vertex_offset;
			const float* vf = &mesh_data->vertex_data[vtxOffset * MAX_STREAMS];
			// vmin = MIN(vmin, (vec3){vf[0], vf[1], vf[2]});
      vmin.x = MIN(vmin.x, vf[0]);
      vmin.y = MIN(vmin.y, vf[1]);
      vmin.z = MIN(vmin.z, vf[2]);

			// vmax = MAX(vmax, (vec3){vf[0], vf[1], vf[2]});
      vmax.x = MAX(vmax.x, vf[0]);
      vmax.y = MAX(vmax.y, vf[1]);
      vmax.z = MAX(vmax.z, vf[2]);
		}

    bbox box = (bbox){ vmin, vmax };
    cvec_push(mesh_data->boxes, box);
	}
}

t_mesh_file_header
merge_mesh_data(t_mesh_data* mesh_data,
                cvec(t_mesh_data*)* mds)
{
  u32 totalVertexDataSize = 0;
	u32 totalIndexDataSize  = 0;

	u32 offs = 0;
	//for (const t_mesh_data* i: md)
  for (u64 i = 0; i < cvec_size(*mds); i++)
	{
    t_mesh_data* md = (*mds)[i];
    
		cvec_merge(mesh_data->index_data, md->index_data);
		cvec_merge(mesh_data->vertex_data, md->vertex_data);
		cvec_merge(mesh_data->meshes, md->meshes);
		cvec_merge(mesh_data->boxes, md->boxes);

		u32 vtxOffset = totalVertexDataSize / 8;  /* 8 is the number of per-vertex attributes: position, normal + UV */

		for (u64 j = 0 ; j < (u32)cvec_size(md->meshes); j++)
			// m.vertexCount, m.lodCount and m.streamCount do not change
			// m.vertexOffset also does not change, because vertex offsets are local (i.e., baked into the indices)
			mesh_data->meshes[offs + j].index_offset += totalIndexDataSize;

		// shift individual indices
		for(size_t j = 0 ; j < cvec_size(md->index_data) ; j++)
			mesh_data->index_data[totalIndexDataSize + j] += vtxOffset;

		offs += (u32)cvec_size(md->meshes);

		totalIndexDataSize += (u32)cvec_size(md->index_data);
		totalVertexDataSize += (u32)cvec_size(md->vertex_data);
	}

	return (t_mesh_file_header)
  {
		.magic_value = 0x12345678,
		.mesh_count = (u32)offs,
		.data_block_start_offset = (u32)(sizeof(t_mesh_file_header) + offs * sizeof(t_mesh_v)),
		.index_data_size = (u32)(totalIndexDataSize * sizeof(u32)),
		.vertex_data_size = (u32)(totalVertexDataSize * sizeof(f32))
	};
}

#endif