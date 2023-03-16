#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "Mesh.hpp"
#include "SkyBox.hpp"

#include <iostream>

const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
glm::mat4 lightRotation;

// camera
gps::Camera myCamera(
    glm::vec3(33.0f, 20.5f, 15.0f),
    glm::vec3(33.0f, 20.6f, -15.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.3f;

GLboolean pressedKeys[1024];

// models
gps::Model3D city;
gps::Model3D lightCube;
gps::Model3D frontWheels;
gps::Model3D backWheels;
gps::Model3D carBody;

// scene preview
GLfloat angle;
GLfloat lightAngle;

// shaders
gps::Shader myBasicShader;
gps::Shader lightShader;
gps::Shader depthMapShader;

//skybox
std::vector<const GLchar*> faces;
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

// shadows
GLuint shadowMapFBO;
GLuint depthMapTexture;

// rotate camera
bool cameraRotation = false;
float cameraAngle = 0.0f;

// fog details
int foginit = 0;
GLfloat fogDensity = 0.005f;

// scene animation
bool startAnimation = false;
float animationAngle = 0.0f;

// spotlight
int spotLightInitialize;
float spotLight1;
float spotLight2;

glm::vec3 spotLightDirection;
glm::vec3 spotLightPosition;


void sceneAnimation() {
    if (startAnimation) {
        animationAngle += 0.2f;
        myCamera.scenePreview(animationAngle);
    }
}

GLenum glCheckError_(const char* file, int line)
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
        case GL_STACK_OVERFLOW:
            error = "STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error = "STACK_UNDERFLOW";
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
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void updateView() {
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

double oldXPos;
double oldYPos;
const float cameraDefaultRotation = 0.1f;

//bool mouseMoved = false;
void mouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    //mouseMoved = true;
    myCamera.rotate((oldYPos - yPos) * cameraDefaultRotation, (oldXPos - xPos) * cameraDefaultRotation);

    oldXPos = xPos;
    oldYPos = yPos;

    updateView();
}

// initialize faces for skybox
void initFaces()
{
    faces = {
        "textures/skybox/right.tga",
        "textures/skybox/left.tga",
        "textures/skybox/top.tga",
        "textures/skybox/bottom.tga",
        "textures/skybox/back.tga",
        "textures/skybox/front.tga"
    };
}

bool activateCollisions = false;

bool checkCollision() {
    if (!activateCollisions) {
        return false;
    }
    glm::vec3 cameraPosition = myCamera.getCameraPosition();
    float cameraX = cameraPosition.x;
    float cameraY = cameraPosition.y;
    float cameraZ = cameraPosition.z;
    return cameraX > 80 || cameraX < -30 || cameraY < 0 || cameraZ < -90 || cameraZ > 40;
}

bool carAnimationBool = false;
float wheelAngle = 0.0f;
float carDistance = 0.0f;

void processMovement() {
    if (pressedKeys[GLFW_KEY_N])
        activateCollisions = true;

    if (pressedKeys[GLFW_KEY_M])
        activateCollisions = false;

    if (pressedKeys[GLFW_KEY_W]) {
        if (checkCollision())
            myCamera.move(gps::MOVE_FORWARD, -1);
        else
            myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        //update view matrix
        updateView();
    }

    if (pressedKeys[GLFW_KEY_S]) {
        if (checkCollision())
            myCamera.move(gps::MOVE_BACKWARD, -1);
        else
            myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        updateView();
    }

    if (pressedKeys[GLFW_KEY_A]) {
        if (checkCollision())
            myCamera.move(gps::MOVE_LEFT, -1);
        else
            myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        updateView();
    }

    if (pressedKeys[GLFW_KEY_D]) {
        if (checkCollision())
            myCamera.move(gps::MOVE_RIGHT, -1);
        else
            myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        updateView();
    }

    if (pressedKeys[GLFW_KEY_T]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_R]) {
        if (checkCollision())
            myCamera.move(gps::MOVE_DOWN, -1);
        else
            myCamera.move(gps::MOVE_DOWN, cameraSpeed);
        //update view matrix
        updateView();
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        lightAngle -= 1.0f;
        glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
        myBasicShader.useShaderProgram();
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        lightAngle += 1.0f;
        glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
        myBasicShader.useShaderProgram();
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
    }

    // start scene animation
    if (pressedKeys[GLFW_KEY_1]) {
        startAnimation = true;
    }

    // stop scene animation
    if (pressedKeys[GLFW_KEY_2]) {
        startAnimation = false;
    }

    // start fog
    if (pressedKeys[GLFW_KEY_3]) {
        myBasicShader.useShaderProgram();
        foginit = 1;
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "foginit"), foginit);
    }

    // stop fog
    if (pressedKeys[GLFW_KEY_4]) {
        myBasicShader.useShaderProgram();
        foginit = 0;
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "foginit"), foginit);

    }

    // increase fog density
    if (pressedKeys[GLFW_KEY_5]) {
        fogDensity = glm::min(fogDensity + 0.001f, 1.0f);
    }

    // decrease fog density
    if (pressedKeys[GLFW_KEY_6]) {
        fogDensity = glm::max(fogDensity - 0.001f, 0.0f);
    }


    // start spotlight
    if (pressedKeys[GLFW_KEY_C]) {
        myBasicShader.useShaderProgram();
        spotLightInitialize = 1;
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "spotLightInitialize"), spotLightInitialize);
    }

    // stop spotlight
    if (pressedKeys[GLFW_KEY_V]) {
        myBasicShader.useShaderProgram();
        spotLightInitialize = 0;
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "spotLightInitialize"), spotLightInitialize);
    }

    if (pressedKeys[GLFW_KEY_7]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (pressedKeys[GLFW_KEY_8]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }

    if (pressedKeys[GLFW_KEY_9]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (pressedKeys[GLFW_KEY_Y]) {
        carAnimationBool = true;
        carDistance+=0.01f;
        if (wheelAngle == -360.f)
            wheelAngle = 0.0f;
        wheelAngle -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_U]) {
        carAnimationBool = false;
    }

    if (pressedKeys[GLFW_KEY_I]) {
        carAnimationBool = true;
        carDistance-=0.1f;
        if (wheelAngle == 360.f)
            wheelAngle = 0.0f;
        wheelAngle += 1.0f;
    }

}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
    glfwGetCursorPos(myWindow.getWindow(), &oldXPos, &oldYPos);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
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
    city.LoadModel("models/city/city.obj");
    lightCube.LoadModel("models/cube/cube.obj");
    frontWheels.LoadModel("models/frontWheels/frontWheels.obj");
    backWheels.LoadModel("models/backWheels/backWheels.obj");
    carBody.LoadModel("models/carBody/carBody.obj");
}

void initShaders() {
    myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    myBasicShader.useShaderProgram();
    lightShader.loadShader(
        "shaders/lightCube.vert",
        "shaders/lightCube.frag");
    lightShader.useShaderProgram();
    depthMapShader.loadShader(
        "shaders/depthMap.vert",
        "shaders/depthMap.frag");
    depthMapShader.useShaderProgram();
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    // create model matrix
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // get view matrix for current camera
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    // send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
    //glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // create projection matrix
    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //set the light direction (direction towards the light)
    lightDir = glm::vec3(-49.0f, 62.5f, -43.5f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    // send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir));

    //set light color
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    // send light color to shader
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // spotlight
    spotLight1 = glm::cos(glm::radians(45.5f));
    spotLight2 = glm::cos(glm::radians(90.0f));

    spotLightDirection = glm::vec3(49.25f, 2.6148f, -16.328f);
    spotLightPosition = glm::vec3(49.25f, 4.6148f, -16.328f);

    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotlight1"), spotLight1);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotlight2"), spotLight2);

    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLightDirection"), 1, glm::value_ptr(spotLightDirection));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLightPosition"), 1, glm::value_ptr(spotLightPosition));

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initSkyBoxShader()
{
    mySkyBox.Load(faces);
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
        glm::value_ptr(view));

    projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
        glm::value_ptr(projection));
}

void initFBO() {
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO

     //generate FBO ID
    glGenFramebuffers(1, &shadowMapFBO);

    //create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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

void renderCity(gps::Shader shader, bool depth) {
    // select active shader program
    shader.useShaderProgram();
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(2.0f));
    model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.05f, 6.0f));

    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    if (depth) {
        //send teapot normal matrix data to shader
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw city
    city.Draw(shader);
}

void renderFrontWheels(gps::Shader shader, bool depth) {
    
    // select active shader program
    shader.useShaderProgram();
    model = glm::mat4(1.0f);
    //model = glm::scale(model, glm::vec3(2.0f));
    //model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.05f, 6.0f));
    if (carAnimationBool)
    {

        model = glm::translate(model, glm::vec3(68.085f, 0.072701f, (-24.178f - carDistance)));

        model = glm::rotate(model, glm::radians(wheelAngle), glm::vec3(1, 0, 0));
        model = glm::translate(model, glm::vec3(-68.085f, -0.072701f, 24.178f));


    }
     

    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    if (depth) {
        //send normal matrix data to shader
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw frontWheels
    frontWheels.Draw(shader);
}

void renderbackWheels(gps::Shader shader, bool depth) {
    // select active shader program
    shader.useShaderProgram();
    model = glm::mat4(1.0f);
    //model = glm::scale(model, glm::vec3(2.0f));
    //model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.05f, 6.0f));
  
    if (carAnimationBool)
    {
        model = glm::translate(model, glm::vec3(68.085f, 0.072704f, (-22.22f - carDistance)));

        model = glm::rotate(model, glm::radians(wheelAngle), glm::vec3(1, 0, 0));
        model = glm::translate(model, glm::vec3(-68.085f, -0.072704f, 22.22f));


    }

    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    if (depth) {
        //send normal matrix data to shader
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw backWheels
    backWheels.Draw(shader);
}

void rendercarBody(gps::Shader shader, bool depth) {
    // select active shader program
    shader.useShaderProgram();
    //model = glm::scale(model, glm::vec3(2.0f));
    //model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.05f, 6.0f));
    model = glm::mat4(1.0f);
    if(carAnimationBool)
    {
        //translate body forward
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (-carDistance)));
    }

    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    if (depth) {
        //send normal matrix data to shader
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw carBody
    carBody.Draw(shader);
}

glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix
    glm::mat4 lightView = glm::lookAt(glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = 0.1f, far_plane = 200.0f;
    glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
    return lightSpaceTrMatrix;
}


void renderScene() {


    sceneAnimation();

    depthMapShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    //render scene = draw objects
    renderCity(depthMapShader, false);


    
    rendercarBody(depthMapShader, false);
    renderFrontWheels(depthMapShader, false);
    renderbackWheels(depthMapShader, false);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    myBasicShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir));

    //bind the shadow map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), fogDensity);

    renderCity(myBasicShader, true);
    
    rendercarBody(myBasicShader, true);
    renderFrontWheels(myBasicShader, true);
    renderbackWheels(myBasicShader, true);
    

    lightShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

    model = lightRotation;
    model = glm::translate(model, 1.2f * lightDir);
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    lightCube.Draw(lightShader);

    // render the sky box
    mySkyBox.Draw(skyboxShader, view, projection);

}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initModels();
    initShaders();
    initUniforms();
    initFBO();
    setWindowCallbacks();

    initFaces();
    initSkyBoxShader();

    glCheckError();
    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        /*angle += 1.0f;
        if (angle >= 360)
            angle = 0.0f;*/
        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());
    }

    cleanup();

    return EXIT_SUCCESS;
}
