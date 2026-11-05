//
// Created by Sohal03 on 26. 11. 5..
//

#include <iostream>
#include <string>

#include "resource/Texture.h"

using namespace std;

Texture::Texture(const char* filename) : data(nullptr), width(0), height(0), channels(0) {
    if (!filename || string(filename).empty()) {
        cerr << "ERROR: Texture filename is empty." << endl;
        return;
    }
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(filename, &width, &height, &channels, 3);
    channels = 3;

    if (!data) {
        cerr << "Missing file -> " << filename << '\n'
            << "use default Texture " << endl;
    }
}

Texture::~Texture() {
    if (textureID != 0)
        glDeleteTextures(1, &textureID);

    if (data)
        stbi_image_free(data);
}

bool Texture::upload() {
    if (!data || width <= 0 || height <= 0)
        return false;

    if (textureID != 0)
        return true;
    

    glGenTextures(1, &textureID);

    if (textureID == 0)
        return false;


    if (textureID == 0)
        return false;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    return true;
}

void Texture::bind(const unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureID);
}
