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

#include <FileClasses/LoadSavePNG.h>
#include <FileClasses/lodepng.h>
#include <misc/draw_util.h>
#include <misc/exceptions.h>
#include <Colors.h>
#include <globals.h>

#include <misc/sdl_support.h>

#include <cstdio>


struct free_deleter
{
    void operator()(void* p) const { std::free(p); }
};

typedef std::unique_ptr<unsigned char, free_deleter> lodepng_ptr;

// TODO: Whis is this code using lodepng's C API, instead of the C++ API?

SDL_Surface* LoadPNG_RW(SDL_RWops* RWop, int freesrc) {
    if(RWop == nullptr) {
        return nullptr;
    }

    sdl2::RWop_ptr free_RWop{ freesrc ? RWop : nullptr };

    unsigned int width = 0;
    unsigned int height = 0;
    sdl2::surface_ptr pic;

    LodePNGState lodePNGState;
    lodepng_state_init(&lodePNGState);

    try {
        // read complete file into memory
        const auto endOffset = SDL_RWsize(RWop);
        if(endOffset <= 0) {
            THROW(std::runtime_error, "LoadPNG_RW(): Cannot determine size of this *.png-File!");
        }

        const auto filesize = static_cast<size_t>(endOffset);
        auto pFiledata = std::make_unique<unsigned char[]>(filesize);

        if(SDL_RWread(RWop, pFiledata.get(), filesize, 1) != 1) {
            THROW(std::runtime_error, "LoadPNG_RW(): Reading this *.png-File failed!");
        }


        auto error = lodepng_inspect(&width, &height, &lodePNGState, pFiledata.get(), filesize);
        if(error != 0) {
            THROW(std::runtime_error, "LoadPNG_RW(): Inspecting this *.png-File failed: " + std::string(lodepng_error_text(error)));
        }

        if(lodePNGState.info_png.color.colortype == LCT_PALETTE && lodePNGState.info_png.color.bitdepth == 8) {
            // read image into a palettized SDL_Surface

            // reset state
            lodepng_state_cleanup(&lodePNGState);
            lodepng_state_init(&lodePNGState);

            lodePNGState.decoder.color_convert = 0;     // do not perform any conversion

            unsigned char *lode_out;
            error = lodepng_decode(&lode_out, &width, &height, &lodePNGState, pFiledata.get(), filesize);
            if(error != 0) {
                THROW(std::runtime_error, "LoadPNG_RW(): Decoding this palletized *.png-File failed: " + std::string(lodepng_error_text(error)));
            }

            lodepng_ptr pImageOut{ lode_out };

            // create new picture surface
            pic = sdl2::surface_ptr{ SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0) };
            if(pic == nullptr) {
                THROW(std::runtime_error, "LoadPNG_RW(): SDL_CreateRGBSurface has failed!");
            }

            const auto colors = reinterpret_cast<SDL_Color*>(lodePNGState.info_png.color.palette);
            SDL_SetPaletteColors(pic->format->palette, colors, 0, lodePNGState.info_png.color.palettesize);

            sdl2::surface_lock pic_lock{ pic.get() };

            const unsigned char * RESTRICT const image_out = reinterpret_cast<const unsigned char *>(pImageOut.get());  // NOLINT
            unsigned char * RESTRICT const pic_surface = static_cast<unsigned char *>(pic_lock.pixels()); // NOLINT

            // Now we can copy pixel by pixel
            if (pic->pitch == static_cast<int>(width)) {
                //std::copy_n(image_out, height * width, pic_surface);
                memcpy(pic_surface, image_out, height * width);
            } else for(unsigned int y = 0; y < height; y++) {
                const auto in = image_out + y * width;
                const auto out = pic_surface + y * pic->pitch;

                //std::copy_n(in, width, out);
                memcpy(out, in, width);
            }

        } else {
            // decode to 32-bit RGBA raw image
            unsigned char *lode_out;
            error = lodepng_decode32(&lode_out, &width, &height, pFiledata.get(), filesize);
            if(error != 0) {
                THROW(std::runtime_error, "LoadPNG_RW(): Decoding this *.png-File failed: " + std::string(lodepng_error_text(error)));
            }

            lodepng_ptr pImageOut{ lode_out };

            // create new picture surface
            pic = sdl2::surface_ptr{ SDL_CreateRGBSurface(0, width, height, 32, RMASK, GMASK, BMASK, AMASK) };
            if(pic == nullptr) {
                THROW(std::runtime_error, "LoadPNG_RW(): SDL_CreateRGBSurface has failed!");
            }

            sdl2::surface_lock pic_lock{pic.get()};

            const Uint32 * RESTRICT const image_out = reinterpret_cast<const Uint32*>(pImageOut.get());  // NOLINT
            unsigned char * RESTRICT const pic_surface = static_cast<unsigned char *>(pic_lock.pixels()); // NOLINT

            // Now we can copy pixel by pixel
            if (static_cast<int>(sizeof(Uint32) * width) == pic->pitch) {
                auto in = image_out;
                auto out = reinterpret_cast<Uint32*>(pic_surface);
                for (unsigned int x = 0; x < height * width; ++x) {
                    *out++ = SDL_SwapLE32(*in++);
                }
            } else for(unsigned int y = 0; y < height; y++) {
                auto in = image_out + y * width;
                auto out = reinterpret_cast<Uint32*>(pic_surface + y * pic->pitch);
                for(unsigned int x = 0; x < width; ++x) {
                    *out++ = SDL_SwapLE32(*in++);
                }
            }

        }

        pFiledata.reset();

        lodepng_state_cleanup(&lodePNGState);

        return pic.release();
    } catch (std::exception &e) {
        SDL_Log("%s", e.what());

        return nullptr;
    }
}

int SavePNG_RW(SDL_Surface* surface, SDL_RWops* RWop, int freedst) {
    sdl2::RWop_ptr free_RWop{ freedst ? RWop : nullptr };

    if(surface == nullptr) {
        return -1;
    }

    const unsigned int width = surface->w;
    const unsigned int height = surface->h;

    auto pImage = std::make_unique<unsigned char>(width*height*4);

    { // Scope
        sdl2::surface_lock lock{ surface };

        // Now we can copy pixel by pixel
        for(unsigned int y = 0; y < height; y++) {
            unsigned char * RESTRICT out = pImage.get() + y * 4*width;
            for(unsigned int x = 0; x < width; x++) {
                const auto pixel = getPixel(surface, x, y);
                SDL_GetRGBA(pixel, surface->format, &out[0], &out[1], &out[2], &out[3]);
                out += 4;
            }
        }
    }


    size_t pngFileSize;

    unsigned char* lode_out;
    const auto error = lodepng_encode32(&lode_out, &pngFileSize, pImage.get(), width, height);
    pImage.release();
    if(error != 0) {
        SDL_Log("%s", lodepng_error_text(error));
        return -1;
    }

    lodepng_ptr ppngFile{ lode_out };

    if(SDL_RWwrite(RWop, ppngFile.get(), 1, pngFileSize) != pngFileSize) {
        SDL_Log("%s", SDL_GetError());
        return -1;
    }

    return 0;
}
