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
#include <ft2build.h>
#include FT_FREETYPE_H

#include <random>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <ctime>
#include <cmath>


/*float iks;
float igrec;*/

float dist = 2.5f;

struct ModelAttributes
{
    float appearance_timestamp;
    glm::vec3 coords;
    glm::vec3 real_coords;
};

struct StarShipAttributes
{
    float appearance_timestamp;
    float last_shot_timestamp;
    glm::vec3 coords;
    glm::vec3 real_coords;
};

/// Holds all state information relevant to a character as loaded using FreeType
struct Character
{
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
GLuint VAO;
GLuint VBO;

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
ShaderProgram text_program;
ShaderProgram plasm_ball_program;
ShaderProgram explosion_program;
unsigned int cubemapTexture;
unsigned int skyboxVAO;
float current_frame;
GLuint scope_texture;
std::vector<StarShipAttributes> model_attributes;
std::vector<ModelAttributes> plasm_ball_attributes;
std::vector<ModelAttributes> enemy_plasm_ball_attributes;
std::vector<ModelAttributes> explosion_attributes;
std::vector<ModelAttributes> dust_attributes;


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
        // igrec++;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
        // igrec--;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
        // iks--;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
        // iks++;
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

void mouse_button_callback(GLFWwindow* window,
                           int button,
                           int action,
                           int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        
        /*std::cout << "+++++++++++++++++++" << std::endl;
        std::cout << "xpos = " << camera.Front.x << std::endl;
        std::cout << "ypos = " << camera.Front.y << std::endl;
        std::cout << "zpos = " << camera.Front.z << std::endl;
        std::cout << "-------------------" << std::endl << std::endl;*/

        plasm_ball_attributes.push_back(
                {
                    current_frame,
                    glm::vec3(
                        camera.Front.x,
                        camera.Front.y,
                        camera.Front.z
                    )
                });
    
    } else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        /*model_attributes.push_back(
                    {
                        current_frame,
                        glm::vec3(
                            -5.0f,
                            -5.0f,
                             0.0f
                        )
                    });

        model_attributes.push_back(
                    {
                        current_frame,
                        glm::vec3(
                             5.0f,
                            -5.0f,
                             0.0f
                        )
                    });*/

        /*explosion_attributes.push_back(
            {
                current_frame,
                glm::vec3(1.0f, 1.0f, -2.5f)
            });*/
    }
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

void draw_model(Model &model, StarShipAttributes &attrs)
{
    attrs.real_coords = glm::vec3(
            attrs.coords.x,
            attrs.coords.y,
            -100.0f + 20 * (current_frame - attrs.appearance_timestamp));
            /*-20.0f);*/

    model_program.StartUseShader();
    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) WIDTH / (float) HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    model_program.SetUniform("view", view);
    model_program.SetUniform("projection", projection);

    // First starship.
    glm::mat4 starship_model = glm::mat4(1.0f);
    starship_model = glm::translate(starship_model, attrs.real_coords);

    starship_model = glm::rotate(starship_model,
                                 3.14095f,
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    /*starship_model = glm::scale(starship_model, glm::vec3(0.2f, 0.2f, 0.2f));*/

    model_program.SetUniform("model", starship_model);
    model.Draw(model_program);
}

void draw_plasm_ball(Model &model, ModelAttributes &attrs)
{
    attrs.real_coords = glm::vec3(
            200 * attrs.coords.x *
                (current_frame - attrs.appearance_timestamp),
            200 * attrs.coords.y *
                (current_frame - attrs.appearance_timestamp),
            150 * attrs.coords.z / abs(attrs.coords.z) *
                (current_frame - attrs.appearance_timestamp));

    plasm_ball_program.StartUseShader();
    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) WIDTH / (float) HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    plasm_ball_program.SetUniform("view", view);
    plasm_ball_program.SetUniform("projection", projection);

    // First starship.
    glm::mat4 starship_model = glm::mat4(1.0f);
    starship_model = glm::translate(starship_model, attrs.real_coords);

    starship_model = glm::scale(starship_model, glm::vec3(0.005f,
                                                          0.005f,
                                                          0.005f));

    plasm_ball_program.SetUniform("model", starship_model);
    model.Draw(plasm_ball_program);
}

void draw_enemy_plasm_ball(Model &model, ModelAttributes &attrs)
{
    attrs.real_coords = glm::vec3(
            /*200 * attrs.coords.x *
                (current_frame - attrs.appearance_timestamp),
            200 * attrs.coords.y *
                (current_frame - attrs.appearance_timestamp),*/
            /*-150 * attrs.coords.z / abs(attrs.coords.z) *
                (current_frame - attrs.appearance_timestamp)*/
            attrs.coords.x - 2 * attrs.coords.x *
                (current_frame - attrs.appearance_timestamp),

            attrs.coords.y - 2 * attrs.coords.y *
                (current_frame - attrs.appearance_timestamp),

            attrs.coords.z - 2 * (attrs.coords.z - 3.0f) *
                (current_frame - attrs.appearance_timestamp));

    plasm_ball_program.StartUseShader();
    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) WIDTH / (float) HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    plasm_ball_program.SetUniform("view", view);
    plasm_ball_program.SetUniform("projection", projection);

    // First starship.
    glm::mat4 starship_model = glm::mat4(1.0f);
    starship_model = glm::translate(starship_model, attrs.real_coords);

    starship_model = glm::scale(starship_model, glm::vec3(0.005f,
                                                          0.005f,
                                                          0.005f));

    plasm_ball_program.SetUniform("model", starship_model);
    model.Draw(plasm_ball_program);
}

void draw_exploison(Model &model, ModelAttributes &attrs)
{
    explosion_program.StartUseShader();
    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) WIDTH / (float) HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    explosion_program.SetUniform("view", view);
    explosion_program.SetUniform("projection", projection);

    // First starship.
    glm::mat4 starship_model = glm::mat4(1.0f);
    starship_model = glm::translate(starship_model, attrs.coords);

    starship_model =
            glm::scale(starship_model,
glm::vec3(/*0.001f + */0.1f * (current_frame - attrs.appearance_timestamp),
          /*0.001f + */0.1f * (current_frame - attrs.appearance_timestamp),
          /*0.001f + */0.1f * (current_frame - attrs.appearance_timestamp)));

    explosion_program.SetUniform("model", starship_model);
    model.Draw(explosion_program);
}

void draw_dust(Model &model, ModelAttributes &attrs)
{
    attrs.real_coords = glm::vec3(
            attrs.coords.x,
            attrs.coords.y,
            -100.0f + 100 * (current_frame - attrs.appearance_timestamp));

    plasm_ball_program.StartUseShader();
    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) WIDTH / (float) HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    plasm_ball_program.SetUniform("view", view);
    plasm_ball_program.SetUniform("projection", projection);

    // First starship.
    glm::mat4 starship_model = glm::mat4(1.0f);
    starship_model = glm::translate(starship_model, attrs.real_coords);

    starship_model = glm::scale(starship_model, glm::vec3(0.04f,
                                                          0.04f,
                                                          0.04f));

    plasm_ball_program.SetUniform("model", starship_model);
    model.Draw(plasm_ball_program);
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

void RenderText(ShaderProgram &program,
                std::string text,
                GLfloat x,
                GLfloat y,
                GLfloat scale,
                glm::vec3 color)
{
    // Activate corresponding render state  
    program.StartUseShader();
    glUniform3f(glGetUniformLocation(
                program.GetProgram(), "textColor"),
                color.x,
                color.y,
                color.z);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },            
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }           
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void clear_objects()
{
    for (auto &it: model_attributes) {
        if (current_frame - it.appearance_timestamp > 10) {
            model_attributes.erase(model_attributes.begin());
        
        } else {
            break;
        }
    }

    for (auto &it: plasm_ball_attributes) {
        if (current_frame - it.appearance_timestamp > 1) {
            plasm_ball_attributes.erase(plasm_ball_attributes.begin());
        
        } else {
            break;
        }
    }

    for (auto &it: enemy_plasm_ball_attributes) {
        if (current_frame - it.appearance_timestamp > 1) {
            enemy_plasm_ball_attributes.erase(
                    enemy_plasm_ball_attributes.begin());
        
        } else {
            break;
        }
    }

    for (auto &it: explosion_attributes) {
        if (current_frame - it.appearance_timestamp > 0.3) {
            explosion_attributes.erase(explosion_attributes.begin());
        
        } else {
            break;
        }
    }

    for (auto &it: dust_attributes) {
        if (current_frame - it.appearance_timestamp > 1) {
            dust_attributes.erase(dust_attributes.begin());
        
        } else {
            break;
        }
    }
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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
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

    std::unordered_map<GLenum, std::string> skybox_shaders;
    skybox_shaders[GL_VERTEX_SHADER] = "skybox_vertex.glsl";
    skybox_shaders[GL_FRAGMENT_SHADER] = "skybox_fragment.glsl";
    skybox_program = ShaderProgram(skybox_shaders);
    GL_CHECK_ERRORS;

    std::unordered_map<GLenum, std::string> model_shaders;
    model_shaders[GL_VERTEX_SHADER] = "model_vertex.glsl";
    model_shaders[GL_FRAGMENT_SHADER] = "model_fragment.glsl";
    model_program = ShaderProgram(model_shaders);
    GL_CHECK_ERRORS;

    std::unordered_map<GLenum, std::string> text_shaders;
    text_shaders[GL_VERTEX_SHADER] = "text_vertex.glsl";
    text_shaders[GL_FRAGMENT_SHADER] = "text_fragment.glsl";
    text_program = ShaderProgram(text_shaders);
    GL_CHECK_ERRORS;

    std::unordered_map<GLenum, std::string> plasm_ball_shaders;
    plasm_ball_shaders[GL_VERTEX_SHADER] = "plasm_ball_vertex.glsl";
    plasm_ball_shaders[GL_FRAGMENT_SHADER] = "plasm_ball_fragment.glsl";
    plasm_ball_program = ShaderProgram(plasm_ball_shaders);
    GL_CHECK_ERRORS;

    std::unordered_map<GLenum, std::string> explosion_shaders;
    explosion_shaders[GL_VERTEX_SHADER] = "explosion_vertex.glsl";
    explosion_shaders[GL_FRAGMENT_SHADER] = "explosion_fragment.glsl";
    explosion_program = ShaderProgram(explosion_shaders);
    GL_CHECK_ERRORS;

    glm::mat4 projection = glm::ortho(0.0f,
                                      static_cast<GLfloat>(WIDTH),
                                      0.0f,
                                      static_cast<GLfloat>(HEIGHT));
    text_program.StartUseShader();
    glUniformMatrix4fv(glGetUniformLocation(
                       text_program.GetProgram(), "projection"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "../resources/fonts/arial.ttf", 0, &face))
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (GLuint) face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    
    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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

    /*Model vulcan_starship_model(
            "../resources/objects/vulcan_dkyr_class/vulcan_dkyr_class.obj");*/

    Model e45_model(
            "../resources/objects/E-45-Aircraft/E 45 Aircraft_obj.obj");

    Model sphere_model(
            "../resources/objects/Quad_Sphere/3d-model.obj");

    Model dust_model(
            "../resources/objects/cube/cube.obj");

    /*Model explosion_model(
            "../resources/objects/asteroid/10464_Asteroid_v1_Iterations-2.obj");*/

    /*std::vector<void (draw_model)(Model model, float x, float y)>
            models;*/

    srand(time(0));

    float prev_model_timestamp = 0.0f;
    float prev_dust_timestamp = 0.0f;

    // Render loop.
    while (!glfwWindowShouldClose(window)) {
        current_frame = glfwGetTime();
        deltaTime = current_frame - lastFrame;
        lastFrame = current_frame;

        processInput(window);

        glClearColor(0.02f, 0.2f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (current_frame - prev_model_timestamp > 2.0f) {
            model_attributes.push_back(
                {
                    current_frame,
                    current_frame + 1.0f,
                    glm::vec3(
                        (float) -15 + rand() % 31,
                        (float) -15 + rand() % 31,
                        0.0f
                    )
                });

            prev_model_timestamp = current_frame;
        }

        if (current_frame - prev_dust_timestamp > 0.1f) {
            dust_attributes.push_back(
                {
                    current_frame,
                    glm::vec3(
                        (float) -15 + rand() % 31,
                        (float) -15 + rand() % 31,
                        0.0f
                    )
                });

            prev_dust_timestamp = current_frame;
        
        }

        clear_objects();

        for (auto &it: model_attributes) {
            draw_model(e45_model, it);
            if (it.real_coords.z < 0.0f and
                    current_frame - it.last_shot_timestamp > 1.0f) {
                
                enemy_plasm_ball_attributes.push_back(
                    {
                        current_frame,
                        it.real_coords
                    });

                it.last_shot_timestamp = current_frame;
            }
        }

        for (auto &it: plasm_ball_attributes) {
            draw_plasm_ball(sphere_model, it);
        }

        for (auto &it: enemy_plasm_ball_attributes) {
            draw_enemy_plasm_ball(sphere_model, it);
        }

        for (auto &it: dust_attributes) {
            draw_dust(dust_model, it);
        }

        std::set<unsigned int> deleted_models_pos;
        std::set<unsigned int> deleted_plasm_balls_pos;
        auto model_attributes_begin = model_attributes.begin();
        auto plasm_ball_attributes_begin = plasm_ball_attributes.begin();

        for (unsigned int i = 0; i < model_attributes.size(); i++) {
            for (unsigned int j = 0; j < plasm_ball_attributes.size(); j++) {
                if (glm::distance(model_attributes[i].real_coords,
                                  plasm_ball_attributes[j].real_coords) <=
                        dist) {

                    deleted_models_pos.insert(i);
                    deleted_plasm_balls_pos.insert(j);

                    explosion_attributes.push_back(
                        {
                            current_frame,
                            model_attributes[i].real_coords
                        });
                }
            }
        }

        for (auto it = deleted_models_pos.rbegin();
                it != deleted_models_pos.rend(); ++it) {

            model_attributes.erase(
                    model_attributes_begin + *it);
        }

        for (auto it = deleted_plasm_balls_pos.rbegin();
                it != deleted_plasm_balls_pos.rend(); ++it) {

            plasm_ball_attributes.erase(
                    plasm_ball_attributes_begin + *it);
        }

        for (auto &it: explosion_attributes) {
            draw_exploison(sphere_model, it);
        }

        draw_skybox();

        RenderText(text_program,
                   "+",
                   /*545.0f,
                   400.0f,*/
                   555.0f,
                   415.0f,
                   0.5f,
                   glm::vec3(1.0f, 1.0f, 1.0f));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    /*std::cout << "iks = " << iks << std::endl;
    std::cout << "igrec = " << igrec << std::endl;
    std::cout << "dist = " << dist << std::endl;*/

    glfwTerminate();
    return 0;
}
