//internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "camera.h"
#include "model.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <SOIL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <string>
#include <vector>

#include <ctime>
#include <cmath>


struct ModelAttributes
{
    float appearance_timestamp;
    float x;
    float y;
};

static const GLsizei WIDTH = 1120;
static const GLsizei HEIGHT = 840;

// Camera.
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float) WIDTH / 2.0;
float lastY = (float) HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

ShaderProgram program;
ShaderProgram model_program;
ShaderProgram skybox_program;
unsigned int cubemapTexture;
GLuint VAO;
unsigned int skyboxVAO;
float current_frame;
GLuint scope_texture;


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = SOIL_load_image(faces[i].c_str(),
                                              &width,
                                              &height,
                                              &nrChannels,
                                              0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0,
                         GL_RGB,
                         width,
                         height,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         data);
            SOIL_free_image_data(data);
        
        } else {
            std::cout << "Cubemap texture failed to load at path: "
                      << faces[i]
                      << std::endl;
            SOIL_free_image_data(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = SOIL_load_image(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        SOIL_free_image_data(data);
    
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        SOIL_free_image_data(data);
    }

    return textureID;
}

void draw_model(Model model, float appearance_timestamp, float x, float y)
{
    model_program.StartUseShader();
    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) WIDTH / (float) HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    model_program.SetUniform("view", view);
    model_program.SetUniform("projection", projection);

    // First starship.
    glm::mat4 starship_model = glm::mat4(1.0f);
    starship_model = glm::translate(starship_model,
            glm::vec3(x, y,
            -100.0f + 20 * (current_frame - appearance_timestamp)));

    starship_model = glm::rotate(starship_model,
                             3.14095f,
                             glm::vec3(0.0f, 1.0f, 0.0f));

    /*starship_model = glm::scale(starship_model, glm::vec3(0.2f, 0.2f, 0.2f));*/

    model_program.SetUniform("model", starship_model);
    model.Draw(model_program);
}

void draw_skybox()
{
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    skybox_program.StartUseShader();
    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) WIDTH / (float) HEIGHT, 0.1f, 100.0f);
    skybox_program.SetUniform("view", view);
    skybox_program.SetUniform("projection", projection);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}

/*void draw_scope()
{
    glBindTexture(GL_TEXTURE_2D, scope_texture);

    program.StartUseShader();

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}*/

int initGL()
{
	int res = 0;
	//грузим функции opengl через glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	std::cout << "Vendor: "   << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: "  << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL: "     << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	return 0;
}


int main(int argc, char** argv)
{
	if (!glfwInit()) {
        return -1;
    }

	//запрашиваем контекст opengl версии 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); 

    GLFWwindow*  window = glfwCreateWindow(WIDTH,
                                           HEIGHT,
                                           "SMIERTIELNAJA BITWA",
                                           nullptr,
                                           nullptr);

	if (window == nullptr) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	
	glfwMakeContextCurrent(window); 
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (initGL() != 0) {
	   return -1;
    }

    //Reset any OpenGL errors which could be present for some reason
	GLenum gl_error = glGetError();
	while (gl_error != GL_NO_ERROR){
		gl_error = glGetError();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/*//создание шейдерной программы из двух файлов с исходниками шейдеров
	//используется класс-обертка ShaderProgram
	std::unordered_map<GLenum, std::string> shaders;
	shaders[GL_VERTEX_SHADER] = "vertex.glsl";
	shaders[GL_FRAGMENT_SHADER] = "fragment.glsl";
	program = ShaderProgram(shaders);
    GL_CHECK_ERRORS;*/

    //создание шейдерной программы из двух файлов с исходниками шейдеров
    //используется класс-обертка ShaderProgram
    std::unordered_map<GLenum, std::string> skybox_shaders;
    skybox_shaders[GL_VERTEX_SHADER] = "skybox_vertex.glsl";
    skybox_shaders[GL_FRAGMENT_SHADER] = "skybox_fragment.glsl";
    skybox_program = ShaderProgram(skybox_shaders);
    GL_CHECK_ERRORS;

    //создание шейдерной программы из двух файлов с исходниками шейдеров
    //используется класс-обертка ShaderProgram
    std::unordered_map<GLenum, std::string> model_shaders;
    model_shaders[GL_VERTEX_SHADER] = "model_vertex.glsl";
    model_shaders[GL_FRAGMENT_SHADER] = "model_fragment.glsl";
    model_program = ShaderProgram(model_shaders);
    GL_CHECK_ERRORS;

    // glfwSwapInterval(1); // force 60 frames per second

    /*GLfloat vertices[] =
    {
        // Positions          // Colors           // Texture Coords
         0.05f,  0.05f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right
         0.05f, -0.05f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Bottom Right
        -0.05f, -0.05f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // Bottom Left
        -0.05f,  0.05f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // Top Left 
    };

    GLuint indices[] =
    {  // Note that we start from 0!
        0, 1, 3, // First Triangle
        1, 2, 3  // Second Triangle
    };*/

    float skyboxVertices[] =
    {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    /*GLuint VBO;
    GLuint EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // TexCoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) (6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // Unbind VAO

    scope_texture = loadTexture("../resources/textures/scope.png");*/

    // skybox VAO
    unsigned int skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // load textures
    // -------------
    std::vector<std::string> faces
    {
        "../resources/textures/skybox/purplenebula_lf.tga",
        "../resources/textures/skybox/purplenebula_rt.tga",
        "../resources/textures/skybox/purplenebula_up.tga",
        "../resources/textures/skybox/purplenebula_dn.tga",
        "../resources/textures/skybox/purplenebula_ft.tga",
        "../resources/textures/skybox/purplenebula_bk.tga",
    };
    
    cubemapTexture = loadCubemap(faces);

    // shader configuration
    // --------------------
    /*program.StartUseShader();
    program.SetUniform("skybox", 0);*/

    skybox_program.StartUseShader();
    skybox_program.SetUniform("skybox", 0);

    /*model_program.StartUseShader();*/

    /*Model nanosuit_model(
            "../resources/objects/nanosuit/nanosuit.obj");*/

    Model vulcan_starship_model(
            "../resources/objects/vulcan_dkyr_class/vulcan_dkyr_class.obj");

    Model e45_model(
            "../resources/objects/E-45-Aircraft/E 45 Aircraft_obj.obj");

    std::vector<ModelAttributes> model_attributes;
    /*std::vector<void (draw_model)(Model model, float x, float y)>
            models;*/

    srand(time(0));

    float prev_timestamp = 0.0f;
    bool was_push_back = false;

    // Render loop.
    while (!glfwWindowShouldClose(window)) {
        current_frame = glfwGetTime();
        deltaTime = current_frame - lastFrame;
        lastFrame = current_frame;

        processInput(window);

        glClearColor(0.71f, 0.09f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float rounded_current_frame = round(current_frame);
        
        if (not was_push_back and ((int) rounded_current_frame) % 2 == 0) {
            model_attributes.push_back(
                    {
                        current_frame,
                        (float) -15 + rand() % 31,
                        (float) -15 + rand() % 31
                    });

            prev_timestamp = rounded_current_frame;
            was_push_back = true;
        
        } else if (was_push_back and rounded_current_frame > prev_timestamp) {
            was_push_back = false;
        }

        /*std::cout << "+++++++++++++++" << std::endl;
        std::cout << "rounded_current_frame = "
                  << rounded_current_frame
                  << std::endl;
        for (int i = 0; i < model_attributes.size(); i++) {
            std::cout << "model_attributes["
                      << i
                      << "] = "
                      << model_attributes[i].appearance_timestamp
                      << " : "
                      << model_attributes[i].x
                      << " : "
                      << model_attributes[i].y
                      << std::endl;
        }
        std::cout << "---------------" << std::endl << std::endl;*/

        /*std::cout << "rand_value = " << -10 + rand() % 21 << std::endl;*/

        /*std::cout << "current_frame = " << rounded_current_frame << std::endl;*/

        /*draw_model(e45_model,
                   5.0f,
                   -15.0f,
                   -15.0f);*/

        for (auto &it: model_attributes) {
            draw_model(e45_model,
                       it.appearance_timestamp,
                       it.x,
                       it.y);
        }

        draw_skybox();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    /*glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);*/

    glfwTerminate();
    return 0;
}
