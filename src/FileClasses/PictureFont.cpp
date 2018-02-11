/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <FileClasses/PictureFont.h>
#include <misc/exceptions.h>
#include <misc/sdl_support.h>
#include <Colors.h>

#include <numeric>
#include <cstdlib>
#include <cstring>

/// Constructor
/**
    The constructor reads from the surface all data and saves them internal. Immediately after the PictureFont-Object is
    constructed pic can be freed. All data is saved in the class.
    \param  pic The picture which contains the font
    \param  freesrc A non-zero value means it will automatically close/free the src for you.
*/
PictureFont::PictureFont(SDL_Surface* pic, int freesrc)
{
    auto  pic_handle = sdl2::surface_ptr{ freesrc ? pic : nullptr };

    if(pic == nullptr) {
        THROW(std::invalid_argument, "PictureFont::PictureFont(): pic == nullptr!");
    }

    sdl2::surface_lock lock{ pic };

    characterHeight = pic->h - 2;

    auto curXPos = 1;
    auto oldXPos = curXPos;
    char* const RESTRICT secondLine = static_cast<char*>(pic->pixels) + pic->pitch;
    for(auto & c : character) {
        while((curXPos < pic->w) && (secondLine[curXPos] != 14)) {
            curXPos++;
        }

        if(curXPos >= pic->w) {
            THROW(std::runtime_error, "PictureFont::PictureFont(): No valid surface for loading font!");
        }

        c.width = curXPos - oldXPos;
        c.data.resize(c.width * characterHeight);

        auto mempos = 0;
        for(auto y = 1; y < pic->h - 1; y++) {
            const unsigned char * const RESTRICT in = static_cast<unsigned char*>(pic->pixels) + y * pic->pitch;
            for(auto x = oldXPos; x < curXPos; x++) {
                auto col = in[x];
                if(col != 0) {
                    col = 1;
                }

                c.data[mempos] = col;
                mempos++;
            }
        }
        curXPos++;
        oldXPos = curXPos;
    }
}

/// Destructor
/**
    Frees all memory.
*/
PictureFont::~PictureFont() = default;


void PictureFont::drawTextOnSurface(SDL_Surface* pSurface, const std::string& text, Uint32 baseColor) {

    const auto surface_color = MapRGBA(pSurface->format, baseColor);

    sdl2::surface_lock lock{ pSurface };

    const int bpp = pSurface->format->BytesPerPixel;

    auto curXPos = 0;
    for (unsigned char index : text) {
        const auto font_character = character[index];
        //Now we can copy pixel by pixel
        for(int y = 0; y < characterHeight; y++) {
            Uint8 * const RESTRICT out = static_cast<Uint8 *>(pSurface->pixels) + y * pSurface->pitch;
            const char * const RESTRICT in = &font_character.data[y * font_character.width];
            for(int x = 0; x < font_character.width; x++) {
                const char color = in[x];
                if(color == 0) continue;

                const auto pixel = out + (x+curXPos) * bpp;

                switch(bpp) {
                case 1:
                    *pixel = surface_color;
                    break;

                case 2:
                    *reinterpret_cast<Uint16 *>(pixel) = surface_color;
                    break;

                case 3:
                    if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                        pixel[0] = (surface_color>> 16) & 0xff;
                        pixel[1] = (surface_color>> 8) & 0xff;
                        pixel[2] = surface_color& 0xff;
                    } else {
                        pixel[0] = surface_color& 0xff;
                        pixel[1] = (surface_color>> 8) & 0xff;
                        pixel[2] = (surface_color>> 16) & 0xff;
                    }
                    break;

                case 4:
                    *reinterpret_cast<Uint32 *>(pixel) = surface_color;
                    break;
                }
            }
        }

        curXPos += font_character.width;
    }
}

/// Returns the number of pixels a text needs
/**
        This methods returns the number of pixels this text would need if printed.
        \param  text    The text to be checked for it's length in pixel
        \return Number of pixels needed
*/
int PictureFont::getTextWidth(const std::string& text) const {
    return std::accumulate(std::begin(text), std::end(text),
        0, [&](int a, unsigned char b) {return a + character[b].width; });
}

