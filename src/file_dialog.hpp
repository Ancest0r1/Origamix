#pragma once
#include <string>

// Ouvre une boîte de dialogue SDL pour choisir un fichier PNG
// Retourne le chemin complet si un fichier est sélectionné, sinon retourne une chaîne vide.
std::string ouvrirBoiteFichier(bool saveMode);
