#ifndef __MODEL_LOADER_H__
#define __MODEL_LOADER_H__

//#include "../defines.h"
//#include "../math/mathm.h"
//#include <stdlib.h>
//#include <string.h>
//

// #include <cgltf.h>
// #include <tinyobj_loader_c.h>
//#include <tools.h>

#define TRACELOG(p1, p2, p3) printf(p2, p3)
#define RL_CALLOC(count, size) calloc(count, size)
#define RL_MALLOC(size) malloc(size)
#define RL_FREE(ptr) free(ptr)

typedef struct
VertexData
{
	vec3 pos;
	// vec3 n;
	vec2 tc;
}
VertexData;

typedef struct
t_mesh
{
  u64 vertexCount;
  u64 triangleCount;
	float* v_vertices;
  float* v_normals;
  float* v_texcoords;
	u32* v_indices;

	cvec(VertexData) vn_vertices;
}
t_mesh;

typedef struct
t_model
{
	mat4 transform;
  cvec(mat4) transforms;
	int meshCount;
	cvec(t_mesh) meshes;
}
t_model;

t_model* LoadGLTF(const char* fileName)
{
  t_model* model = RL_MALLOC(sizeof(t_model));
  memset(model, 0, sizeof(t_model));

  /*********************************************************************************************

      Function implemented by Wilhem Barbier(@wbrbr), with modifications by Tyler Bezera(@gamerfiend)
      Reviewed by Ramon Santamaria (@raysan5)

      FEATURES:
        - Supports .gltf and .glb files
        - Supports embedded (base64) or external textures
        - Supports PBR metallic/roughness flow, loads material textures, values and colors
                   PBR specular/glossiness flow and extended texture flows not supported
        - Supports multiple meshes per model (every primitives is loaded as a separate mesh)

      RESTRICTIONS:
        - Only triangle meshes supported
        - Vertex attibute types and formats supported:
            > Vertices (position): vec3: float
            > Normals: vec3: float
            > Texcoords: vec2: float
            > Colors: vec4: u8, u16, f32 (normalized)
            > Indices: u16, u32 (truncated to u16)
        - Node hierarchies or transforms not supported

  ***********************************************************************************************/

  // Macro to simplify attributes loading code
#define LOAD_ATTRIBUTE(accesor, numComp, dataType, dstPtr) \
    { \
        int n = 0; \
        dataType *buffer = (dataType *)accesor->buffer_view->buffer->data + accesor->buffer_view->offset/sizeof(dataType) + accesor->offset/sizeof(dataType); \
        for (unsigned int k = 0; k < accesor->count; k++) \
        {\
            for (int l = 0; l < numComp; l++) \
            {\
                dstPtr[numComp*k + l] = buffer[n + l];\
            }\
            n += (int)(accesor->stride/sizeof(dataType));\
        }\
    }

  // glTF file loading
  unsigned int dataSize = 0;
  unsigned char* fileData = LoadFileData(fileName, &dataSize);

  if (fileData == NULL)
  {
    RL_FREE(model);
    model = NULL;
    return model;
  }

  // glTF data loading
  cgltf_options options = { 0 };
  cgltf_data* data = NULL;
  cgltf_result result = cgltf_parse(&options, fileData, dataSize, &data);

  if (result == cgltf_result_success)
  {
    if (data->file_type == cgltf_file_type_glb) TRACELOG(LOG_INFO, "MODEL: [%s] Model basic data (glb) loaded successfully\n", fileName);
    else if (data->file_type == cgltf_file_type_gltf) TRACELOG(LOG_INFO, "MODEL: [%s] Model basic data (glTF) loaded successfully\n", fileName);
    else TRACELOG(LOG_WARNING, "MODEL: [%s] Model format not recognized", fileName);

    TRACELOG(LOG_INFO, "    > Meshes count: %i\n", data->meshes_count);
    TRACELOG(LOG_INFO, "    > Materials count: %i (+1 default)\n", data->materials_count);
    TRACELOG(LOG_DEBUG, "    > Buffers count: %i\n", data->buffers_count);
    TRACELOG(LOG_DEBUG, "    > Images count: %i\n", data->images_count);
    TRACELOG(LOG_DEBUG, "    > Textures count: %i\n", data->textures_count);

    // Force reading data buffers (fills buffer_view->buffer->data)
    // NOTE: If an uri is defined to base64 data or external path, it's automatically loaded -> TODO: Verify this assumption
    result = cgltf_load_buffers(&options, data, fileName);
    if (result != cgltf_result_success) TRACELOG(LOG_INFO, "MODEL: [%s] Failed to load mesh/material buffers\n", fileName);

    int primitivesCount = 0;
    // NOTE: We will load every primitive in the glTF as a separate raylib mesh
    for (unsigned int i = 0; i < data->meshes_count; i++) primitivesCount += (int)data->meshes[i].primitives_count;

    model->transforms = cvec_ncreate(mat4, data->nodes_count);
    cvec_resize(model->transforms, data->nodes_count);

    for (u64 i = 0; i < data->nodes_count; i++)
    {
      cgltf_node* node = &data->nodes[i];
      cgltf_node_transform_world(node, model->transforms[i].data);
    }

    // Load our model data: meshes and materials
    model->meshCount = primitivesCount;
    //model->meshes = RL_CALLOC(model->meshCount, sizeof(t_mesh));
    model->meshes = cvec_ncreate(t_mesh, model->meshCount);
    cvec_resize(model->meshes, model->meshCount);

    // Load meshes data
    //----------------------------------------------------------------------------------------------------
    for (unsigned int i = 0, meshIndex = 0; i < data->meshes_count; i++)
    {
      // NOTE: meshIndex accumulates primitives

      for (unsigned int p = 0; p < data->meshes[i].primitives_count; p++)
      {
        // NOTE: We only support primitives defined by triangles
        // Other alternatives: points, lines, line_strip, triangle_strip
        if (data->meshes[i].primitives[p].type != cgltf_primitive_type_triangles) continue;

        // NOTE: Attributes data could be provided in several data formats (8, 8u, 16u, 32...),
        // Only some formats for each attribute type are supported, read info at the top of this function!

        for (unsigned int j = 0; j < data->meshes[i].primitives[p].attributes_count; j++)
        {
          // Check the different attributes for every pimitive
          if (data->meshes[i].primitives[p].attributes[j].type == cgltf_attribute_type_position)      // POSITION
          {
            cgltf_accessor* attribute = data->meshes[i].primitives[p].attributes[j].data;

            // WARNING: SPECS: POSITION accessor MUST have its min and max properties defined.

            if ((attribute->component_type == cgltf_component_type_r_32f) && (attribute->type == cgltf_type_vec3))
            {
              // Init raylib mesh vertices to copy glTF attribute data
              model->meshes[meshIndex].vertexCount = (int)attribute->count;
              model->meshes[meshIndex].v_vertices = RL_MALLOC(attribute->count * 3 * sizeof(float));

              // Load 3 components of float data type into mesh.vertices
              LOAD_ATTRIBUTE(attribute, 3, float, model->meshes[meshIndex].v_vertices)
            }
            else TRACELOG(LOG_WARNING, "MODEL: [%s] Vertices attribute data format not supported, use vec3 float\n", fileName);
          }
          else if (data->meshes[i].primitives[p].attributes[j].type == cgltf_attribute_type_normal)   // NORMAL
          {
            cgltf_accessor* attribute = data->meshes[i].primitives[p].attributes[j].data;

            if ((attribute->component_type == cgltf_component_type_r_32f) && (attribute->type == cgltf_type_vec3))
            {
              // Init raylib mesh normals to copy glTF attribute data
              model->meshes[meshIndex].v_normals = RL_MALLOC(attribute->count * 3 * sizeof(float));

              // Load 3 components of float data type into mesh.normals
              LOAD_ATTRIBUTE(attribute, 3, float, model->meshes[meshIndex].v_normals)
            }
            else TRACELOG(LOG_WARNING, "MODEL: [%s] Normal attribute data format not supported, use vec3 float\n", fileName);
          }
          else if (data->meshes[i].primitives[p].attributes[j].type == cgltf_attribute_type_texcoord) // TEXCOORD_0
          {
            // TODO: Support additional texture coordinates: TEXCOORD_1 -> mesh.texcoords2

            cgltf_accessor* attribute = data->meshes[i].primitives[p].attributes[j].data;

            if ((attribute->component_type == cgltf_component_type_r_32f) && (attribute->type == cgltf_type_vec2))
            {
              // Init raylib mesh texcoords to copy glTF attribute data
              model->meshes[meshIndex].v_texcoords = RL_MALLOC(attribute->count * 2 * sizeof(float));

              // Load 3 components of float data type into mesh.texcoords
              LOAD_ATTRIBUTE(attribute, 2, float, model->meshes[meshIndex].v_texcoords)
            }
            else TRACELOG(LOG_WARNING, "MODEL: [%s] Texcoords attribute data format not supported, use vec2 float\n", fileName);
          }

          // NOTE: Attributes related to animations are processed separately
        }

        // Load primitive indices data (if provided)
        if (data->meshes[i].primitives[p].indices != NULL)
        {
          cgltf_accessor* attribute = data->meshes[i].primitives[p].indices;

          model->meshes[meshIndex].triangleCount = (int)attribute->count / 3;

          if (attribute->component_type == cgltf_component_type_r_16u)
          {
            // Init raylib mesh indices to copy glTF attribute data
            model->meshes[meshIndex].v_indices = RL_MALLOC(attribute->count * sizeof(unsigned int));

            // Load unsigned short data type into mesh.indices
            LOAD_ATTRIBUTE(attribute, 1, unsigned short, model->meshes[meshIndex].v_indices)
          }
          else if (attribute->component_type == cgltf_component_type_r_32u)
          {
            // Init raylib mesh indices to copy glTF attribute data
            model->meshes[meshIndex].v_indices = RL_MALLOC(attribute->count * sizeof(unsigned int));

            // Load data into a temp buffer to be converted to raylib data type
            unsigned int* temp = RL_MALLOC(attribute->count * sizeof(unsigned int));
            LOAD_ATTRIBUTE(attribute, 1, unsigned int, temp);

            // Convert data to raylib indices data type (unsigned short)
            for (unsigned int d = 0; d < attribute->count; d++) model->meshes[meshIndex].v_indices[d] = temp[d];

            //TRACELOG(LOG_WARNING, "MODEL: [%s] Indices data converted from u32 to u16, possible loss of data\n", fileName);

            RL_FREE(temp);
          }
          else TRACELOG(LOG_WARNING, "MODEL: [%s] Indices data format not supported, use u16\n", fileName);
        }
        else model->meshes[meshIndex].triangleCount = model->meshes[meshIndex].vertexCount / 3;    // Unindexed mesh

        meshIndex++;       // Move to next mesh
      }
    }

    // Free all cgltf loaded data
    cgltf_free(data);
  }
  else TRACELOG(LOG_WARNING, "MODEL: [%s] Failed to load glTF data\n", fileName);

  // WARNING: cgltf requires the file pointer available while reading data
  UnloadFileData(fileData);

  return model;
}

t_model* LoadOBJ(const char* fileName)
{
  t_model* model = malloc(sizeof(t_model));
  memset(model, 0, sizeof(t_model));

  tinyobj_attrib_t attrib = { 0 };
  tinyobj_shape_t* meshes = NULL;
  unsigned int meshCount = 0;

  tinyobj_material_t* materials = NULL;
  unsigned int materialCount = 0;

  char* fileText = read_text_file(fileName).ptr;

  if (fileText)
  {
    unsigned int dataSize = (unsigned int)strlen(fileText);
    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    int ret = tinyobj_parse_obj(&attrib, &meshes, &meshCount, &materials, &materialCount, fileText, dataSize, flags);

    //if (ret != TINYOBJ_SUCCESS) TRACELOG(LOG_WARNING, "MODEL: [%s] Failed to load OBJ data", fileName);
    //else TRACELOG(LOG_INFO, "MODEL: [%s] OBJ data loaded successfully: %i meshes/%i materials", fileName, meshCount, materialCount);

    model->meshCount = 1;

    model->meshes = cvec_ncreate(t_mesh, 1);
    cvec_resize(model->meshes, 1);

    int* matFaces = calloc(model->meshCount, sizeof(int));

    matFaces[0] = attrib.num_faces;

    int* vCount = calloc(model->meshCount, sizeof(int));
    int* vtCount = calloc(model->meshCount, sizeof(int));
    int* vnCount = calloc(model->meshCount, sizeof(int));
    int* faceCount = calloc(model->meshCount, sizeof(int));

    for (int mi = 0; mi < model->meshCount; mi++)
    {
      model->meshes[mi].vertexCount = matFaces[mi] * 3;
      model->meshes[mi].triangleCount = matFaces[mi];
      model->meshes[mi].v_vertices = (float*)calloc(model->meshes[mi].vertexCount * 3, sizeof(float));
      model->meshes[mi].v_texcoords = (float*)calloc(model->meshes[mi].vertexCount * 2, sizeof(float));
      model->meshes[mi].v_normals = (float*)calloc(model->meshes[mi].vertexCount * 3, sizeof(float));
      model->meshes[mi].v_indices = (u32*)calloc(matFaces[mi] * 3, sizeof(u32));
      // model->meshMaterial[mi] = mi;
    }


    for (unsigned int af = 0; af < attrib.num_faces; af++)
    {
      int mm = attrib.material_ids[af];   // mesh material for this face
      if (mm == -1) { mm = 0; }           // no material object..

      // Get indices for the face
      tinyobj_vertex_index_t idx0 = attrib.faces[3 * af + 0];
      cvec_push(model->meshes[mm].v_indices, (u32)cvec_size(model->meshes[mm].v_indices));
      tinyobj_vertex_index_t idx1 = attrib.faces[3 * af + 1];
      cvec_push(model->meshes[mm].v_indices, (u32)cvec_size(model->meshes[mm].v_indices));
      tinyobj_vertex_index_t idx2 = attrib.faces[3 * af + 2];
      cvec_push(model->meshes[mm].v_indices, (u32)cvec_size(model->meshes[mm].v_indices));

      // Fill vertices buffer (float) using vertex index of the face
      for (int v = 0; v < 3; v++) { model->meshes[mm].v_vertices[vCount[mm] + v] = attrib.vertices[idx0.v_idx * 3 + v]; } vCount[mm] += 3;
      for (int v = 0; v < 3; v++) { model->meshes[mm].v_vertices[vCount[mm] + v] = attrib.vertices[idx1.v_idx * 3 + v]; } vCount[mm] += 3;
      for (int v = 0; v < 3; v++) { model->meshes[mm].v_vertices[vCount[mm] + v] = attrib.vertices[idx2.v_idx * 3 + v]; } vCount[mm] += 3;

      if (attrib.num_texcoords > 0)
      {
        // Fill texcoords buffer (float) using vertex index of the face
        // NOTE: Y-coordinate must be flipped upside-down to account for
        // raylib's upside down textures...
        model->meshes[mm].v_texcoords[vtCount[mm] + 0] = attrib.texcoords[idx0.vt_idx * 2 + 0];
        model->meshes[mm].v_texcoords[vtCount[mm] + 1] = 1.0f - attrib.texcoords[idx0.vt_idx * 2 + 1]; vtCount[mm] += 2;
        model->meshes[mm].v_texcoords[vtCount[mm] + 0] = attrib.texcoords[idx1.vt_idx * 2 + 0];
        model->meshes[mm].v_texcoords[vtCount[mm] + 1] = 1.0f - attrib.texcoords[idx1.vt_idx * 2 + 1]; vtCount[mm] += 2;
        model->meshes[mm].v_texcoords[vtCount[mm] + 0] = attrib.texcoords[idx2.vt_idx * 2 + 0];
        model->meshes[mm].v_texcoords[vtCount[mm] + 1] = 1.0f - attrib.texcoords[idx2.vt_idx * 2 + 1]; vtCount[mm] += 2;
      }

      if (attrib.num_normals > 0)
      {
        // Fill normals buffer (float) using vertex index of the face
        for (int v = 0; v < 3; v++) { model->meshes[mm].v_normals[vnCount[mm] + v] = attrib.normals[idx0.vn_idx * 3 + v]; } vnCount[mm] += 3;
        for (int v = 0; v < 3; v++) { model->meshes[mm].v_normals[vnCount[mm] + v] = attrib.normals[idx1.vn_idx * 3 + v]; } vnCount[mm] += 3;
        for (int v = 0; v < 3; v++) { model->meshes[mm].v_normals[vnCount[mm] + v] = attrib.normals[idx2.vn_idx * 3 + v]; } vnCount[mm] += 3;
      }
    }

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(meshes, meshCount);
    tinyobj_materials_free(materials, materialCount);

    // UnloadFileData(fileText);

    RL_FREE(matFaces);
    RL_FREE(vCount);
    RL_FREE(vtCount);
    RL_FREE(vnCount);
    RL_FREE(faceCount);
  }

  return model;
}

#endif