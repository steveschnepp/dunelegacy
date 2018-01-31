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

#include <CutScenes/CutScene.h>

#include <FileClasses/Palfile.h>
#include <FileClasses/FileManager.h>
#include <FileClasses/music/MusicPlayer.h>

#include <globals.h>
#include <sand.h>

#include <SDL2/SDL.h>

CutScene::CutScene()
{
    quiting = false;
}

CutScene::~CutScene()
{
    // Fixes some flickering
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    while(scenes.empty() == false) {
        Scene* pScene = scenes.front();
        scenes.pop();
        delete pScene;
    }
}

void CutScene::run()
{
    SDL_Event event;

    while (!quiting)
    {
        const int frameStart = SDL_GetTicks();

        const auto nextFrameTime = draw();

        while(SDL_PollEvent(&event)) {

            //check the events
            switch (event.type)
            {
                case (SDL_KEYDOWN): // Look for a keypress
                {
                    if((event.key.keysym.sym == SDLK_SPACE) || (event.key.keysym.sym == SDLK_ESCAPE)) {
                        // Fixes some flickering
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        SDL_RenderClear(renderer);
                        SDL_RenderPresent(renderer);
                        quiting = true;
                    }
                }
            }
        }

        const int frameTime = SDL_GetTicks() - frameStart;
        if(frameTime < nextFrameTime) {
            SDL_Delay(nextFrameTime - frameTime);
        }
    }
}

void CutScene::startNewScene() {
    scenes.push(new Scene());
}

void CutScene::addVideoEvent(VideoEvent* newVideoEvent)
{
    if(scenes.empty()) {
        scenes.push(new Scene());
    }

    scenes.back()->addVideoEvent(newVideoEvent);
}

void CutScene::addTextEvent(TextEvent* newTextEvent)
{
    if(scenes.empty()) {
        scenes.push(new Scene());
    }

    scenes.back()->addTextEvent(newTextEvent);
}

void CutScene::addTrigger(CutSceneTrigger* newTrigger)
{
    if(scenes.empty()) {
        scenes.push(new Scene());
    }

    scenes.back()->addTrigger(newTrigger);
}

int CutScene::draw()
{
    int nextFrameTime = 0;

    while(scenes.empty() == false) {

        Scene* pScene = scenes.front();

        if(pScene->isFinished() == true) {
            delete pScene;
            scenes.pop();
            continue;
        } else {
            nextFrameTime = pScene->draw();
            break;
        }
    }

    if(scenes.empty() == true && !musicPlayer->isMusicPlaying()) {
        quit();
    }

    return nextFrameTime;
}

std::unique_ptr<Wsafile> CutScene::create_wsafile(const char* name1)
{
    auto file1 = sdl2::RWop_ptr{pFileManager->openFile(name1)};

    return std::make_unique<Wsafile>(file1.get());
}

std::unique_ptr<Wsafile> CutScene::create_wsafile(const char* name1, const char* name2)
{
    auto file1 = sdl2::RWop_ptr{pFileManager->openFile(name1)};
    auto file2 = sdl2::RWop_ptr{pFileManager->openFile(name2)};

    return std::make_unique<Wsafile>(file1.get(), file2.get());
}

std::unique_ptr<Wsafile> CutScene::create_wsafile(const char* name1, const char* name2, const char* name3)
{
    auto file1 = sdl2::RWop_ptr{pFileManager->openFile(name1)};
    auto file2 = sdl2::RWop_ptr{pFileManager->openFile(name2)};
    auto file3 = sdl2::RWop_ptr{pFileManager->openFile(name3)};

    return std::make_unique<Wsafile>(file1.get(), file2.get(), file3.get());
}
