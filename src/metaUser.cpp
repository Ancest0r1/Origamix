#include "metaUser.hpp"
#include <chrono>
#include <thread>
#include <fstream>

MetaUser& MetaUser::instance() {
    static MetaUser inst;
    return inst;
}

void MetaUser::startRecording(const std::string& filename) {
    recordFile.open(filename, std::ios::binary);
    if (recordFile.is_open()) {
        recording = true;
        recordStart = SDL_GetTicks();
    }
}

void MetaUser::stopRecording() {
    recording = false;
    if (recordFile.is_open()) {
        recordFile.close();
    }
}

void MetaUser::handleEvent(const SDL_Event& e) {
    if (!recording || !recordFile.is_open()) return;
    Uint32 ts = SDL_GetTicks() - recordStart;
    recordFile.write(reinterpret_cast<const char*>(&ts), sizeof(Uint32));
    recordFile.write(reinterpret_cast<const char*>(&e), sizeof(SDL_Event));
    recordFile.flush();
}

void MetaUser::replay(const std::string& filename) {
    if (replaying) return;
    replaying = true;
    replayThread = std::thread(&MetaUser::replayLoop, this, filename);
}

void MetaUser::replayLoop(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        replaying = false;
        return;
    }
    Uint32 last = 0;
    while (replaying) {
        Uint32 ts;
        SDL_Event e;
        in.read(reinterpret_cast<char*>(&ts), sizeof(Uint32));
        if (!in) break;
        in.read(reinterpret_cast<char*>(&e), sizeof(SDL_Event));
        if (!in) break;
        if (ts > last) SDL_Delay(ts - last);
        last = ts;
        SDL_PushEvent(&e);
    }
    replaying = false;
}

void MetaUser::stopReplay() {
    if (!replaying) return;
    replaying = false;
    if (replayThread.joinable()) replayThread.join();
}

MetaUser::~MetaUser() {
    stopRecording();
    stopReplay();
}

