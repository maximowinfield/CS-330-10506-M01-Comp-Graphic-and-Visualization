///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    
#include <cmath>


// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
	// ================================
	// CAMERA CONTROLS (Milestone Three)
	// ================================
	float gYaw = -90.0f;      // looking down -Z by default
	float gPitch = 0.0f;
	float gMouseSensitivity = 0.10f;

	// movement speed (scroll wheel adjusts this)
	float gMoveSpeed = 5.0f;

	// key toggle edge detection (prevents key-hold spamming)
	bool gPKeyWasDown = false;
	bool gOKeyWasDown = false;



}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// receive mouse scroll events (used to adjust movement speed)
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);


	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	if (gFirstMouse)
	{
		gLastX = (float)xMousePos;
		gLastY = (float)yMousePos;
		gFirstMouse = false;
	}

	float xOffset = (float)xMousePos - gLastX;
	float yOffset = gLastY - (float)yMousePos; // reversed since y-coordinates go down

	gLastX = (float)xMousePos;
	gLastY = (float)yMousePos;

	xOffset *= gMouseSensitivity;
	yOffset *= gMouseSensitivity;

	gYaw += xOffset;
	gPitch += yOffset;

	// clamp pitch so we don’t flip the camera upside down
	if (gPitch > 89.0f) gPitch = 89.0f;
	if (gPitch < -89.0f) gPitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(gYaw)) * cos(glm::radians(gPitch));
	front.y = sin(glm::radians(gPitch));
	front.z = sin(glm::radians(gYaw)) * cos(glm::radians(gPitch));

	g_pCamera->Front = glm::normalize(front);
}

void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	// Scroll up => faster, Scroll down => slower
	gMoveSpeed += (float)yOffset * 0.5f;

	// clamp speed so it stays usable
	if (gMoveSpeed < 1.0f) gMoveSpeed = 1.0f;
	if (gMoveSpeed > 20.0f) gMoveSpeed = 20.0f;
}



/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// ================================
	// CAMERA NAVIGATION (WASD + QE)
	// ================================
	float velocity = gMoveSpeed * gDeltaTime;

	glm::vec3 right = glm::normalize(glm::cross(g_pCamera->Front, g_pCamera->Up));

	// forward / backward
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
		g_pCamera->Position += g_pCamera->Front * velocity;
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
		g_pCamera->Position -= g_pCamera->Front * velocity;

	// left / right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
		g_pCamera->Position -= right * velocity;
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
		g_pCamera->Position += right * velocity;

	// up / down
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
		g_pCamera->Position += g_pCamera->Up * velocity;
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
		g_pCamera->Position -= g_pCamera->Up * velocity;

	// ================================
	// PROJECTION TOGGLE (P = perspective, O = orthographic)
	// ================================
	bool pDown = glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS;
	bool oDown = glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS;

	if (oDown && !gOKeyWasDown)
	{
		// switching into ORTHO
		bOrthographicProjection = true;
	}

	if (pDown && !gPKeyWasDown)
	{
		// switching into PERSPECTIVE
		bOrthographicProjection = false;
	}

	gPKeyWasDown = pDown;
	gOKeyWasDown = oDown;

}
glm::vec3 ViewManager::GetCameraPosition() const
{
	if (g_pCamera == NULL)
	{
		return glm::vec3(0.0f, 0.0f, 0.0f);
	}

	return g_pCamera->Position;
}


/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the
	// event queue
	ProcessKeyboardEvents();

	// Keep the camera in the same orientation for BOTH modes
	view = g_pCamera->GetViewMatrix();

	if (bOrthographicProjection == false)
	{
		// Perspective projection (3D view)
		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f,
			100.0f
		);
	}
	else
	{
		// Orthographic projection (2D-like view)
		// IMPORTANT (Rubric): Keep camera orientation the same.
		// Only swap projection type.

		// Ortho "size" controls how zoomed-in the ortho view feels.
		// You can tweak orthoSize if objects look too small/large.
		float orthoSize = 6.0f;

		float aspect = (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT;
		float left = -orthoSize * aspect;
		float right = orthoSize * aspect;
		float bottom = -orthoSize;
		float top = orthoSize;

		projection = glm::ortho(
			left, right,
			bottom, top,
			0.1f, 100.0f
		);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// send view/projection to shader every frame
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
	}
}
