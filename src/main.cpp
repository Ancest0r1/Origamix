#include "file_dialog.hpp"
#include "objx.hpp"
#include "metaUser.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengles2.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;
namespace fs = std::filesystem;

const int WIDTH = 800;
const int HEIGHT = 600;

const char* vertexShaderSrc = R"(
    #version 100
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;
    attribute vec4 vPosition;
    uniform float angleX;
    uniform float angleY;
    uniform float offsetX;
    uniform float offsetY;
    uniform float scale;
    void main() {
        float cosX = cos(angleX);
        float sinX = sin(angleX);
        float cosY = cos(angleY);
        float sinY = sin(angleY);
        vec4 pos = vPosition;

        float y = pos.y * cosX - pos.z * sinX;
        float z = pos.y * sinX + pos.z * cosX;
        float x = pos.x * cosY + z * sinY;
        z = -pos.x * sinY + z * cosY;

        gl_Position = vec4(x*scale + offsetX, y*scale + offsetY, z*scale, z + 2.0);
        vTexCoord = aTexCoord;
    }
)";

const char* fragmentShaderSrc = R"(
    #version 100
    precision mediump float;
    uniform sampler2D tex;
    varying vec2 vTexCoord;
    void main() {
        gl_FragColor = texture2D(tex, vTexCoord);
    }
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        cerr << "[Shader Compilation Error] " << log << endl;
    }
    return shader;
}

GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glBindAttribLocation(program, 0, "vPosition");
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        cerr << "[Program Link Error] " << log << endl;
    }
    return program;
}

GLuint loadTexture(const string& filename) {
    cout << "[loadTexture] Chargement de " << filename << endl;
    SDL_Surface* surface = IMG_Load(filename.c_str());
    if (!surface) {
        cerr << "Erreur chargement texture : " << IMG_GetError() << endl;
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLenum format = surface->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    SDL_FreeSurface(surface);

    cout << "[loadTexture] réussi! " << endl;
    return texID;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_Window* window = SDL_CreateWindow("Pilonix Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    MetaUser& metaUser = MetaUser::instance();
    // metaUser.startRecording("scenario.rec");
    // metaUser.replay("scenario.rec");

    GLuint program = createProgram();
    glUseProgram(program);
    GLint texLoc = glGetUniformLocation(program, "tex");
    glUniform1i(texLoc, 0);
    GLint angleXLoc = glGetUniformLocation(program, "angleX");
    GLint angleYLoc = glGetUniformLocation(program, "angleY");
    GLint scaleLoc  = glGetUniformLocation(program, "scale");
    GLint offsetXLoc = glGetUniformLocation(program, "offsetX");
    GLint offsetYLoc = glGetUniformLocation(program, "offsetY");

    float angleX = 0, angleY = 0, scale = 1.0f;
    float offsetX = 0, offsetY = 0;

    vector<Objx> Objxs;
    map<string, GLuint> textureIDs;

    for (const auto& entry : fs::directory_iterator("assets")) {
        if (entry.path().extension() == ".plxl") {
            Objx p = Objx::open(entry.path().string());
            Objxs.push_back(p);
        }
    }

    for (auto& p : Objxs) {
        for (auto& s : p.getSurfaces()) {
            string tex = fs::path(s.texture).filename().string();
            if (textureIDs.count(tex) == 0) {
                string path = "assets/temp_extract/" + tex;
                textureIDs[tex] = loadTexture(path);
            }
        }
    }

    glViewport(-2*WIDTH, -2*HEIGHT, 5*WIDTH, 5*HEIGHT);
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            metaUser.handleEvent(event);
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                bool ctrl = (event.key.keysym.mod & KMOD_CTRL);
                switch (event.key.keysym.sym) {
                    case SDLK_UP:    ctrl ? angleX += 0.1f : offsetY -= 0.1f; break;
                    case SDLK_DOWN:  ctrl ? angleX -= 0.1f : offsetY += 0.1f; break;
                    case SDLK_LEFT:  ctrl ? angleY += 0.1f : offsetX += 0.1f; break;
                    case SDLK_RIGHT: ctrl ? angleY -= 0.1f : offsetX -= 0.1f; break;
                    case SDLK_PLUS:
                    case SDLK_EQUALS: scale *= 1.1f; break;
                    case SDLK_MINUS:  scale /= 1.1f; break;

                    case SDLK_n: {
                        string path = ouvrirBoiteFichier(false);
                        if (!path.empty()) {
                            Objx p = Objx::buildFromPNG(path);
                            SDL_GL_MakeCurrent(window, context);
                            for (const auto& s : p.getSurfaces()) {
                                string tex = fs::path(s.texture).filename().string();
                                if (textureIDs.count(tex) == 0) {
                                    textureIDs[tex] = loadTexture(s.texture);
                                }
                            }
                            Objxs.push_back(p);
                        }
                        break;
                    }
                }
            }
        }

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1f(angleXLoc, angleX);
        glUniform1f(angleYLoc, angleY);
        glUniform1f(scaleLoc, scale);
        glUniform1f(offsetXLoc, offsetX);
        glUniform1f(offsetYLoc, offsetY);

        for (auto& p : Objxs) {
            for (auto& s : p.getSurfaces()) {
                string tex = fs::path(s.texture).filename().string();
                glBindTexture(GL_TEXTURE_2D, textureIDs[tex]);

                const auto& pts = s.points;
                for (size_t i = 0; i + 2 < pts.size(); i += 3) {
                    float tri[15] = {
                        pts[i].x, pts[i].y, pts[i].z, pts[i].u, pts[i].v,
                        pts[i+1].x, pts[i+1].y, pts[i+1].z, pts[i+1].u, pts[i+1].v,
                        pts[i+2].x, pts[i+2].y, pts[i+2].z, pts[i+2].u, pts[i+2].v
                    };

                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), tri);
                    glEnableVertexAttribArray(0);
                    GLint texCoordLoc = glGetAttribLocation(program, "aTexCoord");
                    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), tri + 3);
                    glEnableVertexAttribArray(texCoordLoc);

                    glDrawArrays(GL_TRIANGLES, 0, 3);
                }
            }
        }

        SDL_GL_SwapWindow(window);
    }

    metaUser.stopRecording();
    metaUser.stopReplay();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
