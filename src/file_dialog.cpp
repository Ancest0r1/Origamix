// file_dialog.cpp
#include "file_dialog.hpp"
#include <SDL2/SDL_ttf.h>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

static std::string racine = fs::path(getenv("HOME")).string();

struct Entry {
    std::string name;
    std::string path;
    bool is_dir;
    int depth;
};

static void lister(const std::string& path, int depth, std::vector<Entry>& entries) {
    for (const auto& entry : fs::directory_iterator(path)) {
        std::string name = entry.path().filename().string();
        if (name.size() && name[0] == '.') continue;
        entries.push_back({name, entry.path().string(), entry.is_directory(), depth});
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.name < b.name;
    });
}

std::string ouvrirBoiteFichier(bool saveMode) {
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!font) return "";

    SDL_Window* win = SDL_CreateWindow(saveMode ? "Enregistrer sous..." : "Choisir un fichier",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 440, SDL_WINDOW_SHOWN);
    SDL_Renderer* r = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    std::string current = racine;
    std::string selection = "";
    std::string typedFilename = "";

    bool running = true;
    SDL_StartTextInput();
    while (running) {
        std::vector<Entry> liste;
        lister(current, 0, liste);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (e.key.keysym.sym == SDLK_BACKSPACE && !typedFilename.empty())
                    typedFilename.pop_back();
                if (e.key.keysym.sym == SDLK_RETURN) {
                    if (!typedFilename.empty()) {
                        selection = current + "/" + typedFilename;
                        running = false;
                    } else if (!selection.empty()) {
                        running = false;
                    }
                }
            }
            if (e.type == SDL_TEXTINPUT) {
                typedFilename += e.text.text;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;
                int y = 10;
                for (auto& entry : liste) {
                    SDL_Rect rect = {10 + entry.depth * 20, y, 580, 20};
                    if (my >= y && my <= y + 20) {
                        if (entry.is_dir) {
                            current = entry.path;
                            typedFilename = "";
                            break;
                        } else {
                            selection = entry.path;
                            typedFilename = fs::path(entry.name).string();
                            if (!saveMode) running = false;
                        }
                    }
                    y += 22;
                }
            }
        }

        SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
        SDL_RenderClear(r);

        int y = 10;
        for (auto& entry : liste) {
            std::string label = entry.is_dir ? ("[" + entry.name + "]") : entry.name;
            SDL_Color color = {0, 0, 0};
            SDL_Surface* surf = TTF_RenderText_Solid(font, label.c_str(), color);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_Rect dst = {10 + entry.depth * 20, y, surf->w, surf->h};
            SDL_RenderCopy(r, tex, NULL, &dst);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);
            y += 22;
        }

        if (saveMode) {
            SDL_Color color = {0, 0, 0};
            std::string prompt = "Nom de fichier: " + typedFilename;
            SDL_Surface* surf = TTF_RenderText_Solid(font, prompt.c_str(), color);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_Rect dst = {10, 400, surf->w, surf->h};
            SDL_RenderCopy(r, tex, NULL, &dst);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);
        }

        SDL_RenderPresent(r);
        SDL_Delay(26);
    }

    SDL_StopTextInput();
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    TTF_CloseFont(font);
    TTF_Quit();

    return selection;
}

