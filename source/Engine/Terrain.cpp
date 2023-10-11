#include "pch.h"
#include "Terrain.h"
#include <iostream>
//#include "stdafx.h"

Terrain::Terrain()
{
	m_terrainGeneratedToggle = false;
	for (int i = 0; i < 512; i++) perm[i] = p[i & 255]; 
}


Terrain::~Terrain()
{
}

#pragma region Initialisation
bool Terrain::Initialize(ID3D11Device* device, int terrainWidth, int terrainHeight)
{
	int index;
	float height = 0.0;
	bool result;

	// Save the dimensions of the terrain.
	m_terrainWidth = terrainWidth;
	m_terrainHeight = terrainHeight;

	m_frequency = m_terrainWidth / 20;
	m_amplitude = 3.0;
	m_wavelength = 1;

	// Create the structure to hold the terrain data.
	m_heightMap = new HeightMapType[m_terrainWidth * m_terrainHeight];
	if (!m_heightMap)
	{
		return false;
	}

	//this is how we calculate the texture coordinates first calculate the step size there will be between vertices. 
	float textureCoordinatesStep = 5.0f / m_terrainWidth;  //tile 5 times across the terrain. 
	// Initialise the data in the height map (flat).
	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)height;
			m_heightMap[index].z = (float)j;

			//and use this step to calculate the texture coordinates for this point on the terrain.
			m_heightMap[index].u = (float)i * textureCoordinatesStep;
			m_heightMap[index].v = (float)j * textureCoordinatesStep;

		}
	}

	//even though we are generating a flat terrain, we still need to normalise it. 
	// Calculate the normals for the terrain data.
	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	// Initialize the vertex and index buffer that hold the geometry for the terrain.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}


	return true;
}

bool Terrain::CalculateNormals()
{
	int i, j, index1, index2, index3, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	DirectX::SimpleMath::Vector3* normals;


	// Create a temporary array to hold the un-normalized normal vectors.
	normals = new DirectX::SimpleMath::Vector3[(m_terrainHeight - 1) * (m_terrainWidth - 1)];
	if (!normals)
	{
		return false;
	}

	// Go through all the faces in the mesh and calculate their normals.
	for (j = 0; j < (m_terrainHeight - 1); j++)
	{
		for (i = 0; i < (m_terrainWidth - 1); i++)
		{
			index1 = (j * m_terrainHeight) + i;
			index2 = (j * m_terrainHeight) + (i + 1);
			index3 = ((j + 1) * m_terrainHeight) + i;

			// Get three vertices from the face.
			vertex1[0] = m_heightMap[index1].x;
			vertex1[1] = m_heightMap[index1].y;
			vertex1[2] = m_heightMap[index1].z;

			vertex2[0] = m_heightMap[index2].x;
			vertex2[1] = m_heightMap[index2].y;
			vertex2[2] = m_heightMap[index2].z;

			vertex3[0] = m_heightMap[index3].x;
			vertex3[1] = m_heightMap[index3].y;
			vertex3[2] = m_heightMap[index3].z;

			// Calculate the two vectors for this face.
			vector1[0] = vertex1[0] - vertex3[0];
			vector1[1] = vertex1[1] - vertex3[1];
			vector1[2] = vertex1[2] - vertex3[2];
			vector2[0] = vertex3[0] - vertex2[0];
			vector2[1] = vertex3[1] - vertex2[1];
			vector2[2] = vertex3[2] - vertex2[2];

			index = (j * (m_terrainHeight - 1)) + i;

			// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
			normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
		}
	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.
	for (j = 0; j < m_terrainHeight; j++)
	{
		for (i = 0; i < m_terrainWidth; i++)
		{
			// Initialize the sum.
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;

			// Initialize the count.
			count = 0;

			// Bottom left face.
			if (((i - 1) >= 0) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Bottom right face.
			if ((i < (m_terrainWidth - 1)) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper left face.
			if (((i - 1) >= 0) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper right face.
			if ((i < (m_terrainWidth - 1)) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Take the average of the faces touching this vertex.
			sum[0] = (sum[0] / (float)count);
			sum[1] = (sum[1] / (float)count);
			sum[2] = (sum[2] / (float)count);

			// Calculate the length of this normal.
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

			// Get an index to the vertex location in the height map array.
			index = (j * m_terrainHeight) + i;

			// Normalize the final shared normal for this vertex and store it in the height map array.
			m_heightMap[index].nx = (sum[0] / length);
			m_heightMap[index].ny = (sum[1] / length);
			m_heightMap[index].nz = (sum[2] / length);
		}
	}

	// Release the temporary normals.
	delete[] normals;
	normals = 0;

	return true;
}

bool Terrain::InitializeBuffers(ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int index, i, j;
	int index1, index2, index3, index4; //geometric indices. 

	// Calculate the number of vertices in the terrain mesh.
	m_vertexCount = (m_terrainWidth - 1) * (m_terrainHeight - 1) * 6;

	// Set the index count to the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}

	// Initialize the index to the vertex buffer.
	index = 0;
	Triangle tri;
	// Wipe the triangles vector every time the terrain is generated 
	triangles.erase(triangles.begin(), triangles.end());

	for (j = 0; j < (m_terrainHeight - 1); j++)
	{
		for (i = 0; i < (m_terrainWidth - 1); i++)
		{
			index1 = (m_terrainHeight * j) + i;          // Bottom left.
			index2 = (m_terrainHeight * j) + (i + 1);      // Bottom right.
			index3 = (m_terrainHeight * (j + 1)) + i;      // Upper left.
			index4 = (m_terrainHeight * (j + 1)) + (i + 1);  // Upper right.

			// Upper left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index3].u, m_heightMap[index3].v);
			indices[index] = index;
			index++;

			tri.positions[0] = SimpleMath::Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);

			// Upper right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
			indices[index] = index;
			index++;

			tri.positions[1] = SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);

			// Bottom left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
			indices[index] = index;
			index++;

			tri.positions[2] = SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			triangles.push_back(tri);

			// Bottom left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
			indices[index] = index;
			index++;

			tri.positions[0] = SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);

			// Upper right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
			indices[index] = index;
			index++;

			tri.positions[1] = SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);

			// Bottom right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index2].u, m_heightMap[index2].v);
			indices[index] = index;
			index++;

			tri.positions[2] = SimpleMath::Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			triangles.push_back(tri);
		}
	}

	loopIndex = 0;

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}
#pragma endregion

#pragma region Rendering
void Terrain::Render(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);
	deviceContext->DrawIndexed(m_indexCount, 0, 0);

	return;
}

void Terrain::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}
#pragma endregion

void Terrain::Shutdown()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

#pragma region MathMethods
double Terrain::Mix(double a, double b, double t) 
{
	return (1 - t) * a + t * b;
}

double Terrain::Dot(int g[], double x, double y) {
	return g[0] * x + g[1] * y;
}

int Terrain::FastFloor(double x)
{
	return x > 0 ? (int)x : (int)x - 1;
}

float Terrain::RandomFloat(float a, float b) 
{
	float x = (float)(rand() % (int)b) + a;
	return x;
}

float Terrain::Random(SimpleMath::Vector2 st) 
{
	double x0 = st.x * 12.9898;
	double y0 = st.y * 78.233;
	double dot = x0 + y0;
	double s = sin(dot) * 43758.5453123;
	double f = floor(s);

	return (s - f);
}
#pragma endregion


/*
* Unused in this project - intial terrain generation method
*/
bool Terrain::GenerateRandomHeightMap(ID3D11Device* device)
{
	bool result;

	int index;
	float height = { RandomFloat(0.2f, 5.f) };

	m_frequency = (6.283/m_terrainHeight) / m_wavelength; //we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283

	//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
	//in this case I will run a sin-wave through the terrain in one axis.

	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = ((float)rand() / (RAND_MAX)) * m_amplitude;
			m_heightMap[index].z = (float)j;
		}
	}

	Smoothing(device);

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}

bool Terrain::Update()
{
	return true; 
}

#pragma region Getters
float* Terrain::GetWavelength()
{
	return &m_wavelength;
}

float* Terrain::GetAmplitude()
{
	return &m_amplitude;
}
#pragma endregion

#pragma region Smoothing
bool Terrain::Smoothing(ID3D11Device* device)
{
	int index;
	bool result;
	int neighbours[8];

	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			//Top neighbours
			neighbours[0] = (m_terrainHeight * (j + 1)) + (i - 1);
			neighbours[1] = (m_terrainHeight * (j + 1)) + (i);
			neighbours[2] = (m_terrainHeight * (j + 1)) + (i + 1);

			//Side neighbours
			neighbours[3] = (m_terrainHeight * (j)) + (i - 1);
			neighbours[4] = (m_terrainHeight * (j)) + (i + 1);

			//Bottom neighbours
			neighbours[5] = (m_terrainHeight * (j - 1)) + (i - 1);
			neighbours[6] = (m_terrainHeight * (j - 1)) + (i);
			neighbours[7] = (m_terrainHeight * (j - 1)) + (i + 1);

			float sum = m_heightMap[index].y;
			for (int i = 0; i < 8; i++)
			{
				if (neighbours[i] >= 0 && neighbours[i] < m_terrainHeight * m_terrainWidth)
				{
					sum += m_heightMap[neighbours[i]].y;
				}		
				// Add current position's height to the sum
				else
				{
					sum += m_heightMap[index].y;
				}
					
			}

			// Set to average of all neighbour heights
			m_heightMap[index].y = sum / 9.0f;
		}
	}

	result = CalculateNormals(); //Re-evaluate the normals after altering the vertices coordinates
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device); //Re-initialize the buffers after altering the vertices coordinates
	if (!result)
	{
		return false;
	}
}
#pragma endregion

#pragma region GenericNoiseGen
/*
* From https://thebookofshaders.com/13/
*/
float Terrain::Noise(double x, double y)
{
	double i0 = floor(x);
	double i1 = floor(y);

	double f0 = x - i0;
	double f1 = y - i1;

	SimpleMath::Vector2 i = SimpleMath::Vector2(i0, i1);
	SimpleMath::Vector2 f = SimpleMath::Vector2(f0, f1);

	float a = Random(i);
	float b = Random(i + SimpleMath::Vector2(1.0, 0.0));
	float c = Random(i + SimpleMath::Vector2(0.0, 1.0));
	float d = Random(i + SimpleMath::Vector2(1.0, 1.0));

	SimpleMath::Vector2 u = f * f * (SimpleMath::Vector2(3.0, 3.0) - (2.0 * f));

	return Mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}
#pragma endregion

#pragma region SimplexNoise
/*
* From Simplex Noise Demystified (Gustavson, S. 2005) 
* https://weber.itn.liu.se/~stegu/simplexnoise/simplexnoise.pdf
*/
double Terrain::SimplexNoise(double xIn, double yIn)
{
	double n0, n1, n2; // noise contributions from the three corners 

	// skew the input space to determine which simplex cell we are in
	double F2 = 0.5 * (sqrt(3.0) - 1.0);
	double skewFactor = (xIn + yIn) * F2;
	int i = FastFloor(xIn + skewFactor);
	int j = FastFloor(yIn + skewFactor);

	double G2 = (3.0 - sqrt(3.0)) / 6.0;
	double t = (i + j) * G2;
	double X0 = i - t;	// unskew cell origin back to 0
	double Y0 = j - t;
	double x0 = xIn - X0; // the x-y distances from the cell origin 
	double y0 = yIn - Y0;

	// for 2d the simplex shape is an equilateral triangle
	// determine which simplex we are in 
	int i1, j1;		// offsets for second (middle) corner of simplex in (i,j) coords 
	if (x0 > y0)
	{
		i1 = 1;
		j1 = 0;
	}
	else
	{
		i1 = 0;
		j1 = 1;
	}

	// a step of (1,0) in (i,j) means a step of (1-c, c) in (x,y) 
	// a step of (0,1) in (i,j) means a step of (-c, 1-c) in (x,y)
	// where c = (3-sqrt(3))-6

	double x1 = x0 - i1 + G2; //offsets for middle corner in (x,y) unskewed coords 
	double y1 = y0 - j1 + G2;
	double x2 = x0 - 1.0 + 2.0 * G2; // offsets for last corner in (x,y) unskewed coords 
	double y2 = y0 - 1.0 + 2.0 * G2;

	// work out the hashed gradient indices of the three simplex corners 
	int ii = i & 255;
	int jj = j & 255;
	int gi0 = perm[ii + perm[jj] % 12];
	int gi1 = perm[ii + i1 + perm[jj + j1]] % 12;
	int gi2 = perm[ii + 1 + perm[jj + 1]] % 12;

	// calculate contribution from 3 corners 
	double t0 = 0.5 - x0 * x0 - y0 * y0;
	if (t < 0)
	{
		n0 = 0.0;
	}
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * Dot(grad3[gi0], x0, y0);	//(x,y) of grad3 used for 2d gradient
	}

	double t1 = 0.5 - x1 * x1 - y1 * y1;
	if (t1 < 0)
	{
		n1 = 0.0;
	}
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * Dot(grad3[gi1], x1, y1);	//(x,y) of grad3 used for 2d gradient
	}

	double t2 = 0.5 - x2 * x2 - y2 * y2;
	if (t2 < 0)
	{
		n2 = 0.0;
	}
	else
	{
		t2 *= t2;
		n2 = t2 * t2 * Dot(grad3[gi2], x2, y2);	//(x,y) of grad3 used for 2d gradient
	}

	return (n0 + n1 + n2);
}

/*
* Derived from https://www.redblobgames.com/maps/terrain-from-noise/
*/
void Terrain::SimplexNoiseHeightMap()
{
	int index;
	float redist = 4.f;
	float fudgeFactor = 0.9f;

	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			float e = ((float)SimplexNoise((double)i * m_wavelength, (double)j * m_wavelength)
					+ (0.5 * SimplexNoise((double)2 * i * m_wavelength + RandomFloat(5.f, 10.f), (double)2 * j * m_wavelength + RandomFloat(5.f, 10.f))
					+ 0.25 * SimplexNoise((double)4 * i * m_wavelength + RandomFloat(15.f, 25.f), (double)4 * j * m_wavelength + RandomFloat(15.f, 25.f)))
				);

			e = e / 1.75;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)pow(e * fudgeFactor, redist) * m_amplitude;
			m_heightMap[index].z = (float)j;
		}
	}
}

bool Terrain::SimplexNoiseHeightMap(ID3D11Device* device)
{
	bool result;

	SimplexNoiseHeightMap();

	Smoothing(device);
	Smoothing(device);

	result = CalculateNormals(); //Re-evaluate the normals after altering the vertices coordinates
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device); //Re-initialize the buffers after altering the vertices coordinates
	if (!result)
	{
		return false;
	}
}
#pragma endregion

#pragma region fBm
/*
* From https://thebookofshaders.com/13/
*/
#define OCTAVES 6
float Terrain::fBm(double x, double y)
{
	float val = 0.0;
	float amplitude = m_amplitude;
	float frequency = m_frequency;
	float denom = 0.f;

	for (int i = 0; i < OCTAVES; i++)
	{
		val += amplitude * SimplexNoise(x * frequency, y * frequency);
		denom += amplitude;

		frequency *= 2;
		amplitude *= .5;
	}

	float tot = val / denom;

	if (tot < -1)
	{
		tot = -1;
	}
	else if (tot > 1)
	{
		tot = 1;
	}
	return tot;
}

#pragma endregion

#pragma region CollisionDetection
/*
* Method to check for collisions between camera/player and terrain
*/
void Terrain::CheckCollisions(Camera* cam, Player* plr)
{
	SimpleMath::Vector3 v0, v1, v2;
	float t = 0;
	loopIndex = 0;

	// global loop index so the for loop doesn't continuously restart 
	if (loopIndex <= triangles.size())
	{
		// loop through all triangles in the terrain
		for (int i = 0; i < triangles.size(); i++)
		{
			v0 = triangles[i].positions[0];
			v1 = triangles[i].positions[1];
			v2 = triangles[i].positions[2];

			bool hit = m_collisionDetection->RayTriangleIntersect(cam->getPosition(), -DirectX::SimpleMath::Vector3::UnitY, v0, v1, v2, t);

			if (hit)
			{
				loopIndex = 0;

				SimpleMath::Vector3 intersectionPoint = m_collisionDetection->BarycentricCoords(triangles[i], cam->getPosition());

				// check if the position of the camera is within 2.f of it's intersection point
				if (m_collisionDetection->CompareVectorsWithinRadius(cam->getPosition(), intersectionPoint, 2.f))
				{
					// set the position of the camera so that it doesn't go beneath the terrain
					cam->setPosition(SimpleMath::Vector3(
						cam->getPosition().x,
						intersectionPoint.y + 2,
						cam->getPosition().z
					));
					// set grounded to true so that the player can jump
					plr->SetGrounded(true);

					// if the player is currently jumping then set jumping to false as they are 'on' the ground
					if (plr->GetJumping())
					{
						plr->SetJumping(false);
					}

					return;
				}
				else 
				{
					// if not within range then set grounded to false so player cannot jump (as they are already jumping)
					plr->SetGrounded(false);
				}
			}
			// if no hit detected then run another loop
			else if (!hit)
			{
				loopIndex++;
			}
			// reset the loop
			else if (loopIndex >= triangles.size())
			{
				loopIndex = 0;
				return;
			}
		}
	}
}
/*
* Method to detect collisions between a collectable in the scene and the terrain
*/
void Terrain::CheckCollectableCollision(Collectable _coin)
{
	SimpleMath::Vector3 v0, v1, v2;
	float t = 0;

	for (int i = 0; i < triangles.size(); i++)
	{
		v0 = triangles[i].positions[0];
		v1 = triangles[i].positions[1];
		v2 = triangles[i].positions[2];

		bool hit = m_collisionDetection->RayTriangleIntersect(_coin.GetPosition(), -DirectX::SimpleMath::Vector3::UnitY, v0, v1, v2, t);
		// detect upwards as well as coins may go beneath the terrain upon generation
		bool hitUp = m_collisionDetection->RayTriangleIntersect(_coin.GetPosition(), SimpleMath::Vector3::UnitY, v0, v1, v2, t);

		if (hit || hitUp)
		{
			SimpleMath::Vector3 intersectionPoint = m_collisionDetection->BarycentricCoords(triangles[i], _coin.GetPosition());
			
			// set the position of the coin to slightly above the terrain
			_coin.SetPosition(
				SimpleMath::Vector3(
					intersectionPoint.x,
					intersectionPoint.y + 3.f,
					intersectionPoint.z
				)
			);

			return;
		}
	}
}
#pragma endregion
