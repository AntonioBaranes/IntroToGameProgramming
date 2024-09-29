#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

/**
* Author: Antonio Baranes
* Assignment: Simple 2D Scene
* Date due: 2024-09-28, 11:00pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
LEVEL_OF_DETAIL = 0, // mipmap reduction image level
TEXTURE_BORDER = 0; // this value MUST be zero

//Most sprites aquierd from spriters-resource.com
constexpr char BACKGROUND_FILEPATH[] = "bg_lavos.png",
    LAVOS_FILEPATH[] = "sprt_lavos.png",
    LAVOS_POD_LEFT_FILEPATH[] = "sprt_lavos_pod_left.png",
    LAVOS_POD_RIGHT_FILEPATH[] = "sprt_lavos_pod_right.png",
    CHRONO_FILEPATH[] = "sprt_chrono.png",
    MARLE_FILEPATH[] = "sprt_marle.png",
    FROG_FILEPATH[] = "sprt_frog.png";

constexpr glm::vec3 INIT_SCALE_BACKGROUND = glm::vec3(500.0f, 500.98f, 0.0f),
    INIT_POS_BACKGROUND = glm::vec3(0.0f, 0.0f, 0.0f),
    INIT_POS_LAVOS = glm::vec3(0.0f, 3.0f, 0.0f),
    INIT_POS_MARLE = glm::vec3(-2.5f, -1.5f, 0.0f),
    INIT_POS_CHRONO = glm::vec3(0.0f, -1.5f, 0.0f),
    INIT_POS_FROG = glm::vec3(2.5f, -1.5f, 0.0f),
    INIT_POS_LAVOS_POD_RIGHT = glm::vec3(-2.5f, 0.5f, 0.0f),
    INIT_POS_LAVOS_POD_LEFT = glm::vec3(2.5f, 0.5f, 0.0f),
    INIT_SCALE_FROG = glm::vec3(1.80f, 1.22f, 0.0f),
    INIT_SCALE_LAVOS = glm::vec3(12.0f, 6.72f, 0.0f),
    INIT_SCALE_MARLE = glm::vec3(2.5f, 1.25f, 0.0f),
    INIT_SCALE_LAVOS_POD_RIGHT = glm::vec3(1.25f, 1.5f, 0.0f),
    INIT_SCALE_LAVOS_POD_LEFT = glm::vec3(1.25f, 1.5f, 0.0f),
    INIT_SCALE_CHRONO = glm::vec3(1.6f, 2.3f, 0.0f);

glm::vec3 chrono_rotation_matrix = glm::vec3(0.0f, 0.0f, 0.0f);

constexpr float ROT_INCREMENT = 4.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();


//model matrix for objects
glm::mat4 g_view_matrix,
g_background_matrix,
g_projection_matrix,
g_lavos_matrix,
g_marle_matrix,
g_chrono_matrix,
g_left_matrix,
g_right_matrix,
g_frog_matrix,
//position and movement
g_movement_pods,
g_position_pods;




float g_previous_ticks = 0.0f;

//ids for textures
GLuint g_background_id,
       g_lavos_id,
       g_chrono_id,
       g_frog_id,
       g_left_id,
       g_right_id,
       g_marle_id;

//variable counter for pulsing and movement
float g_lavos_pulse = 0.0f;
float g_lavos_pods_mov = 0.0f;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Simple2D",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_background_matrix = glm::mat4(1.0f);
    g_lavos_matrix = glm::mat4(1.0f);
    g_chrono_matrix = glm::mat4(1.0f);
    g_marle_matrix = glm::mat4(1.0f);
    g_frog_matrix = glm::mat4(1.0f);
    g_left_matrix = glm::mat4(1.0f);
    g_right_matrix = glm::mat4(1.0f);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_background_id = load_texture(BACKGROUND_FILEPATH);
    g_lavos_id = load_texture(LAVOS_FILEPATH);
    g_chrono_id = load_texture(CHRONO_FILEPATH);
    g_marle_id = load_texture(MARLE_FILEPATH);
    g_frog_id = load_texture(FROG_FILEPATH);
    g_left_id = load_texture(LAVOS_POD_LEFT_FILEPATH);
    g_right_id = load_texture(LAVOS_POD_RIGHT_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}

//Scale Background outside of update


void update()
{
    /* Delta time calculations */
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;



    /* Game logic */
    g_lavos_pulse += 1.5f * delta_time;
    g_lavos_pods_mov += 4.0f * delta_time;

    float hori_mov = 0.1*glm::cos(g_lavos_pods_mov);
    float verti_mov = 0.1*glm::sin(g_lavos_pods_mov);

    chrono_rotation_matrix.y += ROT_INCREMENT * delta_time;

    /* Model matrix reset */
    g_background_matrix = glm::mat4(1.0f);
    g_lavos_matrix = glm::mat4(1.0f);
    g_chrono_matrix = glm::mat4(1.0f);
    g_marle_matrix = glm::mat4(1.0f);
    g_frog_matrix = glm::mat4(1.0f);
    g_left_matrix = glm::mat4(1.0f);
    g_right_matrix = glm::mat4(1.0f);
    /* Transformations */

    g_background_matrix = glm::translate(g_background_matrix, INIT_POS_BACKGROUND);
    g_lavos_matrix = glm::translate(g_lavos_matrix, INIT_POS_LAVOS);
    g_chrono_matrix = glm::translate(g_chrono_matrix, INIT_POS_CHRONO);
    g_marle_matrix = glm::translate(g_marle_matrix, INIT_POS_MARLE);
    g_frog_matrix = glm::translate(g_frog_matrix, INIT_POS_FROG);
    g_right_matrix = glm::translate(g_right_matrix, INIT_POS_LAVOS_POD_RIGHT);
    g_left_matrix = glm::translate(g_left_matrix, INIT_POS_LAVOS_POD_LEFT);

    //lavos pods cirle code
    //I hope this is what they mean when they say in relation to eachother :/
    //I mean the distance between them is always constant so that means they're relative... I think...
    glm::vec3 rightpod_mov_vec = glm::vec3(hori_mov, verti_mov, 0.0f);
    glm::vec3 leftpod_mov_vec = glm::vec3(rightpod_mov_vec.x, rightpod_mov_vec.y, 0.0f); //takes same transformation as rightpod
    g_right_matrix = glm::translate(g_right_matrix, rightpod_mov_vec);
    g_left_matrix = glm::translate(g_left_matrix, leftpod_mov_vec);
    //I've realized I didn't need to make a leftpod_mov_vec and could've just used the same one as the right...
    //but im too afraid to break it :////

    //chrono getting obliterated rotation
    g_chrono_matrix = glm::rotate(g_chrono_matrix,
        chrono_rotation_matrix.y,
        glm::vec3(0.0f, 1.0f, 0.0f));

    g_background_matrix = glm::scale(g_background_matrix, glm::vec3(10.5f, 8.5f, 0.0));
    g_lavos_matrix = glm::scale(g_lavos_matrix, INIT_SCALE_LAVOS);
    g_chrono_matrix = glm::scale(g_chrono_matrix, INIT_SCALE_CHRONO);
    g_marle_matrix = glm::scale(g_marle_matrix, INIT_SCALE_MARLE);
    g_frog_matrix = glm::scale(g_frog_matrix, INIT_SCALE_FROG);
    g_right_matrix = glm::scale(g_right_matrix, INIT_SCALE_LAVOS_POD_RIGHT);
    g_left_matrix = glm::scale(g_left_matrix, INIT_SCALE_LAVOS_POD_LEFT);

    //lavos pulsation code
    glm::vec3 lavos_scaler = glm::vec3(1+0.030*glm::sin(g_lavos_pulse), 1+0.030*glm::sin(g_lavos_pulse), 0.0f);
    g_lavos_matrix = glm::scale(g_lavos_matrix, lavos_scaler);

    
    
}


void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_background_matrix, g_background_id);
    draw_object(g_lavos_matrix, g_lavos_id);
    draw_object(g_chrono_matrix, g_chrono_id);
    draw_object(g_marle_matrix, g_marle_id);
    draw_object(g_frog_matrix, g_frog_id);
    draw_object(g_left_matrix, g_left_id);
    draw_object(g_right_matrix, g_right_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}