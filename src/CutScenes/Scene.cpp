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

#include <CutScenes/Scene.h>

#include <globals.h>

#include <SDL2/SDL.h>

Scene::Scene() = default;

Scene::~Scene() = default;

void Scene::addVideoEvent(VideoEvent* newVideoEvent)
{
    videoEvents.emplace(newVideoEvent);
}

void Scene::addTextEvent(TextEvent* newTextEvent)
{
    textEvents.emplace_back(newTextEvent);
}

void Scene::addTrigger(CutSceneTrigger* newTrigger)
{
    triggerList.emplace(newTrigger);
}

int Scene::draw()
{
    auto nextFrameTime = 0;

    // 1.: Clear the whole screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // 2.: Draw everything on the screen
    while(videoEvents.empty() == false) {
        auto& pVideoEvent = videoEvents.front();

        if(pVideoEvent->isFinished() == true) {
            videoEvents.pop();
            continue;
        } else {
            nextFrameTime = pVideoEvent->draw();
            break;
        }
    }

    for(const auto& pTextEvent : textEvents) {
        pTextEvent->draw(currentFrameNumber);
    }

    // 3.: Render everything
    SDL_RenderPresent(renderer);

    // 4.: Process Triggers
    while(triggerList.empty() == false) {
        auto& pTrigger = triggerList.top();

        if(pTrigger->getTriggerFrameNumber() > currentFrameNumber) {
            break;
        }

        if(pTrigger->getTriggerFrameNumber() == currentFrameNumber) {
            pTrigger->trigger(currentFrameNumber);
        }

        triggerList.pop();
    }

    currentFrameNumber++;

    return nextFrameTime;
}
