#include "ScoundrelScreens.h"

#include <cstdio>
#include "../solitaire/SolitaireSuits.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxText.h"

namespace scoundrelui {

using namespace scoundrel;

namespace {

constexpr int kTopBarH = 100;
constexpr int kRoomY = 100;
constexpr int kPlayerAreaY = 560;

constexpr int kCardW = 160;
constexpr int kCardH = 210;
constexpr int kRadius = 8;
constexpr int kEdge = 2;

constexpr int kRoomSpacingX = 40;
constexpr int kRoomSpacingY = 20;

constexpr int kStartX = (480 - (2 * kCardW + kRoomSpacingX)) / 2;

const freeink::Icon& suitArt(solitaire::Suit suit, bool outline) {
  switch (suit) {
    case solitaire::Suit::Spades: return outline ? icon_spadeOutline_46 : icon_spadeSolid_46;
    case solitaire::Suit::Clubs: return outline ? icon_clubOutline_46 : icon_clubSolid_46;
    case solitaire::Suit::Hearts: return outline ? icon_heartOutline_46 : icon_heartSolid_46;
    case solitaire::Suit::Diamonds: return outline ? icon_diamondOutline_46 : icon_diamondSolid_46;
  }
  return icon_spadeSolid_46; // default
}

const char* labelForRank(int rank) {
  switch (rank) {
    case 0: return "A";
    case 10: return "J";
    case 11: return "Q";
    case 12: return "K";
  }
  static char buf[4];
  snprintf(buf, sizeof(buf), "%d", rank % 100 + 1);
  return buf;
}

void drawCard(toybox::Screen& screen, int x, int y, uint8_t card) {
  if (card == solitaire::kNoCard) {
    return;
  }

  solitaire::Suit suit = solitaire::suitOf(card);
  int rank = solitaire::rankOf(card);
  bool red = solitaire::isRed(card);

  fui::Color inkColor = red ? fui::Color::White : fui::Color::Black;
  fui::Color paperColor = red ? fui::Color::Black : fui::Color::White;

  auto& target = screen.target();
  fui::Rect rect = fui::makeRect(x, y, kCardW, kCardH);

  target.fill(rect, fui::Paint::solid(paperColor), kRadius);
  target.stroke(rect, fui::Paint::solid(fui::Color::Black), kEdge, kRadius);

  // Icon
  const freeink::Icon& art = suitArt(suit, red);
  fui::BitmapRef ref;
  ref.data = art.bits;
  ref.width = art.w;
  ref.height = art.h;
  target.bitmap(fui::makeRect(x + (kCardW - art.w) / 2, y + (kCardH - art.h) / 2, art.w, art.h), ref, fui::BitmapMode::Center, fui::Paint::solid(inkColor));

  // Label
  const char* label = labelForRank(rank);
  fui::TextStyle style;
  style.font = toybox::kDisplayFont;
  style.align = fui::TextAlign::Left;
  // Use toybox defaults for foreground
  target.text(fui::makeRect(x + 16, y + 16, 40, 40), label, style);

  style.align = fui::TextAlign::Right;
  target.text(fui::makeRect(x + kCardW - 56, y + kCardH - 56, 40, 40), label, style);
}

} // namespace

void buildBoard(toybox::Screen& screen, const BoardModel& model, Layout& layout) {
  layout.cardWidth = kCardW;
  layout.cardHeight = kCardH;

  auto& target = screen.target();

  fui::TextStyle style;
  style.font = toybox::kUiFont;
  style.align = fui::TextAlign::Left;

  char hpText[32];
  snprintf(hpText, sizeof(hpText), "HP: %d/%d", model.state->health, ScoundrelState::kMaxHealth);
  target.text(fui::makeRect(20, 40, 150, 40), hpText, style);

  style.align = fui::TextAlign::Right;
  char deckText[32];
  snprintf(deckText, sizeof(deckText), "Deck: %d", model.state->deckCount);
  target.text(fui::makeRect(310, 40, 150, 40), deckText, style);

  bool canFleeBtn = model.state->canFlee && model.state->cardsResolvedThisRoom == 0;

  fui::ButtonProps flee;
  flee.label = "FLEE";
  flee.action = ActionButton;
  flee.value = ButtonFlee;
  flee.enabled = canFleeBtn;
  screen.button(flee, fui::makeRect(190, 20, 100, 60));

  for (int i = 0; i < ScoundrelState::kRoomSize; ++i) {
    int col = i % 2;
    int row = i / 2;
    int cx = kStartX + col * (kCardW + kRoomSpacingX);
    int cy = kRoomY + row * (kCardH + kRoomSpacingY);

    layout.roomX[i] = cx;
    layout.roomY[i] = cy;

    if (model.state->room[i] != solitaire::kNoCard) {
      drawCard(screen, cx, cy, model.state->room[i]);
      fui::ButtonProps cardBtn;
      cardBtn.label = "";
      cardBtn.action = ActionCard;
      cardBtn.value = i;
      cardBtn.borderEdges = fui::EdgesNone;
      screen.button(cardBtn, fui::makeRect(cx, cy, kCardW, kCardH));
    }
  }

  layout.weaponX = kStartX;
  layout.weaponY = kPlayerAreaY;

  style.align = fui::TextAlign::Left;
  target.text(fui::makeRect(layout.weaponX, layout.weaponY - 30, kCardW, 30), "Weapon", style);

  if (model.state->currentWeapon.isEquipped) {
    drawCard(screen, layout.weaponX, layout.weaponY, model.state->currentWeapon.weaponCard);

    char weaponText[64];
    snprintf(weaponText, sizeof(weaponText), "Max Target:\n%d", model.state->currentWeapon.lastMonsterValue);
    target.text(fui::makeRect(layout.weaponX + kCardW + 20, layout.weaponY + kCardH/2 - 20, 200, 40), weaponText, style);
  } else {
    fui::Rect emptyRect = fui::makeRect(layout.weaponX, layout.weaponY, kCardW, kCardH);
    target.stroke(emptyRect, fui::Paint::solid(fui::Color::Black), 1, kRadius);
    style.align = fui::TextAlign::Center;
    target.text(emptyRect, "Barehanded", style);
  }
}

} // namespace scoundrelui
