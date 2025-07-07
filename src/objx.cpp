// Objx.cpp
#include "objx.hpp"
#include "file_dialog.hpp"
#include <filesystem>
#include <zip.h>
#include <iostream>
#include <fstream>
#include <set>

using namespace std;
namespace fs = std::filesystem;

Objx::Objx() {
    surfaces.emplace_back();
}
Objx Objx::open(const std::string& path) {
    cout << "[importation] " << path << "\n";

    // Créer l'objet Objx à remplir
    Objx px;

    // Extraction
    int err = 0;
    zip_t* archive = zip_open(path.c_str(), 0, &err);
    if (!archive) {
        cerr << "Erreur ouverture .Objx: code=" << err << endl;
        return px; // retourne un objet vide
    }
    cout << "[importation] archive ouverte\n";

    string extractDir = "assets/temp_extract";
    fs::create_directories(extractDir);
    zip_int64_t num_files = zip_get_num_entries(archive, 0);
    cout << "[importation] dossier d'extraction créé\n";

    cout << "[importation] l'Archive contient :\n";
    string meshFile;
    for (int i = 0; i < num_files; ++i) {
        const char* name = zip_get_name(archive, i, 0);
        cout << "[importation]\t" << name << "\n";
        zip_file_t* zf = zip_fopen_index(archive, i, 0);
        if (!zf) continue;

        string outPath = extractDir + "/" + name;
        ofstream outFile(outPath, ios::binary);

        char buffer[4096];
        zip_int64_t bytesRead;
        while ((bytesRead = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
            outFile.write(buffer, bytesRead);
        }
        zip_fclose(zf);

        if (string(name).size() > 5 && string(name).substr(string(name).size() - 5) == ".mesh") {
            meshFile = outPath;
        }
    }

    zip_close(archive);

    // Lire le fichier .mesh
    if (meshFile.empty()) {
        cerr << "Aucun fichier .mesh trouvé dans l’archive." << endl;
        return px;
    }

    ifstream in(meshFile);
    if (!in.is_open()) {
        cerr << "Échec d'ouverture de " << meshFile << endl;
        return px;
    }

    string line;
    Surfaces surf;
    while (getline(in, line)) {
        if (line.empty()) {
            if (!surf.points.empty()) px.addSurface(surf);
            surf = Surfaces();
            continue;
        }
        if (line.rfind("texture=", 0) == 0) {
            cout << "[importation] texture trouvée : " << line;
            surf.texture = line.substr(8);
        } else if (line[0] == 'v') {
            istringstream ss(line.substr(2));
            float x, y, z, u, v;
            ss >> x >> y >> z >> u >> v;
            surf.points.push_back({x, y, z, u, v});
        }
    }
    if (!surf.points.empty()) px.addSurface(surf);

    px.setEmplacement(path); 
    return px;
}

void Objx::addSurface(const Surfaces& surf) {
    surfaces.push_back(surf);
}

std::vector<Surfaces>& Objx::getSurfaces() {
    return surfaces;
}

string Objx::getEmplacement() const {
    return emplacement;
}
void Objx::setEmplacement(string path) {
    emplacement = path;
}

bool Objx::save() {
    if (emplacement == "") {
		emplacement = ouvrirBoiteFichier(true);  // ou une variante de boîte de sauvegarde
		if (emplacement == "") return false; // utilisateur a annulé
	}
	cout << "[Objx save] emplacement : " << emplacement;

    string tempFolder = "assets/export_temp/";
    fs::create_directories(tempFolder);

    set<string> copied;
    for (const auto& s : surfaces) {
        fs::path texPath = s.texture;
        string filename = texPath.filename().string();
        string dest = tempFolder + filename;

        // 💡 Copier seulement si le chemin est absolu ou contient un sous-dossier
        bool textureExterne = texPath.is_absolute() || texPath.parent_path() != "";

        if (textureExterne && copied.count(filename) == 0) {
            copied.insert(filename);
            try {
                fs::copy_file(texPath, dest, fs::copy_options::overwrite_existing);
            } catch (const std::exception& ex) {
                cerr << "Erreur lors de la copie de la texture : " << ex.what() << endl;
                return false;
            }
        }
    }

    // 🔁 Écriture du fichier .mesh
    string meshPath = tempFolder + "map.mesh";
    ofstream out(meshPath);
    string p1;
    ostringstream oss;
    for (const auto& s : surfaces) {
        out << "texture=" << fs::path(s.texture).filename().string() << "\n";
        for (const auto& p : s.points) {
            oss << "v " << p.x << " " << p.y << " " << p.z << " " << p.u << " " << p.v << "\n";
            p1 = oss.str();
            out << p1;
            cout << p1;
        }
        out << "\n";
    }

    // 📦 Création du zip
    string zipPath;
    if (zipPath.size() < 5 || zipPath.substr(-5) != ".objx") {
        zipPath = emplacement + ".objx";
    }
    int err = 0;
    zip_t* archive = zip_open(zipPath.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!archive) return false;

    for (const auto& entry : fs::directory_iterator(tempFolder)) {
        string filePath = entry.path().string();
        string fileName = entry.path().filename().string();
        zip_source_t* source = zip_source_file(archive, filePath.c_str(), 0, 0);
        if (!source || zip_file_add(archive, fileName.c_str(), source, ZIP_FL_OVERWRITE) < 0) {
            if (source) zip_source_free(source);
        }
    }

    zip_close(archive);
    fs::remove_all(tempFolder);
    emplacement = zipPath;
    return true;
}

string Objx::toString() {
    stringstream ss;
    for (const auto& s : surfaces) {
        ss << "texture=" << fs::path(s.texture).filename().string() << "\n";
        for (const auto& p : s.points) {
            ss << "v " << p.x << " " << p.y << " " << p.z << " " << p.u << " " << p.v << "\n";
        }
        ss << "\n";
    }
    return ss.str();
}
