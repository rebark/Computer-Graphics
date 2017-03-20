// Std. Includes
#include <string>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GL includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Other Libs
#include <SOIL.h>

// Properties
GLuint screenWidth = 1280, screenHeight = 720;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();
void set_lights(Shader &shader);
void RenderScene(Shader &shader);
void RenderQuad();

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

// Light attributes
glm::vec3 lightPos (0.5f, 25.0f, -0.8f);
glm::vec3 lampPos(0.0f, 0.5f, -4.5f);
glm::vec3 spotPos(0.5f, 1.0f, 2.0f);
bool point = true;
bool spot = true;

//Delta time
GLfloat deltaTime = 0.0f;   // Time between current frame and last frame
GLfloat lastFrame = 0.0f;	// Time of last frame

// The MAIN function, from here we start our application and run our Game loop
int main()
{
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Magics", nullptr, nullptr); // Windowed
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Options
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize GLEW to setup the OpenGL Function pointers
	glewExperimental = GL_TRUE;
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, screenWidth, screenHeight);

	// Setup some OpenGL options
	glEnable(GL_DEPTH_TEST);

	// Setup and compile our shaders
	Shader shader("shaders/standard_shader.vs", "shaders/standard_shader.fs");
	Shader lightShader("light.vs", "light.fs");
	Shader simpleDepthShader("shaders/shadow_mapping_depth.vs", "shaders/shadow_mapping_depth.fs");
	Shader debugDepthQuad("shaders/debug_quad.vs", "shaders/debug_quad_depth.fs");

	// Load models
	Model ourModel("nope/nope.obj");

	//light
	// Set up vertex data (and buffer(s)) and attribute pointers
	GLfloat vertices[] = {
		-0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f,  0.5f, -0.5f,
		0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f, -0.5f,  0.5f,
		0.5f, -0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,

		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,

		0.5f,  0.5f,  0.5f,
		0.5f,  0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f, -0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,

		-0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f, -0.5f,  0.5f,
		0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f,  0.5f, -0.5f,
		0.5f,  0.5f, -0.5f,
		0.5f,  0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f
	};

	// Then, we set the light's VAO (VBO stays the same. After all, the vertices are the same for the light object (also a 3D cube))
	GLuint VBO, lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(lightVAO);
	// We only need to bind to the VBO (to link it with glVertexAttribPointer), no need to fill it; the VBO's data already contains all we need.
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Set the vertex attributes (only position data for the lamp))
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);


	// Configure depth map FBO
	const GLuint SHADOW_WIDTH = 3000, SHADOW_HEIGHT = 3000;
	GLuint depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// - Create depth texture
	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);


	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		glfwPollEvents();
		Do_Movement();

		// 1. Render depth of scene to texture (from ligth's perspective)
		// - Get light projection/view matrix.
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		GLfloat near_plane = 1.0f, far_plane = 100.0f;
		lightProjection = glm::ortho(-20.0f, 10.0f, -10.0f, 20.0f, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;
		// - now render scene from light's point of view
		simpleDepthShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(simpleDepthShader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		// Draw the loaded model
		glm::mat4 model;
		model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f)); // Translate it down a bit so it's at the center of the scene
		glUniformMatrix4fv(glGetUniformLocation(simpleDepthShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		ourModel.Draw(simpleDepthShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);



		//----------------------------------------
		//----------------------------------------
		// 2. Render scene as normal 
		glViewport(0, 0, screenWidth, screenHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.Use();
		//initialize view and lights
		glUniform1i(glGetUniformLocation(shader.Program, "shadowMap"), 0);
		GLint viewPosLoc = glGetUniformLocation(shader.Program, "viewPos");
		glUniform3f(viewPosLoc, camera.Position.x, camera.Position.y, camera.Position.z);
		set_lights(shader);
		
		// Transformation matrices
		glm::mat4 projection = glm::perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// Draw the loaded model
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f)); // Translate it down a bit so it's at the center of the scene
		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		ourModel.Draw(shader);
		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		// Draw lamp
		lightShader.Use();
		// Get the uniform locations
		GLint lampLoc = glGetUniformLocation(lightShader.Program, "model");
		GLint viewLoc = glGetUniformLocation(lightShader.Program, "view");
		GLint projLoc = glGetUniformLocation(lightShader.Program, "projection");
		// Set matrices
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glm::mat4 lamp;
		lamp = glm::translate(lamp, lampPos);
		lamp = glm::scale(lamp, glm::vec3(0.2f)); // Make it a smaller cube
		glUniformMatrix4fv(lampLoc, 1, GL_FALSE, glm::value_ptr(lamp));
		// Draw the light object (using light's vertex attributes)
		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		// Draw lamp2
		lightShader.Use();
		// Get the uniform locations
		GLint lampLoc2 = glGetUniformLocation(lightShader.Program, "model");
		GLint viewLoc2 = glGetUniformLocation(lightShader.Program, "view");
		GLint projLoc2 = glGetUniformLocation(lightShader.Program, "projection");
		// Set matrices
		glUniformMatrix4fv(viewLoc2, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc2, 1, GL_FALSE, glm::value_ptr(projection));
		glm::mat4 lamp2;
		lamp2 = glm::translate(lamp2, spotPos);
		lamp2 = glm::scale(lamp2, glm::vec3(0.2f)); // Make it a smaller cube
		glUniformMatrix4fv(lampLoc2, 1, GL_FALSE, glm::value_ptr(lamp2));
		// Draw the light object (using light's vertex attributes)
		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		// Draw lamp3
		lightShader.Use();
		// Get the uniform locations
		GLint lampLoc3 = glGetUniformLocation(lightShader.Program, "model");
		GLint viewLoc3 = glGetUniformLocation(lightShader.Program, "view");
		GLint projLoc3 = glGetUniformLocation(lightShader.Program, "projection");
		// Set matrices
		glUniformMatrix4fv(viewLoc3, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc3, 1, GL_FALSE, glm::value_ptr(projection));
		glm::mat4 lamp3;
		lamp3 = glm::translate(lamp3, lightPos);
		lamp3 = glm::scale(lamp3, glm::vec3(0.2f)); // Make it a smaller cube
		glUniformMatrix4fv(lampLoc3, 1, GL_FALSE, glm::value_ptr(lamp3));
		// Draw the light object (using light's vertex attributes)
		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);


		// 3. DEBUG: visualize depth map by rendering it to plane
		debugDepthQuad.Use();
		glUniform1f(glGetUniformLocation(debugDepthQuad.Program, "near_plane"), near_plane);
		glUniform1f(glGetUniformLocation(debugDepthQuad.Program, "far_plane"), far_plane);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		//RenderQuad();

		// Swap the buffers
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void set_lights(Shader &shader) {
	
	// Directional light
	glUniform3f(glGetUniformLocation(shader.Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(glGetUniformLocation(shader.Program, "lightColor"), 1.0f, 1.0f, 1.0f);
	
	//Point Light
	if (point) {
		glUniform3f(glGetUniformLocation(shader.Program, "pointLight.position"), lampPos.x, lampPos.y, lampPos.z);
		glUniform3f(glGetUniformLocation(shader.Program, "pointLight.color"), 1.0f, 1.0f, 1.0f);
		glUniform1f(glGetUniformLocation(shader.Program, "pointLight.intensity"), 0.5f);
		glUniform1f(glGetUniformLocation(shader.Program, "pointLight.constant"), 1.0f);
		glUniform1f(glGetUniformLocation(shader.Program, "pointLight.linear"), 0.09);
		glUniform1f(glGetUniformLocation(shader.Program, "pointLight.quadratic"), 0.032);
	}

	//Spotlight
	if (spot) {
		glUniform3f(glGetUniformLocation(shader.Program, "spotLight.position"), spotPos.x, spotPos.y, spotPos.z);
		glUniform3f(glGetUniformLocation(shader.Program, "spotLight.direction"), 0.0f, -1.0f, 0.0f);
		glUniform3f(glGetUniformLocation(shader.Program, "spotLight.color"), 1.0f, 1.0f, 1.0f);
		glUniform1f(glGetUniformLocation(shader.Program, "spotLight.intensity"), 0.5f);
		glUniform1f(glGetUniformLocation(shader.Program, "spotLight.constant"), 1.0f);
		glUniform1f(glGetUniformLocation(shader.Program, "spotLight.linear"), 0.09);
		glUniform1f(glGetUniformLocation(shader.Program, "spotLight.quadratic"), 0.032);
		glUniform1f(glGetUniformLocation(shader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
		glUniform1f(glGetUniformLocation(shader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(15.0f)));
	}	
}


// RenderQuad() Renders a 1x1 quad in NDC, best used for framebuffer color targets
// and post-processing effects.
GLuint quadVAO = 0;
GLuint quadVBO;
void RenderQuad()
{
	if (quadVAO == 0)
	{
		GLfloat quadVertices[] = {
			// Positions        // Texture Coords
			-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
			1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
			1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		};
		// Setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}


void RenderScene(Shader &shader)
{
	Model ourModel("nope/nope.obj");

	// Draw the loaded model
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f)); // Translate it down a bit so it's at the center of the scene
	glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model1"), 1, GL_FALSE, glm::value_ptr(model));
	ourModel.Draw(shader);
	
}

#pragma region "User input"

// Moves/alters the camera positions based on user input
void Do_Movement()
{
	// Camera controls
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

#pragma endregion