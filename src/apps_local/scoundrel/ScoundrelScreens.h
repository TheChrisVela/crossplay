#pragma once

#include "../ui/ToyboxScreen.h"
#include "ScoundrelState.h"

namespace scoundrelui {

namespace fui = freeink::ui;

enum : fui::ActionId {
  ActionCard = 1,    // value is the room index (0-3)
  ActionButton = 2,  // value is the button ID (Flee, New Game, etc.)
};

enum Button : int {
  ButtonFlee = 0,
  ButtonNew = 1,
};

struct Layout {
  // Top-left of each of the 4 room cards.
  int16_t roomX[scoundrel::ScoundrelState::kRoomSize] = {};
  int16_t roomY[scoundrel::ScoundrelState::kRoomSize] = {};
  int16_t cardWidth = 0;
  int16_t cardHeight = 0;

  // Player weapon area
  int16_t weaponX = 0;
  int16_t weaponY = 0;
};

struct BoardModel {
  const scoundrel::ScoundrelState* state = nullptr;
};

// Fills layout as it draws
void buildBoard(toybox::Screen& screen, const BoardModel& model, Layout& layout);

}  // namespace scoundrelui
