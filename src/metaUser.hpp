#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <fstream>
#include <thread>
#include <atomic>

class MetaUser {
public:
    static MetaUser& instance();

    void startRecording(const std::string& filename);
    void stopRecording();
    void handleEvent(const SDL_Event& e);

    void replay(const std::string& filename);
    void stopReplay();

    ~MetaUser();

private:
    MetaUser() = default;
    std::ofstream recordFile;
    Uint32 recordStart = 0;
    std::atomic<bool> recording{false};

    std::thread replayThread;
    std::atomic<bool> replaying{false};

    void replayLoop(const std::string& filename);
};
