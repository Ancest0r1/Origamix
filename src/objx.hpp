// Objx.hpp
#pragma once
#include <string>
#include <vector>
#include <optional>

using namespace std; 

struct Point5D {
    float x, y, z;
    float u, v;
};

struct Surfaces {
    std::vector<Point5D> points;
    std::string texture; // Peut être un chemin complet tant que non sauvegardé
};

class Objx {
public:
	Objx();
    static Objx open(const std::string& path);             // À implémenter plus tard
    static Objx buildFromPNG(const std::string& imagePath);     // Mode interactif de création

    bool save();                      // met aussi à jour emplacement
	string toString();

    void addSurface(const Surfaces& surface);
	std::vector<Surfaces>& getSurfaces();
    string getEmplacement() const;
    void setEmplacement(string path);

private:
    std::vector<Surfaces> surfaces;
    string emplacement; // devient non-null quand sauvegardé
};

