#pragma once

#include <cstdint>
#include "../solitaire/SolitaireCore.h"

namespace scoundrel {

struct WeaponState {
  bool isEquipped = false;
  uint8_t weaponCard = solitaire::kNoCard; // The Diamond card
  int lastMonsterValue = 99; // Starts high. Dictates what the weapon can fight next.
};

class ScoundrelState {
public:
  ScoundrelState();

  // Game limits
  static constexpr int kMaxHealth = 20;
  static constexpr int kRoomSize = 4;
  static constexpr int kDeckSubset = 44; // standard 52 minus 8 red faces/aces

  int health = kMaxHealth;

  // Custom deck initialized to the 44-card subset
  uint8_t dungeonDeck[kDeckSubset];
  int deckCount = 0;

  // The 4 active cards in the center of the screen
  uint8_t room[kRoomSize] = {solitaire::kNoCard, solitaire::kNoCard, solitaire::kNoCard, solitaire::kNoCard};

  WeaponState currentWeapon;

  int cardsResolvedThisRoom = 0; // Tracks when to deal the next room (max 3)
  bool potionUsedThisRoom = false; // Only 1 potion allowed per room
  bool canFlee = true; // Cannot flee two rooms in a row

  // Setup the deck and initial room
  void deal(uint32_t seed);

  // Core intents:
  bool HandleCardTapped(int roomIndex);
  bool HandleFleeTapped();
  void DealNextRoom();

  int GetCardValue(uint8_t card) const;

private:
  void AddCardToDeck(uint8_t card);
  void ShuffleDeck(uint32_t seed);
};

} // namespace scoundrel
