//
// Created by desktop on 26. 7. 4..
//

#ifndef TOYRENDERER2_TEXTURE_H
#define TOYRENDERER2_TEXTURE_H
#include <cstring>
#include <iostream>
#include <ostream>
#include <string>

#include "stb_image.h"

class Texture {
private:
    stbi_uc *data;
    int width, height, channels;

public:
    explicit Texture(const char* filename) : data(nullptr), width(0), height(0), channels(0) {
        if (!filename || std::string(filename).empty()) {
            std::cerr << "ERROR: Texture filename is empty." << std::endl;
        }
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load(filename, &width, &height, &channels, 3);
        channels = 3;

        if (!data) {
            std::cerr << "Missing file -> " << filename << '\n'
                << "use default Texture " << std::endl;
        }
    }

    Texture(const Texture&) = delete;

    Texture& operator=(const Texture&) = delete;

    const stbi_uc* getData() const { return data; }

    int getWidth() const { return width; }

    int getHeight() const { return height; }

    int getChannels() const { return channels; }


    ~Texture() {
        if (data) stbi_image_free(data);
    }
};

#endif //TOYRENDERER2_TEXTURE_H