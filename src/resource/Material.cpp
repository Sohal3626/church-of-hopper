//
// Created by desktop on 26. 4. 26..
//
#include "../../include/resource/Material.h"

#include <istream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <filesystem>
#include <algorithm>

using namespace std;

Material::Material(const string &filename){
    if (!loadMtl(filename))
        throw runtime_error("Failed to load MTL: " + filename);
}

bool Material::loadMtl(const string &filename) {
    const filesystem::path mtlDirectory = filesystem::path(filename).parent_path();
    unordered_map<string, uint16_t> pathMap;

    ifstream in;
    in.open(filename, ifstream::in);
    if (in.fail()) {
        cerr << "Mising file -> " << filename << endl;
        return false;
    }
    string line;
    string trash;
    string currName;
    material currMtl;
    bool first = true;
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        istringstream iss(line.c_str());
        if (line.compare(0, 7, "newmtl ") == 0) {
            if (!first) {
                mtls[currName] = currMtl;
            }else first = false;
            currMtl = material();
            iss >> trash >> currName;
        }else if (line.compare(0, 3, "Ns ") == 0) {
            iss >> trash >> currMtl.shininess;
        }else if (line.compare(0, 3, "Ka ") == 0) {
            iss >> trash >> currMtl.ambient.x >> currMtl.ambient.y >> currMtl.ambient.z;
        }else if (line.compare(0, 3, "Kd ") == 0) {
            iss >> trash >> currMtl.diffuse.x >> currMtl.diffuse.y >> currMtl.diffuse.z;
        }else if (line.compare(0, 3, "Ks ") == 0) {
            iss >> trash >> currMtl.specular.x >> currMtl.specular.y >> currMtl.specular.z;
        }else if (line.compare(0, 3, "Ke ") == 0) {
            iss >> trash >> currMtl.emission.x >> currMtl.emission.y >> currMtl.emission.z;
        }else if (line.compare(0, 3, "Ni ") == 0) {
            iss >> trash >> currMtl.opticalDensity;
        }else if (line.compare(0, 2, "d ") == 0) {
            iss >> trash >> currMtl.dissolve;
        }else if (line.compare(0, 6, "illum ") == 0) {
            iss >> trash >> currMtl.illumination;
        }else if (line.compare(0, 7, "map_Kd ") == 0 || line.compare(0, 7, "map_kd ") == 0) {
            string texturePath;
            iss >> trash >> texturePath;

            ranges::replace(texturePath, '\\', '/');

            const string textureName = filesystem::path(texturePath).filename().string();
            const string fullPath = (mtlDirectory / textureName).string();

            if (!textures.contains(fullPath))
                textures.try_emplace(fullPath, fullPath.c_str());

            currMtl.texture = &textures.at(fullPath);
        }
    }
    if (currName.empty()) return false;
    mtls[currName] = currMtl;
    return true;
}

bool Material::uploadTextures() {
    bool success = true;

    for (auto& [name, texture] : textures) {
        if (!texture.upload()) {
            cerr << "Failed to upload texture: " << name << endl;
            success = false;
        }
    }

    return success;
}