// main.cpp
#include "alglin.hpp"
#include "file_dialog.hpp"
#include "objx.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengles2.h>
#include <SDL2/SDL_ttf.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <map>

using namespace std;
namespace fs = std::filesystem;

const int WIDTH = 800;
const int HEIGHT = 600;

const char* vertexShaderSrc = R"(
    #version 100
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;
    attribute vec4 vPosition;
    uniform mat3 rotationMatrix;
    uniform float offsetX;
    uniform float offsetY;
    uniform float scale;
    void main() {
        vec3 rotated = rotationMatrix * vPosition.xyz;
        gl_PointSize = 10.0;
        gl_Position = vec4(rotated.x * scale + offsetX, rotated.y * scale + offsetY, rotated.z * scale, rotated.z + 2.0);
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
    return texID;
}

bool rayIntersectsTriangle(const Vec3& orig, const Vec3& dir,
                           const Vec3& v0, const Vec3& v1, const Vec3& v2,
                           float& tOut) {
    const float EPS = 1e-6f;
    Vec3 e1 = v1 - v0;
    Vec3 e2 = v2 - v0;
    Vec3 h = cross(dir, e2);
    float a = dot(e1, h);
    if (fabs(a) < EPS) return false;
    float f = 1.0f / a;
    Vec3 s = orig - v0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;
    Vec3 q = cross(s, e1);
    float v = f * dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;
    float t = f * dot(e2, q);
    if (t < EPS) return false;
    tOut = t;
    return true;
}

void createTextTexture(TTF_Font* font, const string& text, GLuint& tex,
                       int& w, int& h) {
    SDL_Color black = {0, 0, 0};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), black);
    if (!surf) {
        w = h = 0;
        return;
    }
    if (tex == 0) glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLenum fmt = surf->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, surf->w, surf->h, 0,
                 fmt, GL_UNSIGNED_BYTE, surf->pixels);
    w = surf->w;
    h = surf->h;
    SDL_FreeSurface(surf);
}

void drawText(GLuint program, GLuint tex, int texW, int texH, int x, int y) {
    if (tex == 0 || texW == 0 || texH == 0) return;

    float x1 = 4.0f * x / WIDTH - 2.0f;
    float y1 = -4.0f * y / HEIGHT + 2.0f;
    float x2 = 4.0f * (x + texW) / WIDTH - 2.0f;
    float y2 = -4.0f * (y + texH) / HEIGHT + 2.0f;

    float quad[30] = {
        x1, y1, 0.0f, 0.0f, 0.0f,
        x2, y1, 0.0f, 1.0f, 0.0f,
        x2, y2, 0.0f, 1.0f, 1.0f,
        x1, y1, 0.0f, 0.0f, 0.0f,
        x2, y2, 0.0f, 1.0f, 1.0f,
        x1, y2, 0.0f, 0.0f, 1.0f
    };

    glBindTexture(GL_TEXTURE_2D, tex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), quad);
    glEnableVertexAttribArray(0);
    GLint texCoordLoc = glGetAttribLocation(program, "aTexCoord");
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), quad+3);
    glEnableVertexAttribArray(texCoordLoc);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!font) {
        cerr << "Erreur chargement police: " << TTF_GetError() << endl;
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_Window* window = SDL_CreateWindow("Pilonix Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    GLuint program = createProgram();
    glUseProgram(program);
    GLint texLoc = glGetUniformLocation(program, "tex");
    glUniform1i(texLoc, 0);
    GLint scaleLoc  = glGetUniformLocation(program, "scale");
    GLint offsetXLoc = glGetUniformLocation(program, "offsetX");
    GLint offsetYLoc = glGetUniformLocation(program, "offsetY");
    GLint rotLoc = glGetUniformLocation(program, "rotationMatrix");

    float angleX = 0, angleY = 0, scale = 1.0f;
    float offsetX = 0, offsetY = 0;

    vector<Objx> Objxs;
    map<string, GLuint> textureIDs;
    GLuint textTex = 0;
    int textW = 0, textH = 0;

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
                }
            }
        }

        Mat3 rotation = Mat3::rotXY(angleX, angleY);

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1f(scaleLoc, scale);
        glUniform1f(offsetXLoc, offsetX);
        glUniform1f(offsetYLoc, offsetY);
        glUniformMatrix3fv(rotLoc, 1, GL_FALSE, rotation.data());

        // calcul des repères écran et position curseur
        Vec3 unitX = rotation * Vec3(1, 0, 0);
        Vec3 unitY = rotation * Vec3(0, 1, 0);
        Vec3 unitZ = rotation * Vec3(0, 0, 1);

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        float screenX = ((mouseX - WIDTH / 2.0f) / (WIDTH / 2.0f) - offsetX) / scale;
        float screenY = -((mouseY - HEIGHT / 2.0f) / (HEIGHT / 2.0f)  - offsetY) / scale;
        Vec3 base = unitX * screenX + unitY * screenY;

        int triangleCount = 0;
        int hoveredTriangle = -1;
        float closestT = 1e9f;
        for (auto& p : Objxs) {
            for (auto& s : p.getSurfaces()) {
                string tex = fs::path(s.texture).filename().string();
                glBindTexture(GL_TEXTURE_2D, textureIDs[tex]);
                const auto& pts = s.points;
                for (size_t i = 0; i + 2 < pts.size(); i += 3) {
                    float tri[15];
                    for (int j = 0; j < 3; ++j) {
                        tri[j*5+0] = pts[i+j].x;
                        tri[j*5+1] = pts[i+j].y;
                        tri[j*5+2] = pts[i+j].z;
                        tri[j*5+3] = pts[i+j].u;
                        tri[j*5+4] = pts[i+j].v;
                    }
                    Vec3 v0 = rotation * Vec3(pts[i].x, pts[i].y, pts[i].z);
                    Vec3 v1 = rotation * Vec3(pts[i+1].x, pts[i+1].y, pts[i+1].z);
                    Vec3 v2 = rotation * Vec3(pts[i+2].x, pts[i+2].y, pts[i+2].z);
                    float tHit;
                    if (rayIntersectsTriangle(base, unitZ, v0, v1, v2, tHit)) {
                        if (tHit < closestT) {
                            closestT = tHit;
                            hoveredTriangle = triangleCount;
                        }
                    }

                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), tri);
                    glEnableVertexAttribArray(0);
                    GLint texCoordLoc = glGetAttribLocation(program, "aTexCoord");
                    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), tri + 3);
                    glEnableVertexAttribArray(texCoordLoc);
                    glDrawArrays(GL_TRIANGLES, 0, 3);

                    ++triangleCount;
                }
            }
        }
        
                // ligne entre z=0 et z=5
        Vec3 p0 = base;
        Vec3 p1 = base + unitZ * 5.0f;
        float line[10] = {
            p0.x, p0.y, p0.z, 0.5f, 0.5f,
            p1.x, p1.y, p1.z, 0.5f, 0.5f
        };
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), line);
        glEnableVertexAttribArray(0);
        GLint texCoordLoc = glGetAttribLocation(program, "aTexCoord");
        glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), line + 3);
        glEnableVertexAttribArray(texCoordLoc);
        glLineWidth(4.0f);
        glDrawArrays(GL_LINES, 0, 2);

        string info = "Triangles: " + to_string(triangleCount);
        if (hoveredTriangle >= 0)
            info += "  Hit: " + to_string(hoveredTriangle);
        createTextTexture(font, info, textTex, textW, textH);

        glUniform1f(scaleLoc, 1.0f);
        glUniform1f(offsetXLoc, 0.0f);
        glUniform1f(offsetYLoc, 0.0f);
        Mat3 id = Mat3::identity();
        glUniformMatrix3fv(rotLoc, 1, GL_FALSE, id.data());
        drawText(program, textTex, textW, textH, 10, 10);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
