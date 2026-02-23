///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include <iostream>


#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <GL/glew.h>
#include <string>



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
	m_loadedTextures = 0;

}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	DestroyGLTextures();

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
	if (m_loadedTextures >= 16)
	{
		std::cout << "ERROR: Texture limit reached (16). Cannot load: " << filename << std::endl;
		return false;
	}

	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);

	std::cout << "DBG: after stbi_load (" << filename << ")" << std::endl;

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename
			<< ", width:" << width
			<< ", height:" << height
			<< ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		std::cout << "DBG: after glGenTextures" << std::endl;

		glBindTexture(GL_TEXTURE_2D, textureID);
		std::cout << "DBG: after glBindTexture" << std::endl;

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		std::cout << "DBG: after wrap params" << std::endl;

		// set texture filtering parameters (non-mipmap filtering)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		std::cout << "DBG: after filter params" << std::endl;
		// Ensure OpenGL does not assume 4-byte alignment for each row of pixels
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// upload image pixels to GPU
		if (colorChannels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		}
		else if (colorChannels == 4)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		}
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			stbi_image_free(image);
			glBindTexture(GL_TEXTURE_2D, 0);
			return false;
		}

		std::cout << "DBG: after glTexImage2D" << std::endl;
		std::cout << "DBG: glGetError = " << glGetError() << std::endl;

		// IMPORTANT: Disable mipmaps while debugging crashes.
		// Some environments crash if glGenerateMipmap isn't loaded correctly.
		// glGenerateMipmap(GL_TEXTURE_2D);
		std::cout << "DBG: after (skipped) glGenerateMipmap" << std::endl;

		// free the image data from local memory
		stbi_image_free(image);
		std::cout << "DBG: after stbi_image_free" << std::endl;

		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture
		std::cout << "DBG: after glBindTexture(0)" << std::endl;

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		std::cout << "DBG: after registering texture (slot " << (m_loadedTextures - 1)
			<< ", tag " << tag << ")" << std::endl;

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
		if (m_textureIDs[i].ID == 0)
		{
			std::cout << "WARNING: Texture slot " << i << " has ID 0." << std::endl;
			continue;
		}

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
		glDeleteTextures(1, &m_textureIDs[i].ID);
		m_textureIDs[i].ID = 0;
		m_textureIDs[i].tag = "";
	}
	m_loadedTextures = 0;
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

	return bFound;
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
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL == m_pShaderManager)
		return;

	int textureSlot = FindTextureSlot(textureTag);

	// If texture wasn't loaded/found, fall back to solid color mode
	if (textureSlot < 0)
	{
		std::cout << "WARNING: Texture tag not found: " << textureTag << std::endl;
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		return;
	}

	m_pShaderManager->setIntValue(g_UseTextureName, true);
	m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
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

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// =========================================================
	// Load meshes (only once)
	// =========================================================
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();

	// =========================================================
	// Milestone Four: Load textures (free/open-source)
	// Tags are used later in RenderScene()
	// =========================================================
	std::cout << "Loading floor..." << std::endl;
	if (!CreateGLTexture("textures/floor.jpg", "floor"))
	{
		std::cout << "FAILED to load: textures/floor.jpg (tag: floor)" << std::endl;
	}

	std::cout << "Loading wood..." << std::endl;
	if (!CreateGLTexture("textures/wood.jpg", "wood"))
	{
		std::cout << "FAILED to load: textures/wood.jpg (tag: wood)" << std::endl;
	}

	std::cout << "Loading metal..." << std::endl;
	if (!CreateGLTexture("textures/metal.jpg", "metal"))
	{
		std::cout << "FAILED to load: textures/metal.jpg (tag: metal)" << std::endl;
	}

	std::cout << "Loading marble..." << std::endl;
	if (!CreateGLTexture("textures/marble.jpg", "marble"))
	{
		std::cout << "FAILED to load: textures/marble.jpg (tag: marble)" << std::endl;
	}

	std::cout << "Loading brick..." << std::endl;
	if (!CreateGLTexture("textures/brick.jpg", "brick"))
	{
		std::cout << "FAILED to load: textures/brick.jpg (tag: brick)" << std::endl;
	}

	std::cout << "Loading cylinder..." << std::endl;
	if (!CreateGLTexture("textures/cylinder.jpg", "cylinder"))
	{
		std::cout << "FAILED to load: textures/cylinder.jpg (tag: cylinder)" << std::endl;
	}

	std::cout << "Binding textures..." << std::endl;
	BindGLTextures();

	std::cout << "PrepareScene complete." << std::endl;





}

/***********************************************************
 *  SetSceneLights()
 *
 *  Milestone Five: Configure Phong lighting for the scene.
 *  We use two point lights (key + fill) so nothing is fully dark.
 ***********************************************************/
void SceneManager::SetSceneLights(const glm::vec3& cameraPos)
{
	if (NULL == m_pShaderManager)
		return;

	m_pShaderManager->setIntValue(g_UseLightingName, 1);

	// This matches your shader uniform name exactly:
	m_pShaderManager->setVec3Value("viewPosition", cameraPos);

	// Light 0 (key)
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(6.0f, 8.0f, 2.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.20f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.00f, 0.95f, 0.85f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.00f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.0f);

	// Light 1 (fill)
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-6.0f, 6.0f, 6.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.15f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.60f, 0.70f, 1.00f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.70f, 0.80f, 1.00f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 24.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.6f);

	// Disable lights 2 and 3
	for (int i = 2; i < 4; i++)
	{
		std::string idx = std::to_string(i);
		m_pShaderManager->setVec3Value(("lightSources[" + idx + "].ambientColor").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(("lightSources[" + idx + "].diffuseColor").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(("lightSources[" + idx + "].specularColor").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setFloatValue(("lightSources[" + idx + "].specularIntensity").c_str(), 0.0f);
		m_pShaderManager->setFloatValue(("lightSources[" + idx + "].focalStrength").c_str(), 1.0f);
	}
}




/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene(const glm::vec3& cameraPos)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// =========================================================
	// Milestone Five: PLANE uses Phong lighting + texture
	// =========================================================
	SetSceneLights(cameraPos);

	// Floor material: slightly shiny so light reflects off plane
	// (Your shader uses material.shininess as a specular scale)
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 1.0f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(1.0f));
	m_pShaderManager->setFloatValue("material.shininess", 0.35f);

	// ----- Draw Plane -----
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	SetShaderTexture("floor");
	SetTextureUVScale(8.0f, 4.0f);
	m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// Milestone Five: COMPLEX OBJECTS use texture-only shading
	// (Meets rubric: "Don’t worry about reflecting light from the shapes")
	// =========================================================
	

	/****************************************************************/
	// Complex Object: Coffee Mug
	/****************************************************************/

	// Mug Body: Cylinder
	scaleXYZ = glm::vec3(1.2f, 2.2f, 1.2f);
	positionXYZ = glm::vec3(-4.0f, 0.0f, 2.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("cylinder");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Mug Rim: Thin Cylinder
	scaleXYZ = glm::vec3(1.28f, 0.15f, 1.28f);
	positionXYZ = glm::vec3(-4.0f, 2.0f, 2.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Mug Handle: Torus
	scaleXYZ = glm::vec3(0.55f, 0.55f, 0.55f);
	XrotationDegrees = 170.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 25.0f;
	positionXYZ = glm::vec3(-2.8f, 1.4f, 2.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// ================================
	// Object: Notebook (Box)
	// ================================
	m_pShaderManager->setIntValue(g_UseLightingName, 1);
	SetShaderTexture("marble");          // or "brick" if you prefer
	SetTextureUVScale(1.0f, 1.0f);

	// material for notebook (low specular)
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.25f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.15f));
	m_pShaderManager->setFloatValue("material.shininess", 0.10f);

	scaleXYZ = glm::vec3(5.5f, 0.25f, 4.0f);
	positionXYZ = glm::vec3(4.0f, 0.15f, 1.0f);

	SetTransformations(scaleXYZ, 0.0f, -12.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// ================================
	// Object: Ruler (Thin Box)
	// ================================
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);

	// material for ruler (slightly more specular than notebook)
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.20f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.25f));
	m_pShaderManager->setFloatValue("material.shininess", 0.15f);

	scaleXYZ = glm::vec3(7.0f, 0.12f, 1.0f);
	positionXYZ = glm::vec3(4.0f, 0.12f, -2.0f);

	SetTransformations(scaleXYZ, 0.0f, -15.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// ================================
	// Object: Pencil (Cylinder + Cone + Eraser)
	// ================================
	glm::vec3 pencilCenter = glm::vec3(1.8f, 0.25f, -0.2f);
	float pencilYaw = -35.0f;

	// Match SetTransformations order: Rx then Ry
	glm::mat4 rot =
		glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0)) *
		glm::rotate(glm::radians(pencilYaw), glm::vec3(0, 1, 0));

	glm::vec3 pencilDir = glm::normalize(glm::vec3(rot * glm::vec4(0, 1, 0, 0)));

	// Because SNHU cylinder mesh is BASED at y=0 (not centered),
	// "length" along the cylinder axis is just scaleY.
	glm::vec3 bodyScale = glm::vec3(0.18f, 4.8f, 0.18f);
	glm::vec3 tipScale = glm::vec3(0.22f, 0.45f, 0.22f);
	glm::vec3 eraserScale = glm::vec3(0.22f, 0.40f, 0.22f);

	float bodyLen = bodyScale.y;
	float tipLen = tipScale.y;
	float eraserLen = eraserScale.y;

	float gap = 0.01f;       // small gap; set to 0.0f if you want flush
	float overlap = 0.01f;   // small overlap to hide seams

	// Body cylinder base position so that its MIDPOINT is pencilCenter
	glm::vec3 bodyBasePos = pencilCenter - pencilDir * (bodyLen * 0.5f);

	// --- Pencil body (cylinder) ---
	SetShaderTexture("cylinder");
	SetTextureUVScale(1.0f, 1.0f);

	scaleXYZ = bodyScale;
	positionXYZ = bodyBasePos;
	SetTransformations(scaleXYZ, 90.0f, pencilYaw, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Front end of body (where tip starts)
	glm::vec3 bodyFrontPos = bodyBasePos + pencilDir * (bodyLen);

	// --- Pencil tip (cone) ---
	// Cone mesh is also base-at-origin, so place its base at bodyFrontPos
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);

	scaleXYZ = tipScale;
	positionXYZ = bodyFrontPos + pencilDir * gap;
	SetTransformations(scaleXYZ, 90.0f, pencilYaw, 0.0f, positionXYZ);
	m_basicMeshes->DrawConeMesh(true);

	// --- Eraser (small cylinder) ---
	// Place eraser base behind the body base by eraserLen
	SetShaderTexture("brick");
	SetTextureUVScale(1.0f, 1.0f);

	scaleXYZ = eraserScale;
	positionXYZ = bodyBasePos - pencilDir * (eraserLen - overlap);
	SetTransformations(scaleXYZ, 90.0f, pencilYaw, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh(true, true, true);



}


