#pragma once

#include "Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <fstream>
#include <shellapi.h>
#include <unordered_map>

namespace std
{
	void hash_combine(size_t& seed, size_t hash)
	{
		hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash;
	}

	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			size_t seed = 0;
			hash<float> hasher;
			hash_combine(seed, hasher(vertex.position.x));
			hash_combine(seed, hasher(vertex.position.y));
			hash_combine(seed, hasher(vertex.position.z));

			hash_combine(seed, hasher(vertex.uv.x));
			hash_combine(seed, hasher(vertex.uv.y));

			return seed;
		}
	};
}

using namespace std;

namespace Utils
{
	HRESULT ParseCommandLine(LPWSTR lpCmdLine, ConfigInfo& config)
	{
		LPWSTR* argv = NULL;
		int argc = 0;

		argv = CommandLineToArgvW(GetCommandLine(), &argc);
		if (argv == NULL)
		{
			MessageBox(NULL, L"Unable to parse command line", L"Error", MB_OK);
			return E_FAIL;
		}

		if (argc > 1)
		{
			char str[256];
			int i = 1;
			while (i < argc)
			{
				wcstombs(str, argv[i], 256);
				i++;

				if (!strcmp(str, "-width"))
				{
					wcstombs(str, argv[i], 256);
					i++;
					config.width = atoi(str);
					continue;
				}

				if (!strcmp(str, "-height"))
				{
					wcstombs(str, argv[i], 256);
					i++;
					config.height = atoi(str);
					continue;
				}

				if (!strcmp(str, "-vsync"))
				{
					wcstombs(str, argv[i], 256);
					i++;
					config.vsync = (atoi(str) > 0);
					continue;
				}

				if (!strcmp(str, "-model"))
				{
					wcstombs(str, argv[i], 256);
					i++;
					config.model = str;
					continue;
				}
			}
		}
		else
		{
			MessageBox(NULL, L"Incorrect command line usage!", L"Error", MB_OK);
			return E_FAIL;
		}

		LocalFree(argv);
		return S_OK;
	}

	void Validate(HRESULT hr, LPWSTR msg)
	{
		if (FAILED(hr))
		{
			MessageBox(NULL, msg, L"Error", MB_OK);
			PostQuitMessage(EXIT_FAILURE);
		}
	}

	void LoadModel(string filepath, Model& model, Material& material)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath.c_str(), "materials\\"))
		{
			throw std::runtime_error(err);
		}

		material.name = materials[0].name;
		material.texturePath = materials[0].diffuse_texname;

		unordered_map<Vertex, uint32_t> uniqueVertices = {};
		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = {};
				vertex.position =
				{
					attrib.vertices[3 * index.vertex_index + 2],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 0]
				};

				vertex.uv =
				{
					attrib.texcoords[2 * index.texcoord_index + 0],
					1 - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
					model.vertices.push_back(vertex);
				}

				model.indices.push_back(uniqueVertices[vertex]);
			}
		}
	}

	void FormatTexture(TextureInfo& info, UINT8* pixels)
	{
		const UINT numPixels = (info.width * info.height);
		const UINT oldStride = info.stride;
		const UINT oldSize = (numPixels * info.stride);

		const UINT newStride = DXGI_FORMAT_R32G32B32A32_SINT;
		const UINT newSize = (numPixels * newStride);
		info.pixels.resize(newSize);

		for (UINT i = 0; i < numPixels; i++)
		{
			info.pixels[i * newStride + 0] = pixels[i * oldStride + 0];
			info.pixels[i * newStride + 1] = pixels[i * oldStride + 1];
			info.pixels[i * newStride + 2] = pixels[i * oldStride + 2];
			info.pixels[i * newStride + 3] = 0xFF;
		}

		info.stride = newStride;
	}

	TextureInfo LoadTexture(string filepath)
	{
		TextureInfo result = {};

		UINT8* pixels = stbi_load(filepath.c_str(), &result.width, &result.height, &result.stride, STBI_default);
		if (!pixels)
		{
			throw runtime_error("Error: failed to load image");
		}

		FormatTexture(result, pixels);

		stbi_image_free(pixels);

		return result;
	}
}