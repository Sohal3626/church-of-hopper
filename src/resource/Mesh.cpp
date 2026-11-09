//
// Created by desktop on 26. 3. 19..
//

#include "../../include/resource/Mesh.h"
#include "../../include/resource/Material.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

Mesh::Mesh(const string& filename) {
    loadObj(filename);
}

void Mesh::set_indexStart(){
    if (!subMeshes.empty()) {
        subMeshes.back().indexCount = static_cast<uint32_t>(indices.size()) - subMeshes.back().indexStart;
    }
}

bool Mesh::loadObj(const string& filename) {
    vertices.reserve(10000);
    std::ifstream in;
    in.open(filename, ifstream::in);
    if (in.fail()) {
        cerr << "Mising file -> " << filename << endl;
        return false;
    }
    vector<vec3> positions;
    vector<vec2> uvs;
    vector<vec3> normals;
    positions.reserve(1000);
    uvs.reserve(1000);
    normals.reserve(1000);
    map<FaceVertex, uint32_t> checkUnique;

    string curr_mtl;
    string line;
    while (getline(in,line)) {
        if (line.empty()) continue;
        char trashc;
        string trashs;
        istringstream iss(line.c_str());
        if (!line.compare(0, 7, "usemtl ")) {
            set_indexStart();
            MeshGroup group{};
            group.indexStart = static_cast<uint32_t>(indices.size());
            iss >> trashs >> group.mtlName;
            subMeshes.push_back(group);

            cout << "Using Material: " << group.mtlName << endl;
        }else if (!line.compare(0, 2, "v ")) {
            iss >> trashc;
            vec3 v;
            iss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }else if (!line.compare(0, 3, "vt ")) {
            iss >> trashs;
            vec2 t;
            iss >> t.x >>  t.y;
            uvs.push_back(t);
        }else if (!line.compare(0, 3, "vn ")) {
            iss >> trashs;
            vec3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }else if (!line.compare(0, 2, "f ")) {
            iss >> trashc;

            if (subMeshes.empty()) {
                MeshGroup defaultGroup{};
                defaultGroup.indexStart = 0;
                defaultGroup.indexCount = 0;
                defaultGroup.mtlName = "default";
                subMeshes.push_back(defaultGroup);
            }

            string segment;
            vector<FaceVertex> fvs;
            fvs.reserve(3);
            while (iss >> segment) {
                ranges::replace(segment, '/', ' ');
                istringstream ss(segment);
                FaceVertex fv{-1, -1, -1};

                ss >> fv.position;
                if (ss >> fv.texCoord) {}
                if (ss >> fv.normal) {}

                if (fv.position > 0) fv.position--;
                if (fv.texCoord > 0) fv.texCoord--;
                if (fv.normal > 0) fv.normal--;
                fvs.push_back(fv);
            }
            for (size_t i = 1; i < fvs.size() - 1; ++i) {
                for (FaceVertex tri[3] = { fvs[0], fvs[i], fvs[i+1] }; const FaceVertex& fv : tri) {
                        if (!checkUnique.contains(fv)) {
                            Vertex vertex{};
                            vec3 fvPos = positions[fv.position];
                            vertex.position = {fvPos.x, fvPos.y, fvPos.z, 1.};
                            if (fv.texCoord > -1) vertex.texCoord = uvs[fv.texCoord];
                            if (fv.normal > -1) {
                                vec3 fvNormal = normals[fv.normal];
                                vertex.normal = {fvNormal.x, fvNormal.y, fvNormal.z, 0.};
                            }

                            auto idx = static_cast<uint32_t>(vertices.size());
                            vertices.push_back(vertex);
                            indices.push_back(idx);
                            checkUnique[fv] = idx;
                        }else{
                            indices.push_back(checkUnique[fv]);
                    }
                }
            }
        }
    }
    set_indexStart();
    in.close();

    cout << "--- Mesh Load Report ---" << endl;
    cout << "Vertices: " << vertices.size() << endl;
    cout << "Indices:  " << indices.size() << endl;
    cout << "SubMeshes: " << subMeshes.size() << endl;

    for(int i=0; i < subMeshes.size(); i++) {
        cout << " Group[" << i << "] Start: " << subMeshes[i].indexStart
             << " Count: " << subMeshes[i].indexCount << endl;
    }
    return true;
}

void Mesh::computeBB() {
    if (vertices.empty()) {
        boundingBox = {};
        return;
    }

    boundingBox.min = vec3(vertices[0].position);
    boundingBox.max = vec3(vertices[0].position);

    for (const auto& vertex : vertices) {
        const vec3 pos(vertex.position);
        boundingBox.min = min(boundingBox.min, pos);
        boundingBox.max = max(boundingBox.max, pos);
    }
}