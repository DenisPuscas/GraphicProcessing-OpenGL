#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 turretModel;
glm::mat4 shuttleModel;
glm::mat4 redLightModel;
glm::mat4 skyModel;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// punctiform light
glm::vec3 redLightPos;
glm::vec3 lightPos1;
glm::vec3 lightPos2;

// fog
glm::vec3 fogColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// camera
gps::Camera myCamera(
    glm::vec3(1.2f, 3.04f, 5.0f),
    glm::vec3(4.0f, 3.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.1f;

GLboolean pressedKeys[1024];

// models
gps::Model3D scene;
gps::Model3D shuttle;
gps::Model3D turret1;
gps::Model3D turret2;
gps::Model3D turret3;
GLfloat angle;
GLfloat turretAngle;

// shaders
gps::Shader myBasicShader;
gps::Shader skyboxShader;
gps::Shader depthMapShader;

// skybox
gps::SkyBox mySkyBox;

// shadow
GLuint shadowMapFBO;
GLuint depthMapTexture; 

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// mouse callback
float lastX = 960, lastY = 540;
float pitch = 0.0f, yaw = -79.43f;
bool firstMouse = true;

// intro
bool intro = true;
float shuttlePos = 5.0f;
float shuttleSpeed = 0.3f;

// turret
GLfloat delta = 0;
float movementSpeed = 5;
double lastTimeStamp = glfwGetTime();

// fog
float fogDensity = 0.02f;

// skybox
float lightDirAngle = 0.0f;
float skyboxAngle = 5.0f;
float skyAngle = 0.0f;
float skySpeed = 0.3f;
int testDay = 0;
int redLightOn = 1;

int mode = 0;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}

void testNight() {
    myBasicShader.useShaderProgram();
    if (lightDir.y < 0) {
        float lightLevel = 1.0f + lightDir.y / 50.0f;
        lightColor = glm::vec3(lightLevel, lightLevel, lightLevel);
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

        fogColor = glm::vec3(lightLevel / 2, lightLevel / 2, lightLevel / 2);
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));
    }
    else {
        lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

        fogColor = glm::vec3(0.5f, 0.5f, 0.5f);
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));
    }
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

    if (pressedKeys[GLFW_KEY_SPACE]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        myBasicShader.useShaderProgram();
        // update model matrix
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        myBasicShader.useShaderProgram();
        // update model matrix
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_X]) {
        if (fogDensity < 0.36) {
            fogDensity += 0.002f;
            myBasicShader.useShaderProgram();
            glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), fogDensity);
        }
    }

    if (pressedKeys[GLFW_KEY_Z]) {
        if (fogDensity > 0) {
            fogDensity -= 0.002f;
            myBasicShader.useShaderProgram();
            glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), fogDensity);
        }
    }

    if (pressedKeys[GLFW_KEY_V]) {
        skyboxAngle -= 0.2f;

        glm::mat4 rotateLight = glm::rotate(glm::mat4(1.0f), glm::radians(0.2f), glm::vec3(0.0f, 0.0f, 1.0f));
        lightDir = glm::vec3(rotateLight * glm::vec4(lightDir, 1.0f));
        myBasicShader.useShaderProgram();
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));

        testNight();
    }

    if (pressedKeys[GLFW_KEY_C]) {
        skyboxAngle += 0.2f;

        glm::mat4 rotateLight = glm::rotate(glm::mat4(1.0f), glm::radians(-0.2f), glm::vec3(0.0f, 0.0f, 1.0f));
        lightDir = glm::vec3(rotateLight * glm::vec4(lightDir, 1.0f));
        myBasicShader.useShaderProgram();
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));

        testNight();
    }

    if (pressedKeys[GLFW_KEY_F]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (pressedKeys[GLFW_KEY_P]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }

    if (pressedKeys[GLFW_KEY_L]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
}

void initOpenGLWindow() {
    myWindow.Create(1920, 1080, "Moon Project");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    scene.LoadModel("models/scena/scena.obj");
    scene.LoadModel("models/cover/cover.obj");
    shuttle.LoadModel("models/shuttle/shuttle.obj");
    turret1.LoadModel("models/turret/turret1.obj");
    turret2.LoadModel("models/turret/turret2.obj");
    turret3.LoadModel("models/turret/turret3.obj");
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    myBasicShader.useShaderProgram();
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
    depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
    depthMapShader.useShaderProgram();
}

void initFBO() {
    //generate FBO ID
    glGenFramebuffers(1, &shadowMapFBO);

    //create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 100.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	// set the light direction (direction towards the light)
	lightDir = glm::vec3(32.0f, 20.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	// set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    
    lightPos1 = glm::vec3(2.13f, 0.74f, 3.45f);
    lightPos2 = glm::vec3(1.95f, 0.74f, 3.61f);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightPos1"), 1, glm::value_ptr(lightPos1));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightPos2"), 1, glm::value_ptr(lightPos2));

    redLightModel = glm::mat4(1.0f);
    redLightPos = glm::vec3(2.27f, 0.16f, -1.23f);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "redLightPos"), 1, glm::value_ptr(redLightPos));
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "redLightModel"), 1, GL_FALSE, glm::value_ptr(redLightModel));
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "redLightOn"), redLightOn);

    // send fog density
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), fogDensity);

    fogColor = glm::vec3(0.5f, 0.5f, 0.5f);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));

    skyboxShader.useShaderProgram();

    // create model matrix
    skyModel = glm::rotate(glm::mat4(1.0f), glm::radians(skyboxAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(skyModel));
}

void initSkybox() {
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/right.png");
    faces.push_back("skybox/left.png");
    faces.push_back("skybox/top.png");
    faces.push_back("skybox/bottom.png");
    faces.push_back("skybox/back.png");
    faces.push_back("skybox/front.png");
    mySkyBox.Load(faces);
}

glm::mat4 computeLightSpaceTrMatrix() {
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = 1.0f, far_plane = 100.0f;
    glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
    return lightSpaceTrMatrix;
}

void renderBase(gps::Shader shader) {
    shader.useShaderProgram();
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "lightModel"), 1, GL_FALSE, glm::value_ptr(model));
    scene.Draw(shader);
}

void updateAnimationTime() {
    double currentTimeStamp = glfwGetTime();
    turretAngle += movementSpeed * (float)(currentTimeStamp - lastTimeStamp);
    skyboxAngle += skySpeed * (float)(currentTimeStamp - lastTimeStamp);
    if (intro) {
        shuttlePos = shuttlePos - shuttleSpeed * (float)(currentTimeStamp - lastTimeStamp);
    }
    lastTimeStamp = currentTimeStamp;
}

void renderShuttle(gps::Shader shader, bool renderingDepthMap) {
    shader.useShaderProgram();
    if (intro) {
        if (shuttlePos <= 0) {
            shuttlePos = 0;
            intro = false;

        }
        else if (shuttlePos < 2 && shuttleSpeed == 0.2f) {
            shuttleSpeed -= 0.1f;
        }
        shuttleModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, shuttlePos, 0.0f));
        redLightModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, shuttlePos, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(shuttleModel));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "redLightModel"), 1, GL_FALSE, glm::value_ptr(redLightModel));

        myCamera.move(gps::MOVE_DOWN, shuttleSpeed/290);
        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        if (!renderingDepthMap) {
            normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        }
    }
    else {
        shuttleModel = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(shuttleModel));

        redLightOn = 0;
        redLightModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -100.0f, 0.0f));
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "redLightOn"), redLightOn);

    }
    shuttle.Draw(shader);
}

void renderTurret(gps::Shader shader) {
    shader.useShaderProgram();
    turretModel = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    turretModel = glm::translate(turretModel, glm::vec3(12.811f, 1.7615f, 5.8126f));
    turretModel = glm::rotate(turretModel, glm::radians(turretAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    turretModel = glm::translate(turretModel, glm::vec3(-12.811f, -1.7615f, -5.8126f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(turretModel));
    turret1.Draw(shader);

    turretModel = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    turretModel = glm::translate(turretModel, glm::vec3(-9.7728f, 1.6396f, -3.6212f));
    turretModel = glm::rotate(turretModel, glm::radians(turretAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    turretModel = glm::translate(turretModel, glm::vec3(9.7728f, -1.6396f, 3.6212f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(turretModel));
    turret2.Draw(shader);

    turretModel = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    turretModel = glm::translate(turretModel, glm::vec3(-6.9807f, 1.621f, 17.61f));
    turretModel = glm::rotate(turretModel, glm::radians(turretAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    turretModel = glm::translate(turretModel, glm::vec3(6.9807f, -1.621f, -17.61f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(turretModel));
    turret3.Draw(shader);
}

void renderSkyBox(gps::Shader shader) {
    skyboxShader.useShaderProgram();
    skyModel = glm::rotate(glm::mat4(1.0f), glm::radians(-skyboxAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(skyModel));

    glm::mat4 rotateLight = glm::rotate(glm::mat4(1.0f), glm::radians(-0.002f), glm::vec3(0.0f, 0.0f, 1.0f));
    lightDir = glm::vec3(rotateLight * glm::vec4(lightDir, 1.0f));
    shader.useShaderProgram();
    glUniform3fv(glGetUniformLocation(shader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));

    testNight();

    mySkyBox.Draw(skyboxShader, view, projection);
}

void renderObjects(gps::Shader shader, bool renderingDepthMap) {
    renderBase(shader);
    updateAnimationTime();
    renderTurret(shader);
    renderShuttle(shader, renderingDepthMap);
    renderSkyBox(shader);
}

void renderScene() {

    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    renderObjects(depthMapShader, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    myBasicShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    //bind the shadow map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

    renderObjects(myBasicShader, false);

}

void cleanup() {
    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    //glfwDestroyWindow(glWindow);
    myWindow.Delete();
    //close GL context and any other GLFW resources
    glfwTerminate();
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
    initFBO();
    setWindowCallbacks();
    initSkybox();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        if (!intro) {
            processMovement();
        }
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
