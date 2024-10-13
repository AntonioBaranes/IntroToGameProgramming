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


enum Mode { ONEPLAYER, TWOPLAYER };

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

//textures
constexpr char BALL_SPRITE_FILEPATH[] = "ball.png",
BAR_SPRITE_FILE_PATH[] = "bar.png",
BACKGROUND_FILE_PATH[] = "bg_black.png";

glm::vec3 INIT_SCALE_BACKGROUND = glm::vec3(10.0f, 10.0f, 0.0f),
INIT_SCALE_BAR = glm::vec3(0.5f, 1.75f, 0.0f),
INIT_SCALE_BALL = glm::vec3(0.25f, 0.25f, 0.0f),
INIT_POS_BAR_ONE = glm::vec3(3.5f, 0.0f, 0.0f),
INIT_POS_BACKGROUND = glm::vec3(2.0f, 0.0f, 0.0f),
INIT_POS_BAR_TWO = glm::vec3(-3.5f, 0.0f, 0.0f),
INIT_POS_BALL = glm::vec3(0.0f, 0.0f, 0.0f);

//Movement vecs for bars #using init positions as position
glm::vec3 g_bar_one_movement,
g_bar_two_movement,
g_ball_movement;

float bar_speed = 2.5f,
ball_speed_x = 2.75f,
ball_speed_y = 1.50f;


constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
Mode game_mode = TWOPLAYER;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
g_bar_one_matrix,
g_bar_two_matrix,
g_background_matrix,
g_ball_matrix,
g_projection_matrix;


float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_kimi = glm::vec3(0.0f, 0.0f, 0.0f),
g_rotation_totsuko = glm::vec3(0.0f, 0.0f, 0.0f);



GLuint g_bar_one_texture_id,
g_bar_two_texture_id,
g_background_texture_id,
g_ball_texture_id;


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

    g_display_window = SDL_CreateWindow("Hello, Textures!",
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

    g_bar_one_matrix = glm::mat4(1.0f);
    g_bar_two_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_bar_one_texture_id = load_texture(BAR_SPRITE_FILE_PATH);
    g_bar_two_texture_id = load_texture(BAR_SPRITE_FILE_PATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    g_background_texture_id = load_texture(BACKGROUND_FILE_PATH);

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
        if (event.type == SDL_KEYDOWN) {
            // Which key was pressed?
            switch (event.key.keysym.sym) {
            case SDLK_t: //switch modes
                if (game_mode == TWOPLAYER) {
                    game_mode = ONEPLAYER;
                }
                else if (game_mode == ONEPLAYER) {
                    game_mode = TWOPLAYER;
                }
                break;
            default:
                break;
            }
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL); // if non-NULL, receives the length of the returned array

    if (key_state[SDL_SCANCODE_UP])
    {
        g_bar_one_movement.y += 1;
    }
    if (key_state[SDL_SCANCODE_DOWN])
    {
        g_bar_one_movement.y -= 1;
    }
    if (key_state[SDL_SCANCODE_W])
    {
        if (game_mode == TWOPLAYER)
            g_bar_two_movement.y += 1;
        

    }
    if (key_state[SDL_SCANCODE_S])
    {
        if (game_mode == TWOPLAYER)
            g_bar_two_movement.y -= 1;
        
    }
}


void update()
{
    /* Delta time calculations */
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    /* Model matrix reset */
    g_bar_one_matrix = glm::mat4(1.0f);
    g_bar_two_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);

    //ball movement
    g_ball_movement.x += 1;
    g_ball_movement.y += 1;

    if (game_mode == ONEPLAYER){
        INIT_POS_BAR_TWO.y = INIT_POS_BALL.y;
    }
    
    INIT_POS_BAR_ONE += g_bar_one_movement * delta_time * bar_speed;
    INIT_POS_BAR_TWO += g_bar_two_movement * delta_time * bar_speed;


    if (glm::length(g_ball_movement) > 1.0f)
    {
        g_ball_movement = glm::normalize(g_ball_movement);
    }
    INIT_POS_BALL.x += g_ball_movement.x * delta_time * ball_speed_x;
    INIT_POS_BALL.y += g_ball_movement.y * delta_time * ball_speed_y;



    g_background_matrix = glm::translate(g_background_matrix, INIT_POS_BACKGROUND);
    g_bar_one_matrix = glm::translate(g_bar_one_matrix, INIT_POS_BAR_ONE);
    g_bar_two_matrix = glm::translate(g_bar_two_matrix, INIT_POS_BAR_TWO);
    g_ball_matrix = glm::translate(g_ball_matrix, INIT_POS_BALL);

    g_ball_matrix = glm::scale(g_ball_matrix, INIT_SCALE_BALL);
    g_background_matrix = glm::scale(g_background_matrix, INIT_SCALE_BACKGROUND);
    g_bar_one_matrix = glm::scale(g_bar_one_matrix, INIT_SCALE_BAR);    
    g_bar_two_matrix = glm::scale(g_bar_two_matrix, INIT_SCALE_BAR);

    //reseting movements
    g_bar_one_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    g_bar_two_movement = glm::vec3(0.0f, 0.0f, 0.0f);

    //collision
    float x_distance_one = fabs(INIT_POS_BALL.x - INIT_POS_BAR_ONE.x) -
        ((INIT_SCALE_BALL.x + INIT_SCALE_BAR.x) / 2.0f);
    float y_distance_one = fabs(INIT_POS_BALL.y - INIT_POS_BAR_ONE.y) -
        ((INIT_SCALE_BALL.y + INIT_SCALE_BAR.y) / 2.0f);

    float x_distance_two = fabs(INIT_POS_BALL.x - INIT_POS_BAR_TWO.x) -
        ((INIT_SCALE_BALL.x + INIT_SCALE_BAR.x) / 2.0f);

    float y_distance_two = fabs(INIT_POS_BALL.y - INIT_POS_BAR_TWO.y) -
        ((INIT_SCALE_BALL.y + INIT_SCALE_BAR.y) / 2.0f);

    if (INIT_POS_BALL.y >= 3.6f || INIT_POS_BALL.y <= -3.6f) {
        ball_speed_y *= -1; 
    }

    if (x_distance_two <= 0 && y_distance_two <= 0) {
        ball_speed_x *= -1;
    }
    if (x_distance_one <= 0 && y_distance_one <= 0) {
        ball_speed_x *= -1;
    }

    if (INIT_POS_BALL.x <= -5 || INIT_POS_BALL.x >= 5) {
        ball_speed_x *= -1; 
        g_app_status = TERMINATED;
    }
    
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

    draw_object(g_background_matrix, g_background_texture_id);
    draw_object(g_bar_one_matrix, g_bar_one_texture_id);
    draw_object(g_bar_two_matrix, g_bar_two_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();
    std::cout << "running";

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}