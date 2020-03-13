#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"
#include <vector>

#define PLATFORM_COUNT 4
#define ROCK_COUNT 35

struct GameState {
    Entity* player;
    Entity* rocks;
    Entity* platforms;
};

GameState state;

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint rockTextureID;
GLuint waterTextureID;
GLuint platformTextureID;
GLuint fontTextureID;

GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);
    return textureID;
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Lunar Lander!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    glUseProgram(program.programID);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize Player
    state.player = new Entity();
    state.player->position = glm::vec3(0, 3.25f, 0);
    state.player->movement = glm::vec3(0);
    state.player->acceleration = glm::vec3(0, -0.02f, 0);
    state.player->speed = 1.5f;
    state.player->textureID = LoadTexture("playerShip1_blue.png");

    state.player->height = 0.95f;
    state.player->width = 0.95f;

    //The sides and water are all treated as rocks
    state.platforms = new Entity[PLATFORM_COUNT];
    state.rocks = new Entity[ROCK_COUNT];

    rockTextureID = LoadTexture("platformPack_tile016.png");
    waterTextureID = LoadTexture("platformPack_tile017.png");
    platformTextureID = LoadTexture("platformPack_tile013.png");
    fontTextureID = LoadTexture("font1.png");

    float rockLeft_y1 = 3.25f;
    float rockRight_y1 = 3.25f;
    float platform_x1 = 0.50f;
    float water_x1 = -3.50f;

    for (int i = 0; i < 12; i++) {
        state.rocks[i].textureID = rockTextureID;
        state.rocks[i].position = glm::vec3(-4.50f, rockLeft_y1, 0);
        rockLeft_y1 -= 1;
    }
    for (int i = 12; i < 24; i++) {
        state.rocks[i].textureID = rockTextureID;
        state.rocks[i].position = glm::vec3(4.50f, rockRight_y1, 0);
        rockRight_y1 -= 1;
    }

    for (int j = 0; j < 3; j++) {
        state.platforms[j].textureID = platformTextureID;
        state.platforms[j].position = glm::vec3(platform_x1, -3.00f, 0);
        platform_x1 += 1;
    }

    for (int i = 24; i < 32; i++) {
        state.rocks[i].textureID = waterTextureID;
        state.rocks[i].position = glm::vec3(water_x1, -3.25f, 0);
        water_x1 += 1;
    }

    state.rocks[32].textureID = rockTextureID;
    state.rocks[32].position = glm::vec3(0, 0, 0);

    state.rocks[33].textureID = rockTextureID;
    state.rocks[33].position = glm::vec3(-3.50f, 0, 0);

    state.rocks[34].textureID = rockTextureID;
    state.rocks[34].position = glm::vec3(3.50f, 0, 0);

    GLuint collidedWithX = NULL;
    GLuint collidedWithY = NULL;

    for (int i = 0; i < PLATFORM_COUNT; i++) {
        state.platforms[i].Update(0, NULL, 0, collidedWithX, collidedWithY);
    }
    for (int i = 0; i < ROCK_COUNT; i++) {
        state.rocks[i].Update(0, NULL, 0, collidedWithX, collidedWithY);
    }
}

void ProcessInput() {
    state.player->movement = glm::vec3(0);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                gameIsRunning = false;
                break;
            }
            break; // SDL_KEYDOWN
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (state.player->isFrozen == false) {
        if (keys[SDL_SCANCODE_LEFT]) {
            state.player->acceleration.x -= 0.15;
        }
        else if (keys[SDL_SCANCODE_RIGHT]) {
            state.player->acceleration.x += 0.15;
        }
        if (glm::length(state.player->movement) > 1.0f) {
            state.player->movement = glm::normalize(state.player->movement);
        }
    }
}

void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position)
{
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;

    std::vector<float> vertices;
    std::vector<float> texCoords;

    for (int i = 0; i < text.size(); i++) {

        int index = (int)text[i];
        float offset = (size + spacing) * i;

        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;

        vertices.insert(vertices.end(), {
            offset + (-0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
            });

        texCoords.insert(texCoords.end(), {
        u, v,
        u, v + height,
        u + width, v,
        u + width, v + height,
        u + width, v,
        u, v + height,
        });
    } // end of for loop
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);

    glUseProgram(program->programID);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    deltaTime += accumulator;
    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        GLuint collidedWithX = NULL;
        GLuint collidedWithY = NULL;

        if (state.player->isFrozen == false) {
            state.player->Update(FIXED_TIMESTEP, state.platforms, PLATFORM_COUNT, collidedWithX, collidedWithY);
            state.player->Update(FIXED_TIMESTEP, state.rocks, ROCK_COUNT, collidedWithX, collidedWithY);

            if (collidedWithX == rockTextureID || collidedWithY == rockTextureID ||
                collidedWithX == waterTextureID || collidedWithY == waterTextureID) {
                state.player->isFrozen = true;
                state.player->hasWon = "No";
                state.player->movement = glm::vec3(0);
            }

            if (collidedWithX == platformTextureID || collidedWithY == platformTextureID) {
                state.player->isFrozen = true;
                state.player->hasWon = "Yes";
                state.player->movement = glm::vec3(0);
            }
        }
        deltaTime -= FIXED_TIMESTEP;
    }
    accumulator = deltaTime;
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < PLATFORM_COUNT; i++) {
        state.platforms[i].Render(&program);
    }

    for (int i = 0; i < ROCK_COUNT; i++) {
        state.rocks[i].Render(&program);
    }

    fontTextureID = LoadTexture("font1.png");

    if (state.player->hasWon == "Yes") {
        DrawText(&program, fontTextureID, "Mission Successful!", 0.65f, -0.25f, glm::vec3(-3.5f, 2.0f, 0));
        state.player->isFrozen = true;
    }
    if (state.player->hasWon == "No") {
        DrawText(&program, fontTextureID, "Mission Failed!", 0.7f, -0.25f, glm::vec3(-3.0f, 2.0f, 0));
        state.player->isFrozen = true;
    }

    state.player->Render(&program);

    SDL_GL_SwapWindow(displayWindow);
}

void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();

    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }

    Shutdown();
    return 0;
}