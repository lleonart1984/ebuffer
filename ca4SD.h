#pragma once

#include "ca4G.h"
#include <sstream>
#include <fstream>
#include <stdio.h>


using namespace std;

typedef struct VERTEX
{
public:
	float3 Position;
	float3 Normal;
	float2 Coordinates;
} VERTEX;

typedef struct MATERIAL
{
public:
	float3 Diffuse;
	float3 Specular;
	float SpecularSharpness;
	int TextureIndex;
	float3 Emissive;
	float4 Roulette; // x-diffuse, y-mirror, z-fresnell, w-reflection index
	MATERIAL()
	{
		Diffuse = float3(1, 1, 1);
		Specular = float3(1, 1, 1);
		SpecularSharpness = 40;
		Roulette = float4(1, 0, 0, 1);
		TextureIndex = -1;
	}
} MATERIAL;

template<typename V, typename M>
class Scene;

template <typename V, typename M>
class Visual
{
private:
	float3 center;
	float4x4 World;

	Buffer* VertexBuffer;
	Buffer* IndicesBuffer;

	int start;
	int count;

	int MaterialIndex;

public:
	inline int getMaterialIndex() const {
		return MaterialIndex;
	}

	inline M* getMaterial() const {
		if (MaterialIndex >= 0)
			return scene->getMaterial(MaterialIndex);
		return NULL;
	}

	const Scene<V, M>* const scene;

	Visual(const Scene<V, M>* scene) :scene(scene) {
	}

	void Load(Buffer* VB, const Buffer* IB, int materialIndex, int start = 0, int count = MAXINT)
	{
		this->VertexBuffer = VB;
		this->IndicesBuffer = IB;
		this->MaterialIndex = materialIndex;
		this->World = I_4x4;
		if (count == MAXINT)
			count = IB->getByteLength() / IB->getStride() - start;
		this->start = start;
		this->count = count;
	}

	void Load(Buffer* VB, int materialIndex, int start = 0, int count = MAXINT)
	{
		this->VertexBuffer = VB;
		this->MaterialIndex = materialIndex;
		this->World = I_4x4;
		if (count == MAXINT)
			count = VB->getByteLength() / VB->getStride() - start;
		this->start = start;
		this->count = count;
	}

	inline float4x4 getWorldTransform() const { return World; }

	void Transform(float4x4 &transform) {
		this->World = mul(this->World, transform);
	}

	inline int getVertexCount() const {
		return VertexBuffer->getByteLength() / VertexBuffer->getStride();
	}

	inline bool IsIndexed() const {
		return IndicesBuffer != nullptr;
	}

	inline int getIndicesCount() const {
		return IndicesBuffer->getByteLength() / IndicesBuffer->getStride();
	}

	inline bool isEmissive() const {
		return any(scene->getMaterial(MaterialIndex)->Emissive);
	}

	inline int getPrimitiveCount() const {
		return (IndicesBuffer != NULL) ? getIndicesCount() / 3 : getVertexCount() / 3;
	}

	inline Buffer* getVertexBuffer() const {
		return VertexBuffer;
	}

	inline Buffer* getIndexBuffer() const {
		return IndicesBuffer;
	}

	void Draw() const
	{
		if (IndicesBuffer != NULL)
			scene->manager->drawer->Primitive(VertexBuffer, IndicesBuffer, start, count);
		else
			scene->manager->drawer->Primitive(VertexBuffer, start, count);
	}
};

class Camera
{
public:
	float3 Position;
	float3 Target;
	float3 Up;
	float FoV;
	float NearPlane;
	float FarPlane;
	void Rotate(float hor, float vert) {
		float3 dir = Target - Position;
		float3x3 rot = ::Rotate(hor, Up);
		dir = mul(dir, rot);
		float3 R = cross(dir, Up);
		if (length(R) > 0)
		{
			rot = ::Rotate(vert, R);
			dir = mul(dir, rot);
			Target = Position + dir;
		}
	}

	void RotateAround(float hor, float vert) {
		float3 dir = (Position - Target);
		float3x3 rot = ::Rotate(hor, float3(0, 1, 0));
		dir = mul(dir, rot);
		Position = Target + dir;
	}
	void MoveForward() {
		float3 dir = Target - Position;
		Target = Target + dir * 0.02;
		Position = Position + dir * 0.02;
	}
	void MoveBackward() {
		float3 dir = Target - Position;
		Target = Target - dir * 0.02;
		Position = Position - dir * 0.02;
	}
	void MoveRight() {
		float3 dir = Target - Position;
		float3 R = cross(Up, dir);

		Target = Target + R * 0.02;
		Position = Position + R * 0.02;
	}
	void MoveLeft() {
		float3 dir = Target - Position;
		float3 R = cross(Up, dir);

		Target = Target - R * 0.02;
		Position = Position - R * 0.02;
	}
	void GetMatrices(int screenWidth, int screenHeight, float4x4 &view, float4x4 &projection)
	{
		view = LookAtRH(Position, Target, Up);
		projection = PerspectiveFovRH(FoV, screenHeight / (float)screenWidth, NearPlane, FarPlane);
	}
};

template <class T>
class List
{
private:
	T* data;
	int capacity;
	int count;
	void GrowArray()
	{
		int newCapacity = 2 * capacity;
		T* newData = new T[newCapacity];
		for (int i = 0; i < count; i++)
			newData[i] = data[i];
		delete[] data;
		data = newData;
		capacity = newCapacity;
	}
public:
	List()
	{
		capacity = 32;
		data = new T[capacity];
		count = 0;
	}
	~List()
	{
		delete[] data;
	}

	inline int getCount() { return count; }

	void Add(T item)
	{
		if (count == capacity)
			GrowArray();

		data[count++] = item;
	}

	T &operator[] (int index)
	{
		return data[index];
	}

	T get(int index) const
	{
		return data[index];
	}

	T* ToArray()
	{
		T *a = new T[this->count];
		for (int i = 0; i < this->count; i++)
			a[i] = data[i];
		return a;
	}
};

class LightSource
{
public:
	float3 Position;
	float3 Intensity;
};

class Tokenizer
{
	char* buffer;
	int count;
	int pos;
public:
	Tokenizer(FILE* stream) {
		fseek(stream, 0, SEEK_END);
		fpos_t count;
		fgetpos(stream, &count);
		fseek(stream, 0, SEEK_SET);
		buffer = new char[count];
		int offset = 0;
		int read;
		do
		{
			read = fread_s(&buffer[offset], count, 1, min(count, 1024 * 1024), stream);
			count -= read;
			offset += read;
		} while (read > 0);
		this->count = offset;
	}
	~Tokenizer() {
		delete[] buffer;
	}
	inline bool isEof() {
		return pos == count;
	}

	inline bool isEol()
	{
		return isEof() || peek() == 10;
	}

	void skipCurrentLine() {
		while (!isEol()) pos++;
		if (!isEof()) pos++;
	}

	inline char peek() {
		return buffer[pos];
	}

	bool match(const char* token)
	{
		int initialPos = pos;
		int p = 0;
		while (!isEol() && token[p] == buffer[pos]) {
			p++; pos++;
		}
		if (token[p] == '\0')
			return true;
		pos = initialPos;
		return false;
	}
	bool matchDigit(int &d) {
		char ch = peek();

		if (ch >= '0' && ch <= '9')
		{
			d = ch - '0';
			pos++;
			return true;
		}
		return false;
	}
	bool matchSymbol(char c)
	{
		if (!isEof() && buffer[pos] == c)
		{
			pos++;
			return true;
		}
		return false;
	}

	string readTextToken() {

		int start = pos;
		while (!isEol() && buffer[pos] != ' ' && buffer[pos] != '/' && buffer[pos] != ';'&& buffer[pos] != ':'&& buffer[pos] != '.'&& buffer[pos] != ','&& buffer[pos] != '('&& buffer[pos] != ')')
			pos++;
		int end = pos - 1;
		return string((char*)(buffer + start), end - start + 1);
	}

	string readToEndOfLine()
	{
		int start = pos;
		while (!isEol())
			pos++;
		int end = pos - 1;
		if (!isEof()) pos++;
		return string((char*)(buffer + start), end - start + 1);
	}

	inline bool endsInteger(char c)
	{
		return (c < '0') || (c > '9');
	}

	void ignoreWhiteSpaces() {
		while (!isEol() && (buffer[pos] == ' ' || buffer[pos] == '\t'))
			pos++;
	}

	bool readIntegerToken(int &i) {
		i = 0;
		if (isEol())
			return false;
		int initialPos = pos;
		ignoreWhiteSpaces();
		int sign = 1;
		if (buffer[pos] == '-')
		{
			sign = -1;
			pos++;
		}
		ignoreWhiteSpaces();
		int end = pos;
		while (pos < count && !endsInteger(buffer[pos])) {
			i = i * 10 + (buffer[pos] - '0');
			pos++;
		}
		i *= sign;

		if (pos > end)
			return true;
		pos = initialPos;
		return false;
	}
	bool readFloatToken(float &f) {
		int initialPos = pos;
		int sign = 1;
		ignoreWhiteSpaces();
		if (buffer[pos] == '-')
		{
			sign = -1;
			pos++;
		}
		int intPart;
		if (readIntegerToken(intPart))
		{
			f = intPart;
			if (pos < count && buffer[pos] == '.')
			{
				int fracPos = pos;
				int fracPart;
				pos++;
				if (!readIntegerToken(fracPart))
				{
					pos = initialPos;
					return false;
				}
				float fracPartf = fracPart / pow(10, pos - fracPos - 1);
				f += fracPartf;
			}
			f *= sign;
			return true;
		}
		pos = initialPos;
		return false;
	}
};

template <typename V, typename M>
class Scene
{
private:
	Camera *camera;
	List<Visual<V, M>*> visuals;
	List<M*> materials;
	List<string> materialNames;
	List<Texture2D*> textures;
	List<string> textureNames;
	LightSource* light;
	float4 backColor;
	Texture2D* skyBox;
	int resolveTexture(string subdir, string fileName) {
		for (int i = 0; i < textures.getCount(); i++)
			if (textureNames[i] == fileName)
				return i;

		string full = subdir;
		full += fileName;
		textureNames.Add(fileName);
		Texture2D* t = manager->loader->Texture(full.c_str());
		textures.Add(t);
		return textures.getCount() - 1;
	}

	void importMtlFile(string subdir, string fileName)
	{
		string file = subdir;
		file += fileName;

		FILE* f;
		if (fopen_s(&f, file.c_str(), "r"))
			return;
		Tokenizer t(f);
		M* activeMaterial = NULL;
		while (!t.isEof())
		{
			if (t.match("newmtl "))
			{
				activeMaterial = new M();
				materials.Add(activeMaterial);
				materialNames.Add(t.readToEndOfLine());
			}
			else
				if (t.match("Kd "))
				{
					float r, g, b;
					t.readFloatToken(r);
					t.readFloatToken(g);
					t.readFloatToken(b);
					activeMaterial->Diffuse = float3(r, g, b);
					t.skipCurrentLine();
				}
				else
					if (t.match("Ks "))
					{
						float r, g, b;
						t.readFloatToken(r);
						t.readFloatToken(g);
						t.readFloatToken(b);
						activeMaterial->Specular = float3(r, g, b);
						t.skipCurrentLine();
					}
					else
						if (t.match("Ns "))
						{
							float power;
							t.readFloatToken(power);
							activeMaterial->SpecularSharpness = power;
							t.skipCurrentLine();
						}
						else
							if (t.match("map_Kd "))
							{
								string textureName = t.readToEndOfLine();
								activeMaterial->TextureIndex = resolveTexture(subdir, textureName);
							}
							else
								t.skipCurrentLine();
		}
		fclose(f);
	}

	int getMaterialIndex(string materialName) {
		for (int i = 0; i < materialNames.getCount(); i++)
			if (materialNames[i] == materialName)
				return i;
		materialName = "";
		return 0;
	}
	void addIndex(List<int> &indices, int index, int pos) {
		if (pos <= 2)
			indices.Add(index - 1);
		else
		{
			indices.Add(indices[indices.getCount() - pos]);
			indices.Add(indices[indices.getCount() + 1 - pos]);
			indices.Add(index - 1);
		}
	}
	void ReadFaceIndices(Tokenizer &t, List<int> &posIndices, List<int> &texIndices, List<int> &norIndices)
	{
		int indexRead = 0;
		int pos = 0;
		int type = 0;

		bool n = false;
		bool p = false;
		bool te = false;
		while (!t.isEol())
		{
			int indexRead;
			if (t.readIntegerToken(indexRead))
			{
				switch (type)
				{
				case 0: addIndex(posIndices, indexRead, pos);
					p = true;
					break;
				case 1: addIndex(texIndices, indexRead, pos);
					te = true;
					break;
				case 2: addIndex(norIndices, indexRead, pos);
					n = true;
					break;
				}
			}
			if (t.isEol())
			{
				if (!n)
					addIndex(norIndices, -1, pos);
				if (!te)
					addIndex(texIndices, -1, pos);
				n = false;
				te = false;
				t.skipCurrentLine();
				return;
			}
			if (t.matchSymbol('/'))
			{
				type++;
			}
			else
				if (t.matchSymbol(' '))
				{
					if (!n)
						addIndex(norIndices, -1, pos);
					if (!te)
						addIndex(texIndices, -1, pos);
					pos++;
					type = 0;
					n = false;
					te = false;
				}
				else
				{
					if (!n)
						addIndex(norIndices, -1, pos);
					if (!te)
						addIndex(texIndices, -1, pos);
					n = false;
					te = false;

					t.skipCurrentLine();
					return;
				}
		}
		if (!n)
			addIndex(norIndices, -1, pos);
		if (!te)
			addIndex(texIndices, -1, pos);
	}

public:
	DeviceManager * const manager;
	Scene(DeviceManager *manager) :manager(manager), camera(new Camera()), visuals(List<Visual<V, M>*>()), light(new LightSource())
	{
		materialNames.Add("default");
		M* defaultMaterial = new M();
		materials.Add(defaultMaterial);
	}
	~Scene()
	{
		delete camera;
		delete light;
		delete materials;
	}
	inline Camera* getCamera() { return camera; }
	inline LightSource* getLight() { return light; }
	inline float4 getBackColor() { return backColor; }
	inline void setBackColor(float4 backColor) { this->backColor = backColor; }
	inline void AddVisual(V* vertexes, int vertexesCount, int* indices, int indicesCount, int materialIndex)
	{
		Visual<V, M>* v = new Visual<V, M>(this);
		Buffer* VB = manager->builder->VertexBuffer(vertexes, vertexesCount);
		Buffer* IB = manager->builder->IndexBuffer(indices, indicesCount);
		v->Load(VB, IB, materialIndex);
		visuals.Add(v);
	}
	inline void AddVisual(V* vertexes, int vertexesCount, int materialIndex)
	{
		Visual<V, M>* v = new Visual<V, M>(this);
		Buffer* VB = manager->builder->VertexBuffer(vertexes, vertexesCount);
		v->Load(VB, materialIndex);
		visuals.Add(v);
	}
	inline Visual<V, M>* getVisual(int index)
	{
		return visuals[index];
	}
	inline int Length()
	{
		return visuals.getCount();
	}
	inline M* getMaterial(int index) const {
		return materials.get(index);
	}
	inline int MaterialCount() { return materials.getCount(); }
	inline Texture2D* getTexture(int index) const {
		if (index == -1) return nullptr;
		return textures.get(index);
	}
	inline int TextureCount() { return textures.getCount(); }
	void LoadObj(const char* filePath, float scale = 1)
	{
		List<float3> positions;
		List<float3> normals;
		List<float2> texcoords;

		List<int> positionIndices;
		List<int> textureIndices;
		List<int> normalIndices;

		string full(filePath);

		string subDir = full.substr(0, full.find_last_of('\\') + 1);
		string name = full.substr(full.find_last_of('\\') + 1);

		FILE* stream;

		errno_t err;
		if (err = fopen_s(&stream, filePath, "r"))
		{
			return;
		}

		List<int> groups;
		List<string> usedMaterials;

		Tokenizer t(stream);
		static int facecount = 0;

		while (!t.isEof())
		{
			if (t.match("v "))
			{
				float3 pos;
				t.readFloatToken(pos.x);
				t.readFloatToken(pos.y);
				t.readFloatToken(pos.z);
				positions.Add(pos * scale);
			}
			else
				if (t.match("vn ")) {
					float3 nor;
					t.readFloatToken(nor.x);
					t.readFloatToken(nor.y);
					t.readFloatToken(nor.z);
					normals.Add(nor);
				}
				else
					if (t.match("vt ")) {
						float2 coord;
						t.readFloatToken(coord.x);
						t.readFloatToken(coord.y);
						texcoords.Add(coord);
					}
					else
						if (t.match("f "))
						{
							facecount++;
							ReadFaceIndices(t, positionIndices, textureIndices, normalIndices);
						}
						else
							if (t.match("usemtl "))
							{
								string materialName = t.readToEndOfLine();
								groups.Add(positionIndices.getCount());
								usedMaterials.Add(materialName);
							}
							else
								if (t.match("mtllib ")) {
									string materialLibName = t.readToEndOfLine();
									importMtlFile(subDir, materialLibName);
								}
								else
									t.skipCurrentLine();
		}
		if (groups.getCount() == 0) // no material used
		{
			usedMaterials.Add("default");
			groups.Add(0);
		}
		groups.Add(positionIndices.getCount());

		fclose(stream);
		V* vertices = new V[positionIndices.getCount()];
		for (int i = 0; i < positionIndices.getCount(); i++)
			vertices[i].Position = positions[positionIndices[i]];
		if (normalIndices.getCount() > 0)
			for (int i = 0; i < positionIndices.getCount(); i++)
				if (normalIndices[i] != -1)
					vertices[i].Normal = normals[normalIndices[i]];
		if (textureIndices.getCount() > 0)
			for (int i = 0; i < positionIndices.getCount(); i++)
				if (textureIndices[i] != -1)
					vertices[i].Coordinates = texcoords[textureIndices[i]];

		auto VB = manager->builder->VertexBuffer(vertices, positionIndices.getCount());
		/*for (int i = 0; i < groups.getCount() - 1; i++)
		{
			int startIndex = groups[i];
			int count = groups[i + 1] - groups[i];

			Visual<V, M>* v = new Visual<V, M>(this);
			v->Load(VB, getMaterialIndex(usedMaterials[i]), startIndex, count);
			visuals.Add(v);
		}
*/
		for (int i = 0; i < groups.getCount() - 1; i++)
		{
			int startIndex = groups[i];
			int count = groups[i + 1] - groups[i];

			AddVisual((V*)(vertices + startIndex), count, getMaterialIndex(usedMaterials[i]));
		}

		delete[] vertices;
	}
	void LoadSkybox(const char* filePath) {
		skyBox = manager->loader->Texture(filePath);
	}
	inline Texture2D* getSkybox() {
		return skyBox;
	}
};

#define SScene Scene<VERTEX, MATERIAL>
#define SVisual Visual<VERTEX, MATERIAL>

template<typename D>
class SceneProcess : public Process<D> {
protected:
	SScene *scene;
public:
	SceneProcess(DeviceManager* manager, D description) :Process<D>(manager, description) {
	}

	virtual void SetScene(SScene *scene) {
		this->scene = scene;
	}
};

struct NoDescription {

};

struct ScreenDescription
{
	int Width;
	int Height;
	ScreenDescription(int width, int height) :Width(width), Height(height) {}
};

struct DebugInfo {
	int FaceIndex;

	int LayerIndex;

	int Level;

	float __rem;
};

struct DebugableProcess {
	DebugInfo Debugging;
	virtual void Boo() const {}
};
class DrawSceneProcess : public SceneProcess<ScreenDescription> {
protected:
	void Initialize() {
		this->DepthBuffer = this->builder->DepthBuffer(Description.Width, Description.Height);
	}
public:
	Texture2D* RenderTarget;
	Texture2D* DepthBuffer;
	DrawSceneProcess(DeviceManager* manager, ScreenDescription description) :SceneProcess<ScreenDescription>(manager, description) {
	}
};