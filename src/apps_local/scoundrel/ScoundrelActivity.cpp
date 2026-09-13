#include "ScoundrelActivity.h"

#include <Memory.h>
#include <HalStorage.h>

#include "../Shelf.h"
#include "../ui/Toybox.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxTheme.h"

namespace {
namespace fui = freeink::ui;
namespace ui = scoundrelui;
}

std::unique_ptr<Activity> ScoundrelActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<ScoundrelActivity>(renderer, mappedInput);
}

void ScoundrelActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);

  newDeal();
}

void ScoundrelActivity::onExit() {
  Activity::onExit();
}

void ScoundrelActivity::newDeal() {
  state.deal(millis());
  flashOnNextPaint = true;
  requestUpdate();
}

void ScoundrelActivity::routeCard(int cardIndex) {
  if (state.HandleCardTapped(cardIndex)) {
    requestUpdate();
  }
}

void ScoundrelActivity::routeButton(int button) {
  if (button == ui::ButtonFlee) {
    if (state.HandleFleeTapped()) {
      flashOnNextPaint = true;
      requestUpdate();
    }
  } else if (button == ui::ButtonNew) {
    newDeal();
  }
}

void ScoundrelActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    shelf::leave(renderer, mappedInput);
    return;
  }

  if (!interactionsReady) return;

  int tapX = 0;
  int tapY = 0;
  if (mappedInput.wasScreenTapped(tapX, tapY)) {
    fui::InputSnapshot input;
    input.touchX = tapX;
    input.touchY = tapY;
    input.touchReleased = true;

    const fui::ActionEvent action = interactions.route(input);
    if (action.action == ui::ActionCard) {
      routeCard(action.value);
    } else if (action.action == ui::ActionButton) {
      routeButton(action.value);
    }
  }
}

void ScoundrelActivity::render(RenderLock&& lock) {
  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer, toybox::toyboxFaces());
  const fui::InputSnapshot noInput{};
  interactionsReady = false;
  toybox::Frame frame(target, target.deviceContext(), noInput, interactions);
  toybox::Screen screen(frame);

  const bool fullFlash = flashOnNextPaint;
  flashOnNextPaint = false;

  target.fill(fui::makeRect(0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()), fui::Paint::solid(fui::Color::White));

  ui::BoardModel model;
  model.state = &state;

  ui::buildBoard(screen, model, layout);

  interactionsReady = true;

  renderer.displayBuffer(fullFlash ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
}
