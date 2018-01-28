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

#include <GUI/dune/NewsTicker.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/FontManager.h>

#define MESSAGETIME 440
#define SLOWDOWN (timer/55)

NewsTicker::NewsTicker() {
    Widget::enableResizing(false,false);

    timer = -MESSAGETIME/2;
    pBackground = pGFXManager->getUIGraphic(UI_MessageBox);
    pCurrentMessageTexture = nullptr;

    Widget::resize(getTextureSize(pBackground));
}

NewsTicker::~NewsTicker() = default;

void NewsTicker::addMessage(const std::string& msg)
{
    if (messages.contains(msg) || messages.size() >= 3)
        return;

    messages.push(msg);
}

void NewsTicker::addUrgentMessage(const std::string& msg)
{
    messages.clear();

    messages.push(msg);
}

void NewsTicker::draw(Point position) {
    if(isVisible() == false) {
        return;
    }

    //draw background
    if(pBackground == nullptr) {
        return;
    }

    SDL_Rect dest = calcDrawingRect(pBackground, position.x, position.y);
    SDL_RenderCopy(renderer, pBackground, nullptr, &dest);

    // draw message
    if(!messages.empty()) {
        if(timer++ == (MESSAGETIME/3)) {
            timer = -MESSAGETIME/2;
            // delete first message
            messages.pop();

            // if no more messages leave
            if(messages.empty()) {
                timer = -MESSAGETIME/2;
                return;
            };
        };

        //draw text
        SDL_Rect textLocation = { position.x + 10, position.y + 5, 0, 0 };
        if(timer>0) {
            textLocation.y -= SLOWDOWN;
        }

        if(currentMessage != messages.front()) {
            currentMessage = messages.front();
            pCurrentMessageTexture = pFontManager->createTextureWithText(currentMessage, COLOR_BLACK, FONT_STD10);
        }

        if(pCurrentMessageTexture != nullptr) {

            SDL_Rect cut = { 0, 0, 0, 0 };
            if(timer>0) {
                cut.y = 3*SLOWDOWN;
            }

            textLocation.w = cut.w = getWidth(pCurrentMessageTexture.get());
            textLocation.h = cut.h = getHeight(pCurrentMessageTexture.get()) - cut.y;
            SDL_RenderCopy(renderer, pCurrentMessageTexture.get(), &cut, &textLocation);
        }
    };
}
