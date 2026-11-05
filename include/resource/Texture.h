//
// Created by desktop on 26. 7. 4..
//

#ifndef TOYRENDERER2_TEXTURE_H
#define TOYRENDERER2_TEXTURE_H

#include <glad/glad.h>

#include "stb_image.h"

class Texture {
private:
    stbi_uc *data;
    int width, height, channels;
    GLuint textureID = 0;
public:
    explicit Texture(const char* filename);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    [[nodiscard]] const stbi_uc* getData() const { return data; }
    [[nodiscard]] int getWidth() const { return width; }
    [[nodiscard]] int getHeight() const { return height; }
    [[nodiscard]] int getChannels() const { return channels; }


    ~Texture();

    bool upload();
    void bind(unsigned int unit) const;
};

#endif //TOYRENDERER2_TEXTURE_H