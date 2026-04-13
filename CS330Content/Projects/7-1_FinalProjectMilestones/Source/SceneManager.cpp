///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
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

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	// All texture dimensions must be divisible by 4 or this will crash when generating the mipmaps for the texture.  
	// This is a requirement of OpenGL.
	if (!CreateGLTexture("./Utilities/textures/wood.jpg", "wood"))
		std::cout << "Failed: wood\n";

	if (!CreateGLTexture("./Utilities/textures/glass_1.jpg", "glass_1"))
		std::cout << "Failed: glass_1\n";

	if (!CreateGLTexture("./Utilities/textures/glass_2.jpg", "glass_2"))
		std::cout << "Failed: glass_2\n";

	if (!CreateGLTexture("./Utilities/textures/glass_3.jpg", "glass_3"))
		std::cout << "Failed: glass_3\n";

	if (!CreateGLTexture("./Utilities/textures/compass.jpg", "compass"))
		std::cout << "Failed: compass\n";

	if (!CreateGLTexture("./Utilities/textures/pavers.jpg", "pavers"))
		std::cout << "Failed: pavers\n";

	if (!CreateGLTexture("./Utilities/textures/brick.jpg", "brick"))
		std::cout << "Failed: brick\n";

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.ambientColor = glm::vec3(0.35f, 0.35f, 0.35f);
	goldMaterial.diffuseColor = glm::vec3(0.75f, 0.75f, 0.75f);
	goldMaterial.specularColor = glm::vec3(0.90f, 0.90f, 0.90f);
	goldMaterial.shininess = 22.0f;
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.ambientColor = glm::vec3(0.20f, 0.20f, 0.20f);
	cementMaterial.diffuseColor = glm::vec3(0.55f, 0.55f, 0.55f);
	cementMaterial.specularColor = glm::vec3(0.30f, 0.30f, 0.30f);
	cementMaterial.shininess = 0.5f;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
	woodMaterial.diffuseColor = glm::vec3(0.50f, 0.50f, 0.50f);
	woodMaterial.specularColor = glm::vec3(0.20f, 0.20f, 0.20f);
	woodMaterial.shininess = 0.3f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.ambientColor = glm::vec3(0.30f, 0.30f, 0.30f);
	tileMaterial.diffuseColor = glm::vec3(0.65f, 0.65f, 0.65f);
	tileMaterial.specularColor = glm::vec3(0.80f, 0.80f, 0.80f);
	tileMaterial.shininess = 25.0f;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientStrength = 0.15f;
	glassMaterial.ambientColor = glm::vec3(0.20f, 0.20f, 0.20f);
	glassMaterial.diffuseColor = glm::vec3(0.85f, 0.85f, 0.85f);
	glassMaterial.specularColor = glm::vec3(1.00f, 1.00f, 1.00f);
	glassMaterial.shininess = 85.0f;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
	clayMaterial.diffuseColor = glm::vec3(0.55f, 0.55f, 0.55f);
	clayMaterial.specularColor = glm::vec3(0.25f, 0.25f, 0.25f);
	clayMaterial.shininess = 0.5f;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);

	OBJECT_MATERIAL steelMaterial;
	steelMaterial.ambientStrength = 0.25f;
	steelMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.30f);
	steelMaterial.diffuseColor = glm::vec3(0.55f, 0.57f, 0.60f);
	steelMaterial.specularColor = glm::vec3(0.90f, 0.90f, 0.95f);
	steelMaterial.shininess = 120.0f;
	steelMaterial.tag = "steel";
	m_objectMaterials.push_back(steelMaterial);

	

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

	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);

	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.5f);

	m_pShaderManager->setBoolValue("bUseLighting", true);

	

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
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 * 
 * Normalized Colors to use below:

	Floor
	0.71f, 0.46f, 0.29f, 1.0f

	Left Wall
	0.32f, 0.31f, 0.33f, 1.0f

	Right Wall
	0.29f, 0.22f, 0.19f, 1.0f

	Paper Towel Roll
	0.47f, 0.40f, 0.27f, 1.0f

	Cube
	0.43f, 0.33f, 0.19f, 1.0f

	Sphere
	0.35f, 0.37f, 0.45f, 1.0f

	Glass
	0.77f, 0.72f, 0.77f, 1.0f

	Cone
	0.33f, 0.31f, 0.31f, 1.0f
 ***********************************************************/
void SceneManager::RenderScene()
{	
	// This is used instead of the color value when a texture is being mapped to a shape.
	//  The shader will ignore the color value and use the texture instead when this is used.
	glm::vec4 use_texture_instead = glm::vec4(1.0f);

	// Draw Floor
	glm::vec3 floor_scaleXYZ = glm::vec3(10.0f, 1.0f, 10.0f);
	glm::vec3 floor_rotationXYZ = glm::vec3(0.0f, 45.0f, 0.0f);
	glm::vec3 floor_positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec4 floor_colorRGBA = use_texture_instead;
	std::string floor_texture_tag = "wood";
	std::string floor_material_tag = "wood";
	DrawShape("Box",
		floor_rotationXYZ,
		floor_scaleXYZ,
		floor_positionXYZ,
		floor_colorRGBA,
		floor_texture_tag,
		floor_material_tag
		);

	/************************
	// START - Draw WineGlass
	*************************/
	// variables used for all the parts of the wine glass
	glm::vec4 glass_color = use_texture_instead;
	float glass_x_pos = 1.2f;
	float glass_z_pos = 5.0f;
	float glass_base_y_pos = 0.5f; // This may come in handy for positioning other shapes
	std::string glass_material_tag = "glass";

	// Rim (tapered cylinder)
	glm::vec3 rim_scaleXYZ = glm::vec3(0.385f, 0.8f, 0.4f);
	glm::vec3 rim_rotationXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 rim_positionXYZ = glm::vec3(glass_x_pos, glass_base_y_pos + 1.0f, glass_z_pos);
	std::string rim_texture_tag = "glass_1";
	DrawShape("Tapered_Cylinder",
		rim_rotationXYZ,
		rim_scaleXYZ,
		rim_positionXYZ,
		glass_color,
		rim_texture_tag,
		glass_material_tag);

	// Cup (sphere)
	glm::vec3 cup_scaleXYZ = glm::vec3(0.4f, 0.4f, 0.4f);
	glm::vec3 cup_rotationXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cup_positionXYZ = glm::vec3(glass_x_pos, glass_base_y_pos + 0.9f, glass_z_pos);
	std::string cup_texture_tag = "glass_3";
	DrawShape("Sphere",
		cup_rotationXYZ,
		cup_scaleXYZ,
		cup_positionXYZ,
		glass_color,
		cup_texture_tag,
		glass_material_tag);

	// Stem (cylinder)
	glm::vec3 stem_scaleXYZ = glm::vec3(0.1f, 0.5f, 0.1f);
	glm::vec3 stem_rotationXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 stem_positionXYZ = glm::vec3(glass_x_pos, glass_base_y_pos + 0.1f, glass_z_pos);
	std::string stem_texture_tag = "glass_2";
	DrawShape("Cylinder",
		stem_rotationXYZ,
		stem_scaleXYZ,
		stem_positionXYZ,
		glass_color,
		stem_texture_tag,
		glass_material_tag);

	// Base (flat cylinder)
	glm::vec3 base_scaleXYZ = glm::vec3(0.5f, 0.1f, 0.5f);
	glm::vec3 base_rotationXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 base_positionXYZ = glm::vec3(glass_x_pos, glass_base_y_pos, glass_z_pos);
	std::string base_texture_tag = "glass_3";
	DrawShape("Cylinder",
		base_rotationXYZ,
		base_scaleXYZ,
		base_positionXYZ,
		glass_color,
		base_texture_tag,
		glass_material_tag);

	/************************
	// End - Draw WineGlass
	*************************/

	// Draw Cube
	glm::vec3 cube_scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);
	glm::vec3 cube_rotationXYZ = glm::vec3(0.0f, 45.0f, 0.0f);
	glm::vec3 cube_positionXYZ = glm::vec3(0.5f, 1.0f, 3.0f);
	glm::vec4 cube_colorRGBA = use_texture_instead;
	std::string cube_texture_tag = "compass";
	std::string cube_material_tag = "clay";
	DrawShape("Box",
		cube_rotationXYZ,
		cube_scaleXYZ,
		cube_positionXYZ,
		cube_colorRGBA,
		cube_texture_tag,
		cube_material_tag
	);

	// Draw Sphere
	glm::vec3 sphere_scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 sphere_rotationXYZ = glm::vec3(0.0f, 45.0f, 0.0f);
	glm::vec3 sphere_positionXYZ = glm::vec3(0.5f, 2.6f, 3.0f);
	glm::vec4 sphere_colorRGBA = glm::vec4(0.35f, 0.37f, 0.45f, 1.0f);
	std::string sphere_texture_tag = "";
	std::string sphere_material_tag = "steel";
	DrawShape("Sphere",
		sphere_rotationXYZ,
		sphere_scaleXYZ,
		sphere_positionXYZ,
		sphere_colorRGBA,
		sphere_texture_tag,
		sphere_material_tag
	);

	// Draw Towel Roll
	glm::vec3 towellroll_scaleXYZ = glm::vec3(0.5f, 2.5f, 0.5f);
	glm::vec3 towellroll_rotationXYZ = glm::vec3(0.0f, 45.0f, 0.0f);
	glm::vec3 towellroll_positionXYZ = glm::vec3(-1.5f, 0.5f, 3.0f);
	glm::vec4 towellroll_colorRGBA = glm::vec4(0.47f, 0.40f, 0.27f, 1.0f);
	std::string towellroll_texture_tag = "";
	std::string towellroll_material_tag = "wood";
	DrawShape("Cylinder",
		towellroll_rotationXYZ,
		towellroll_scaleXYZ,
		towellroll_positionXYZ,
		towellroll_colorRGBA,
		towellroll_texture_tag,
		towellroll_material_tag
	);
	
	// Draw Cone
	glm::vec3 cone_scaleXYZ = glm::vec3(0.7f, 2.5f, 0.7f);
	glm::vec3 cone_rotationXYZ = glm::vec3(0.0f, 45.0f, 0.0f);
	glm::vec3 cone_positionXYZ = glm::vec3(2.5f, 0.6f, 2.5f);
	glm::vec4 cone_colorRGBA = glm::vec4(0.33f, 0.31f, 0.31f, 1.0f);
	std::string cone_texture_tag = "";
	std::string cone_material_tag = "steel";
	DrawShape("Cone",
		cone_rotationXYZ,
		cone_scaleXYZ,
		cone_positionXYZ,
		cone_colorRGBA,
		cone_texture_tag,
		cone_material_tag
	);

	// Draw Left Wall
	glm::vec3 leftwall_scaleXYZ = glm::vec3(13.0f, 5.0f, 2.0f);
	glm::vec3 leftwall_rotationXYZ = glm::vec3(0.0f, 45.0f, 0.0f);
	glm::vec3 leftwall_positionXYZ = glm::vec3(-2.5f, 3.0f, -4.9f);
	glm::vec4 leftwall_colorRGBA = use_texture_instead;
	std::string leftwall_texture_tag = "pavers";
	std::string leftwall_material_tag = "cement";
	DrawShape("Box",
		leftwall_rotationXYZ,
		leftwall_scaleXYZ,
		leftwall_positionXYZ,
		leftwall_colorRGBA,
		leftwall_texture_tag,
		leftwall_material_tag
	);

	// Draw Right Wall
	glm::vec3 rightwall_scaleXYZ = glm::vec3(13.0f, 5.0f, 2.0f);
	glm::vec3 rightwall_rotationXYZ = glm::vec3(0.0f, -45.0f, 0.0f);
	glm::vec3 rightwall_positionXYZ = glm::vec3(2.5f, 3.0f, -4.9f);
	glm::vec4 rightwall_colorRGBA = use_texture_instead;
	std::string rightwall_texture_tag = "brick";
	std::string rightwall_material_tag = "cement";
	DrawShape("Box",
		rightwall_rotationXYZ,
		rightwall_scaleXYZ,
		rightwall_positionXYZ,
		rightwall_colorRGBA,
		rightwall_texture_tag,
		rightwall_material_tag
	);
}

void SceneManager::DrawShape(
	std::string shape_type,
	glm::vec3 rotationXYZ,
	glm::vec3 scaleXYZ,
	glm::vec3 positionXYZ,
	glm::vec4 colorRGBA,
	std::string textureTag,
	std::string materialTag
	)
{
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		rotationXYZ.x,
		rotationXYZ.y,
		rotationXYZ.z,
		positionXYZ);

	// set the color into memory to be used on the drawn meshes
	// if a texture tag is provided, then set the texture into memory to be used on the drawn meshes
	if (textureTag.compare("") != 0)
	{
		SetShaderTexture(textureTag);
	}
	else {
		SetShaderColor(colorRGBA.r, colorRGBA.g, colorRGBA.b, colorRGBA.a);
	}

	SetShaderMaterial(materialTag);

	// draw the mesh with transformation values
	if (shape_type == "Box") {
		m_basicMeshes->DrawBoxMesh();
	}
	else if (shape_type == "Plane") {
		m_basicMeshes->DrawPlaneMesh();
	}
	else if (shape_type == "Sphere") {
		m_basicMeshes->DrawSphereMesh();
	}
	else if (shape_type == "Cylinder") {
		m_basicMeshes->DrawCylinderMesh();
	}
	else if (shape_type == "Tapered_Cylinder") {
		m_basicMeshes->DrawTaperedCylinderMesh();
	}
	else if (shape_type == "Cone") {
		m_basicMeshes->DrawConeMesh();
	}
}
