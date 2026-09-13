#include "ScoundrelState.h"

namespace scoundrel {

namespace {
} // namespace

ScoundrelState::ScoundrelState() {
}

int ScoundrelState::GetCardValue(uint8_t card) const {
  int rank = solitaire::rankOf(card);
  if (rank == 0) return 14; // Ace
  return rank + 1; // 2=2, ..., 10=10, J=11, Q=12, K=13
}

void ScoundrelState::deal(uint32_t seed) {
  uint8_t fullDeck[solitaire::kDeck];
  solitaire::shuffledDeck(seed, fullDeck);

  deckCount = 0;
  for (int i = 0; i < solitaire::kDeck; ++i) {
    uint8_t card = fullDeck[i];
    solitaire::Suit suit = solitaire::suitOf(card);
    int rank = solitaire::rankOf(card);

    // Remove all Red Face Cards (J, Q, K of Hearts & Diamonds) and Red Aces
    if ((suit == solitaire::Suit::Hearts || suit == solitaire::Suit::Diamonds) &&
        (rank == 0 || rank == 10 || rank == 11 || rank == 12)) {
      continue;
    }

    AddCardToDeck(card);
  }

  // Now we have deckCount == 44 cards.

  health = kMaxHealth;
  currentWeapon = WeaponState();
  cardsResolvedThisRoom = 0;
  potionUsedThisRoom = false;
  canFlee = true;

  for (int i = 0; i < kRoomSize; ++i) {
    room[i] = solitaire::kNoCard;
  }

  DealNextRoom();
}

void ScoundrelState::AddCardToDeck(uint8_t card) {
  if (deckCount < kDeckSubset) {
    dungeonDeck[deckCount++] = solitaire::faceUp(card);
  }
}

bool ScoundrelState::HandleCardTapped(int roomIndex) {
  if (roomIndex < 0 || roomIndex >= kRoomSize) return false;

  uint8_t card = room[roomIndex];
  if (card == solitaire::kNoCard) return false;

  solitaire::Suit suit = solitaire::suitOf(card);
  int value = GetCardValue(card);

  if (suit == solitaire::Suit::Hearts) {
    // Potion
    if (!potionUsedThisRoom) {
      health += value;
      if (health > kMaxHealth) health = kMaxHealth;
      potionUsedThisRoom = true;
    }
    // If potionUsedThisRoom == true, it's discarded with no healing effect
  } else if (suit == solitaire::Suit::Diamonds) {
    // Weapon
    currentWeapon.isEquipped = true;
    currentWeapon.weaponCard = card;
    currentWeapon.lastMonsterValue = 99;
  } else {
    // Monster (Spades/Clubs)
    if (currentWeapon.isEquipped && value < currentWeapon.lastMonsterValue) {
      int weaponValue = GetCardValue(currentWeapon.weaponCard);
      int damage = value - weaponValue;
      if (damage < 0) damage = 0;
      health -= damage;
      currentWeapon.lastMonsterValue = value;
    } else {
      health -= value;
    }
  }

  room[roomIndex] = solitaire::kNoCard;
  cardsResolvedThisRoom++;

  if (cardsResolvedThisRoom == 3) {
    DealNextRoom();
  }

  return true;
}

bool ScoundrelState::HandleFleeTapped() {
  if (cardsResolvedThisRoom != 0 || !canFlee) return false;

  // Cards moved to bottom of deck. Since we deal from the end (deckCount-1),
  // bottom of deck is index 0.
  // Shift existing deck elements right to make room for fleeing cards.
  int cardsToMove = 0;
  uint8_t fledCards[kRoomSize];
  for (int i = 0; i < kRoomSize; ++i) {
    if (room[i] != solitaire::kNoCard) {
      fledCards[cardsToMove++] = room[i];
      room[i] = solitaire::kNoCard;
    }
  }

  if (cardsToMove > 0 && deckCount + cardsToMove <= kDeckSubset) {
    for (int i = deckCount - 1; i >= 0; --i) {
      dungeonDeck[i + cardsToMove] = dungeonDeck[i];
    }
    for (int i = 0; i < cardsToMove; ++i) {
      dungeonDeck[i] = fledCards[i];
    }
    deckCount += cardsToMove;
  }

  // Fleeing disables flee until next room is fully resolved (i.e. transitions),
  // DealNextRoom shouldn't reset canFlee if we just fled.
  DealNextRoom();
  canFlee = false;
  return true;
}

void ScoundrelState::DealNextRoom() {
  for (int i = 0; i < kRoomSize; ++i) {
    if (room[i] == solitaire::kNoCard && deckCount > 0) {
      room[i] = dungeonDeck[--deckCount];
    }
  }

  cardsResolvedThisRoom = 0;
  potionUsedThisRoom = false;
  canFlee = true;
}

} // namespace scoundrel
