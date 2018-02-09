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

#include <FileClasses/FontManager.h>

#include <globals.h>

#include <misc/draw_util.h>

#include <FileClasses/FileManager.h>
#include <FileClasses/PictureFont.h>
#include <FileClasses/LoadSavePNG.h>

FontManager::FontManager() {
    fonts[FONT_STD10] = std::make_unique<PictureFont>(LoadPNG_RW(pFileManager->openFile("Font10.png"), true).release(), true);
    fonts[FONT_STD12] = std::make_unique<PictureFont>(LoadPNG_RW(pFileManager->openFile("Font12.png"), true).release(), true);
    fonts[FONT_STD24] = std::make_unique<PictureFont>(LoadPNG_RW(pFileManager->openFile("Font24.png"), true).release(), true);
}

FontManager::~FontManager() = default;

void FontManager::drawTextOnSurface(SDL_Surface* pSurface, const std::string& text, Uint32 color, unsigned int fontNum) {
    if(fontNum >= NUM_FONTS) {
        return;
    }

    fonts[fontNum]->drawTextOnSurface(pSurface,text,color);
}

int FontManager::getTextWidth(const std::string& text, unsigned int fontNum) const {
    if(fontNum >= NUM_FONTS) {
        return 0;
    }

    return fonts[fontNum]->getTextWidth(text);
}

int FontManager::getTextHeight(unsigned int fontNum) const {
    if(fontNum >= NUM_FONTS) {
        return 0;
    }

    return fonts[fontNum]->getTextHeight();
}

sdl2::surface_ptr FontManager::createSurfaceWithText(const std::string& text, Uint32 color, unsigned int fontNum) const {
    if(fontNum >= NUM_FONTS) {
        return nullptr;
    }

    const auto font = fonts[fontNum].get();
    const auto width = font->getTextWidth(text);
    const auto height = font->getTextHeight();

    auto pic = sdl2::surface_ptr{ SDL_CreateRGBSurfaceWithFormat(0, width, height, SCREEN_BPP, SCREEN_FORMAT) };

    // create new picture surface
    if(pic == nullptr) {
        return nullptr;
    }

    SDL_SetSurfaceBlendMode(pic.get(), SDL_BLENDMODE_BLEND);
    SDL_FillRect(pic.get(), nullptr, SDL_MapRGBA(pic->format, 0, 0, 0, 0));

    font->drawTextOnSurface(pic.get(),text, color);

    return pic;
}

sdl2::texture_ptr FontManager::createTextureWithText(const std::string& text, Uint32 color, unsigned int fontNum) const {
    const auto surface = createSurfaceWithText(text, color, fontNum);

    return convertSurfaceToTexture(surface.get(), false);
}

sdl2::surface_ptr FontManager::createSurfaceWithMultilineText(const std::string& text, Uint32 color, unsigned int fontNum, bool bCentered) const {
    if(fontNum >= NUM_FONTS) {
        return nullptr;
    }

    size_t startpos = 0;
    size_t nextpos;
    std::vector<std::string> textLines;
    do {
        nextpos = text.find('\n',startpos);
        if(nextpos == std::string::npos) {
            textLines.emplace_back(text.substr(startpos,text.length()-startpos));
        } else {
            textLines.emplace_back(text.substr(startpos,nextpos-startpos));
            startpos = nextpos+1;
        }
    } while(nextpos != std::string::npos);

    const auto font = fonts[fontNum].get();

    const auto lineHeight = font->getTextHeight();
    const auto width = font->getTextWidth(text);
    const int height = lineHeight * textLines.size() + (lineHeight * (textLines.size()-1))/2;

    // create new picture surface
    auto pic = sdl2::surface_ptr{ SDL_CreateRGBSurfaceWithFormat(0, width, height, SCREEN_BPP, SCREEN_FORMAT) };
    if(pic == nullptr) {
        return nullptr;
    }

    SDL_SetSurfaceBlendMode(pic.get(), SDL_BLENDMODE_BLEND);
    SDL_FillRect(pic.get(), nullptr, SDL_MapRGBA(pic->format, 0, 0, 0, 0));

    auto currentLineNum = 0;
    for(const auto& textLine : textLines) {
        const auto tmpSurface = createSurfaceWithText(textLine, color, fontNum);

        auto dest = calcDrawingRect(tmpSurface.get(), bCentered ? width/2 : 0, currentLineNum*lineHeight, bCentered ? HAlign::Center : HAlign::Left, VAlign::Top);
        SDL_BlitSurface(tmpSurface.get(),nullptr,pic.get(),&dest);

        currentLineNum++;
    }

    return pic;
}

sdl2::texture_ptr FontManager::createTextureWithMultilineText(const std::string& text, Uint32 color, unsigned int fontNum, bool bCentered) const {
    const auto surface = createSurfaceWithMultilineText(text, color, fontNum, bCentered);

    return convertSurfaceToTexture(surface.get(), false);
}
