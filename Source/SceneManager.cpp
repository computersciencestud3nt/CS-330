///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>


// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_TextureOverlayValueName = "overlayTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
	const char* g_UseTextureOverlayName = "bUseTextureOverlay";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}
/***********************************************************
 *  SetShaderTextureOverlay()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTextureOverlay(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		if (textureTag.size() > 0)
		{
			m_pShaderManager->setIntValue(g_UseTextureOverlayName, true);

			int textureID = -1;
			textureID = FindTextureSlot(textureTag);
			m_pShaderManager->setSampler2DValue(g_TextureOverlayValueName, textureID);
		}
		else
		{
			m_pShaderManager->setIntValue(g_UseTextureOverlayName, false);
		}
	}
}
/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/wood.jpg",
		"wood");

	bReturn = CreateGLTexture(
		"textures/metal.jpg",
		"metal");

	bReturn = CreateGLTexture(
		"textures/rock.jpg",
		"rock");

	bReturn = CreateGLTexture(
		"textures/GreenPlastic.jpg",
		"GreenPlastic");

	bReturn = CreateGLTexture(
		"textures/gold.jpg",
		"gold");

	bReturn = CreateGLTexture(
		"textures/black.jpg",
		"black");
	
	bReturn = CreateGLTexture(
		"textures/blue.jpg",
		"blue");



	

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}
/***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// Materials used for objects
	// Metal material so that the lioght reflects
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	metalMaterial.shininess = 42.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	// Wood material so that it absorbs light
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.45f, 0.30f, 0.15f);
	woodMaterial.specularColor = glm::vec3(0.08f, 0.08f, 0.08f);
	woodMaterial.shininess = 8.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	// Rock Material
	OBJECT_MATERIAL rockMaterial;

	rockMaterial.diffuseColor = glm::vec3(0.35f, 0.33f, 0.30f);   // earthy gray-brown
	rockMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);   
	rockMaterial.shininess = 4.0f;                             // rough surface
	rockMaterial.tag = "rock";

	m_objectMaterials.push_back(rockMaterial);

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	plasticMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	plasticMaterial.shininess = 64.0f;
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL goldMaterial;

	goldMaterial.diffuseColor = glm::vec3(0.75f, 0.60f, 0.15f);   // warm base
	goldMaterial.specularColor = glm::vec3(1.0f, 0.85f, 0.30f);    // yellowish highlight
	goldMaterial.shininess = 96.0f;                            // very shiny
	goldMaterial.tag = "gold";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL blackMatte;

	blackMatte.diffuseColor = glm::vec3(0.08f, 0.08f, 0.09f);   // very dark base
	blackMatte.specularColor = glm::vec3(0.2f, 0.2f, 0.25f);
	blackMatte.shininess = 24.0f;                          // semi-gloss
	blackMatte.tag = "blackMatte";

	m_objectMaterials.push_back(blackMatte);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// directional light 
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.3f, -1.0f, -0.2f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 1 to make the scene brighter and add some white light reflections on the objects
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 10.0f, 3.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", .5f, .5f, .5f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// point light 2 - warm sunlight tone
	m_pShaderManager->setVec3Value("pointLights[1].position", -3.0f, 7.0f, 3.0f);
	// Soft warm ambient fill
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.2f, 0.17f, 0.1f);
	// Strong warm diffuse (sunlight feel)
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 1.0f, 0.85f, 0.6f);
	// Warm specular highlights
	m_pShaderManager->setVec3Value("pointLights[1].specular", 1.0f, 0.9f, 0.7f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);


}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadPyramid3Mesh();
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Ground plane
	scaleXYZ = glm::vec3(40.0f, 1.0f, 40.0f);  // bigger travel area
	positionXYZ = glm::vec3(0.0f, -0.01f, 0.0f); // tiny drop to prevent Z-fighting

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);
	// Material for the ground plane so that it reflects light
	SetShaderTexture("rock");
	// Set the UV scale to tile the texture along the length of the bands
	SetShaderMaterial("rock");
	m_basicMeshes->DrawPlaneMesh();

	// Table
	DrawTable(glm::vec3(0.0f, 0.0f, 0.0f));
 
	// Treasure Chest (Milestone Two)
	DrawTreasureChest(glm::vec3(0.0f, 4.0f, 0.0f));

	// Draw the essentials kit box 
	glm::vec3 kitPos(2.5f, 4.2f, 2.0f);
	DrawEssentialsKitBox(kitPos);

	// Put DND in front of the box 
	glm::vec3 dndPos = kitPos + glm::vec3(-0.9f, 0.25f, 0.95f);
	DrawDND(dndPos);
	// Objects on the table
	DrawTapeMeasure(glm::vec3(-1.0f, 4.2f, 1.5f));
	DrawBlueD6(glm::vec3(-2.3f, 4.2f, 2.2f));
	DrawCoin(glm::vec3(-1.9f, 4.2f, 2.8f));
	DrawGreenDie(glm::vec3(1.7f, 4.2f, 2.0f));
}

void SceneManager::DrawTable(const glm::vec3& pos)
{
	SetShaderTexture("wood");
	SetShaderMaterial("wood");

	// TABLE TOP 
	glm::vec3 topScale(12.0f, 0.5f, 8.0f);
	glm::vec3 topPos = pos + glm::vec3(0.0f, 4.0f, 0.0f);

	SetTransformations(topScale, 0.0f, 0.0f, 0.0f, topPos);
	m_basicMeshes->DrawBoxMesh();

	// LEGS 
	glm::vec3 legScale(0.6f, 4.0f, 0.6f);

	float xOffset = 5.0f;
	float zOffset = 3.0f;

	glm::vec3 leg1 = pos + glm::vec3(-xOffset, 2.0f, -zOffset);
	glm::vec3 leg2 = pos + glm::vec3(xOffset, 2.0f, -zOffset);
	glm::vec3 leg3 = pos + glm::vec3(-xOffset, 2.0f, zOffset);
	glm::vec3 leg4 = pos + glm::vec3(xOffset, 2.0f, zOffset);

	SetTransformations(legScale, 0.0f, 0.0f, 0.0f, leg1);
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(legScale, 0.0f, 0.0f, 0.0f, leg2);
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(legScale, 0.0f, 0.0f, 0.0f, leg3);
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(legScale, 0.0f, 0.0f, 0.0f, leg4);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawTreasureChest(const glm::vec3& chestPos)
{
	DrawChestBase(chestPos);
	DrawChestLid(chestPos);
	DrawChestBands(chestPos);
	DrawChestLock(chestPos);
	DrawChestHinges(chestPos);
}

void SceneManager::DrawChestBase(const glm::vec3& chestPos)
{
	SetShaderTexture("wood");
	SetTextureUVScale(3.0f, 1.0f);   

	glm::vec3 scaleXYZ(5.0f, 1.3f, 1.5f);
	glm::vec3 positionXYZ = chestPos + glm::vec3(0.0f, 0.75f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();
}


void SceneManager::DrawChestLid(const glm::vec3& chestPos)
{
	SetShaderTexture("wood");
	SetTextureUVScale(3.0f, 1.0f);

	glm::vec3 scaleXYZ(0.83f, 5.0f, 1.05f);
	glm::vec3 positionXYZ = chestPos + glm::vec3(-2.5f, 1.75f, 0.0f);

	SetTransformations(scaleXYZ, 90.0f, 90.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();
}


void SceneManager::DrawChestBands(const glm::vec3& chestPos)
{
	SetShaderTexture("metal");
	// UV scale to tile the texture along the length of the bands
	SetShaderMaterial("metal");
	SetTextureUVScale(6.0f, 1.0f);

	glm::vec3 scaleXYZ(1.6f, 0.12f, 1.7f);

	float bandY = 2.0f;
	float bandZ = 0.0f;

	glm::vec3 pos1 = chestPos + glm::vec3(-0.9f, bandY, bandZ);
	glm::vec3 pos2 = chestPos + glm::vec3(0.0f, bandY, bandZ);
	glm::vec3 pos3 = chestPos + glm::vec3(0.9f, bandY, bandZ);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, pos1);
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, pos2);
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, pos3);
	m_basicMeshes->DrawBoxMesh();
}




void SceneManager::DrawChestLock(const glm::vec3& chestPos)
{
	SetShaderTexture("metal");
	// UV scale to tile the texture along the length of the lock
	SetShaderMaterial("metal");
	SetTextureUVScale(2.0f, 2.0f);

	glm::vec3 scaleXYZ(0.35f, 0.35f, 0.10f);
	glm::vec3 positionXYZ = chestPos + glm::vec3(0.0f, 0.75f, 0.80f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawChestHinges(const glm::vec3& chestPos)
{
	SetShaderTexture("metal");
	// UV scale
	SetShaderMaterial("metal");
	SetTextureUVScale(4.0f, 1.0f);

	glm::vec3 scaleXYZ(0.15f, 0.15f, 0.40f);

	glm::vec3 pos1 = chestPos + glm::vec3(-0.8f, 1.15f, -0.80f);
	glm::vec3 pos2 = chestPos + glm::vec3(0.8f, 1.15f, -0.80f);

	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, pos1);
	m_basicMeshes->DrawCylinderMesh();

	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, pos2);
	m_basicMeshes->DrawCylinderMesh();
}
void SceneManager::DrawEssentialsKitBox(const glm::vec3& pos)
{
	// Cardboard
	SetShaderColor(0.08f, 0.10f, 0.14f, 1.0f);  // dark navy
	SetShaderTexture("blue");                  // rough-ish 

	glm::vec3 scaleXYZ(2.4f, 1.2f, 1.6f);
	glm::vec3 positionXYZ = pos + glm::vec3(0.0f, 0.60f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();
}
void SceneManager::DrawLetterD(const glm::vec3& pos)
{
	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("plastic");

	

	// Vertical bar
	SetTransformations(glm::vec3(0.12f, 0.80f, 0.12f),
		0.0f, 0.0f, 0.0f,
		pos + glm::vec3(0.0f, 0.40f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	// Top bar
	SetTransformations(glm::vec3(0.40f, 0.12f, 0.12f),
		0.0f, 0.0f, 0.0f,
		pos + glm::vec3(0.20f, 0.80f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	// Bottom bar
	SetTransformations(glm::vec3(0.40f, 0.12f, 0.12f),
		0.0f, 0.0f, 0.0f,
		pos + glm::vec3(0.20f, 0.00f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	// Right side
	SetTransformations(glm::vec3(0.12f, 0.56f, 0.12f),
		0.0f, 0.0f, 0.0f,
		pos + glm::vec3(0.40f, 0.40f, 0.0f));
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawLetterN(const glm::vec3& pos)
{
	SetShaderColor(0.8f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("plastic");


	// Left vertical
	SetTransformations(glm::vec3(0.12f, 0.80f, 0.12f),
		0.0f, 0.0f, 0.0f,
		pos + glm::vec3(0.0f, 0.40f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	// Right vertical
	SetTransformations(glm::vec3(0.12f, 0.80f, 0.12f),
		0.0f, 0.0f, 0.0f,
		pos + glm::vec3(0.40f, 0.40f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	// Diagonal 
	SetTransformations(glm::vec3(0.12f, 0.92f, 0.12f),
		0.0f, 0.0f, 25.0f,
		pos + glm::vec3(0.20f, 0.40f, 0.0f));
	m_basicMeshes->DrawBoxMesh();
}
void SceneManager::DrawDND(const glm::vec3& pos)
{
	DrawLetterD(pos);
	DrawLetterN(pos + glm::vec3(0.55f, 0.0f, 0.0f));
	DrawLetterD(pos + glm::vec3(1.10f, 0.0f, 0.0f));
}
void SceneManager::DrawTapeMeasure(const glm::vec3& pos)
{
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
	SetShaderTexture("black");
	SetShaderMaterial("blackMatte");                  

	// Dimensions for the tape measure body
	glm::vec3 bodyScale(0.5f, 0.5f, 0.5f);
	glm::vec3 bodyPos = pos + glm::vec3(0.0f, 0.55f, 0.0f);

	// CylinderMesh
	SetTransformations(bodyScale, 90.0f, 0.0f, 0.0f, bodyPos);
	m_basicMeshes->DrawCylinderMesh();

	// Yellow side panel
	SetShaderColor(0.95f, 0.80f, 0.15f, 1.0f);
	SetShaderMaterial("plastic");

	glm::vec3 sideScale(0.85f, 0.85f, 0.48f);   // smaller + thinner
	glm::vec3 sidePos = bodyPos + glm::vec3(0.0f, 0.0f, 0.25f);

	SetTransformations(sideScale, 0.0f, 0.0f, 0.0f, sidePos);
	m_basicMeshes->DrawBoxMesh();

	// Small “tape tab” sticking out
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
	SetShaderMaterial("metal");

	glm::vec3 tabScale(0.15f, 0.05f, 0.5f);
	glm::vec3 tabPos = bodyPos + glm::vec3(-0.55f, -0.25f, 0.0f);

	SetTransformations(tabScale, 0.0f, 0.0f, 0.0f, tabPos);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawBlueD6(const glm::vec3& pos)
{
	SetShaderColor(0.08f, 0.20f, 0.85f, 1.0f);  // blue
	SetShaderMaterial("metal");                 // gives nicer highlights

	glm::vec3 scaleXYZ(0.35f, 0.35f, 0.35f);
	glm::vec3 positionXYZ = pos + glm::vec3(-0.2f, 0.25f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 25.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawCoin(const glm::vec3& pos)
{
	SetShaderColor(0.85f, 0.70f, 0.20f, 1.0f);  // gold-ish
	SetShaderTexture("gold");
	SetShaderMaterial("gold");


	// Thin cylinder coin
	glm::vec3 scaleXYZ(0.45f, 0.05f, 0.45f);
	glm::vec3 positionXYZ = pos + glm::vec3(0.0f, 0.06f, 0.0f);   

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();
} 

void SceneManager::DrawGreenDie(const glm::vec3& pos)
{
	SetShaderColor(1.0f, 1.0f, 0.0f, 1.0f);
	SetShaderTexture("GreenPlastic");   // texture
	SetShaderMaterial("plastic");       // lighting properties

	// Slightly wider so it reads clearly as a D4
	glm::vec3 scaleXYZ(0.40f, 0.4f, 0.4f);

	// Lift by half the Y scale so it rests on ground
	glm::vec3 positionXYZ = pos + glm::vec3(0.0f, 1.4f, 0.0f);

	// Rotate so it doesn't look perfectly symmetrical
	SetTransformations(scaleXYZ, 0.0f, 43.0f, 0.0f, positionXYZ);

	m_basicMeshes->DrawPyramid3Mesh();

}
