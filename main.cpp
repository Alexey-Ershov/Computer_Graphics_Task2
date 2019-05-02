//internal includes
#include "common.h"
#include "ShaderProgram.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <SOIL.h>

#include <random>
#include <string>
#include <vector>

static const GLsizei WIDTH = 1024, HEIGHT = 768; //размеры окна


void key_callback(GLFWwindow* window,
                  int key,
                  int scancode,
                  int action,
                  int mode)
{
    // Когда пользователь нажимает ESC, мы устанавливаем свойство WindowShouldClose в true, 
    // и приложение после этого закроется
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    std::string face = "../starry_sky.tga";

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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glfwSetKeyCallback(window, key_callback);

	if (initGL() != 0) {
	   return -1;
    }
	
    //Reset any OpenGL errors which could be present for some reason
	GLenum gl_error = glGetError();
	while (gl_error != GL_NO_ERROR){
		gl_error = glGetError();
    }

	//создание шейдерной программы из двух файлов с исходниками шейдеров
	//используется класс-обертка ShaderProgram
	std::unordered_map<GLenum, std::string> shaders;
	shaders[GL_VERTEX_SHADER] = "vertex.glsl";
	shaders[GL_FRAGMENT_SHADER] = "fragment.glsl";
	ShaderProgram program(shaders);
    GL_CHECK_ERRORS;

    //создание шейдерной программы из двух файлов с исходниками шейдеров
    //используется класс-обертка ShaderProgram
    std::unordered_map<GLenum, std::string> skybox_shaders;
    skybox_shaders[GL_VERTEX_SHADER] = "skybox_vertex.glsl";
    skybox_shaders[GL_FRAGMENT_SHADER] = "skybox_fragment.glsl";
    ShaderProgram skybox_program(skybox_shaders);
    GL_CHECK_ERRORS;

    glfwSwapInterval(1); // force 60 frames per second

    //Создаем и загружаем геометрию поверхности
    GLuint g_vertexBufferObject;
    GLuint g_vertexArrayObject;
    GLuint EBO;
  
    GLfloat vertices[] =
    {
        // Positions           // Texture Coords
         0.5f,  0.5f,  0.0f,   0.9f, 0.0f, // Top Right
         0.5f, -0.5f,  0.0f,   0.9f, 1.0f, // Bottom Right
        -0.5f, -0.5f,  0.0f,   0.0f, 1.0f, // Bottom Left
        -0.5f,  0.5f,  0.0f,   0.0f, 0.0f  // Top Left
    };

    GLuint indices[] =
    {
        0, 1, 3, // Первый треугольник
        1, 2, 3  // Второй треугольник
    };

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

    g_vertexBufferObject = 0;
    GLuint vertexLocation = 0;

    glGenVertexArrays(1, &g_vertexArrayObject);

    glGenBuffers(1, &g_vertexBufferObject);
    GL_CHECK_ERRORS;
    
    glGenBuffers(1, &EBO);
    GL_CHECK_ERRORS;
    
    glBindVertexArray(g_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject);
    GL_CHECK_ERRORS;

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vertices),
                 (GLfloat *) vertices,
                 GL_STATIC_DRAW);
    GL_CHECK_ERRORS;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    GL_CHECK_ERRORS;

    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(indices),
                 (GLfloat *) indices,
                 GL_STATIC_DRAW);
    GL_CHECK_ERRORS;

    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    GL_CHECK_ERRORS;

    glEnableVertexAttribArray(vertexLocation);

    // Position attribute
    glVertexAttribPointer(vertexLocation,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          5 * sizeof(GLfloat),
                          (GLvoid *) 0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute
    glVertexAttribPointer(vertexLocation + 1,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          5 * sizeof(GLfloat),
                          (GLvoid *) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
    GL_CHECK_ERRORS;

    GLuint textureID;
    glGenTextures(1, &textureID);
    
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    int width, height;
    unsigned char* image = SOIL_load_image("../e38.png",
                                           &width,
                                           &height,
                                           0,
                                           SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 width,
                 height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    // skybox VAO
    unsigned int skyboxVAO;
    unsigned int skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(skyboxVertices),
                 &skyboxVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          3 * sizeof(float),
                          (void *) 0);

    std::vector<std::string> faces
    {
        "../textures/skybox/vr_rt.tga",
        "../textures/skybox/vr_lf.tga",
        "../textures/skybox/vr_up.tga",
        "../textures/skybox/vr_dn.tga",
        "../textures/skybox/vr_bk.tga",
        "../textures/skybox/vr_ft.tga",
    };

    unsigned int cubemapTexture = loadCubemap(faces);

	// Game loop.
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		//очищаем экран каждый кадр
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        GL_CHECK_ERRORS;
		
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL_CHECK_ERRORS;

        glClearColor(0.0f, 0.169f, 0.212f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);

        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skybox_program.StartUseShader();
        /*view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);*/
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        /*
        glBindTexture(GL_TEXTURE_2D, textureID);
        program.StartUseShader();
        glBindVertexArray(g_vertexArrayObject);
        GL_CHECK_ERRORS;
        // // glDrawArrays(GL_TRIANGLES, 0, 3);
        // GL_CHECK_ERRORS;  // The last parameter of glDrawArrays is equal to
                          // VS invocations
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);*/
        
        // program.StopUseShader();

        glfwSwapBuffers(window); 
	}

	glDeleteVertexArrays(1, &g_vertexArrayObject);
    glDeleteBuffers(1, &g_vertexBufferObject);
    glDeleteBuffers(1, &EBO);

	glfwTerminate();
	return 0;
}
