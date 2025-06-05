// Objx_builder.cpp
#include "objx.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <zip.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <filesystem>
#include <sstream>

using namespace std;
namespace fs = filesystem;

Objx px;
vector<bool> selectedTriangles;

void decoupage() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        cerr << "Erreur SDL_Init : " << SDL_GetError() << endl;
        return;
    }
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* win = SDL_CreateWindow("Découpage", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    auto& points = px.getSurfaces()[0].points;
    SDL_Surface* surface = IMG_Load(px.getSurfaces()[0].texture.c_str());
    if (!surface) {
        cerr << "Erreur IMG_Load : " << IMG_GetError() << endl;
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    int imgW = surface->w;
    int imgH = surface->h;
    SDL_FreeSurface(surface);

    int offsetX = 0, offsetY = 0;
    bool running = true;
    const float selectionRadius = 10.0f;

    while (running) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        int relMouseX = mouseX - offsetX;
        int relMouseY = mouseY - offsetY;

        int hoveredIndex = -1;
        for (size_t i = 0; i < points.size(); ++i) {
            float dx = points[i].x - relMouseX;
            float dy = points[i].y - relMouseY;
            if (sqrt(dx * dx + dy * dy) < selectionRadius) {
                hoveredIndex = i;
                break;
            }
        }

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_LEFT: offsetX -= 10; break;
                    case SDLK_RIGHT: offsetX += 10; break;
                    case SDLK_UP: offsetY -= 10; break;
                    case SDLK_DOWN: offsetY += 10; break;
                    case SDLK_RETURN: {
                        px.save();
                        running = false;
                        cout << "[Objx créé] " << px.toString();
                        break;
                    }
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                if (hoveredIndex != -1) {
                    points.push_back(points[hoveredIndex]);
                } else {
                    int x = e.button.x - offsetX;
                    int y = e.button.y - offsetY;
                    float u = x / (float)imgW;
                    float v = y / (float)imgH;
                    points.push_back({(float)x, (float)y, 0.0f, u, v});
                }
                if (points.size() % 3 == 0) selectedTriangles.push_back(false);
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
                for (size_t i = 0; i + 2 < points.size(); i += 3) {
                    float cx = (points[i].x + points[i+1].x + points[i+2].x) / 3 + offsetX;
                    float cy = (points[i].y + points[i+1].y + points[i+2].y) / 3 + offsetY;
                    float dx = mouseX - cx;
                    float dy = mouseY - cy;
                    if (sqrt(dx * dx + dy * dy) < 15.0f) {
                        if (i / 3 < selectedTriangles.size())
                            selectedTriangles[i / 3] = !selectedTriangles[i / 3];
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderClear(renderer);
        SDL_Rect dst = {offsetX, offsetY, imgW, imgH};
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        for (size_t i = 0; i + 2 < points.size(); i += 3) {
            SDL_Vertex verts[3];
            for (int j = 0; j < 3; ++j) {
                verts[j].position.x = points[i + j].x + offsetX;
                verts[j].position.y = points[i + j].y + offsetY;
                if (i / 3 < selectedTriangles.size() && selectedTriangles[i / 3]) {
                    verts[j].color = {100, 100, 255, 120};
                } else {
                    verts[j].color = {255, 255, 255, 100};
                }
            }
            SDL_RenderGeometry(renderer, nullptr, verts, 3, nullptr, 0);
        }

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (size_t i = 0; i < points.size(); ++i) {
            SDL_Rect r = {(int)(points[i].x + offsetX - 2), (int)(points[i].y + offsetY - 2), 5, 5};
            if ((int)i == hoveredIndex) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderFillRect(renderer, &r);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            } else {
                SDL_RenderFillRect(renderer, &r);
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        for (size_t i = 0; i + 2 < points.size(); i += 3) {
            SDL_RenderDrawLine(renderer, points[i].x + offsetX, points[i].y + offsetY, points[i+1].x + offsetX, points[i+1].y + offsetY);
            SDL_RenderDrawLine(renderer, points[i+1].x + offsetX, points[i+1].y + offsetY, points[i+2].x + offsetX, points[i+2].y + offsetY);
            SDL_RenderDrawLine(renderer, points[i+2].x + offsetX, points[i+2].y + offsetY, points[i].x + offsetX, points[i].y + offsetY);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    IMG_Quit();
}

Objx Objx::buildFromPNG(const string& imagePath) {    
    px = Objx();
    px.getSurfaces()[0].texture = imagePath;
    decoupage();
    return px;
}



