#include <cassert>
#include <iostream>

#include "../../src/apps_local/scoundrel/ScoundrelState.h"

using namespace scoundrel;
using namespace solitaire;

#define TEST(name) void name()
#define RUN_TEST(name) \
    std::cout << "Running " << #name << "..." << std::endl; \
    name(); \
    std::cout << "Passed." << std::endl;

uint8_t makeTestCard(Suit suit, int rank) {
    return solitaire::faceUp(solitaire::makeCard(suit, rank));
}

TEST(TestDeckSlicing) {
    ScoundrelState state;
    state.deal(12345);

    assert(state.deckCount == 40);

    for (int i = 0; i < state.deckCount; ++i) {
        uint8_t c = state.dungeonDeck[i];
        Suit s = suitOf(c);
        int r = rankOf(c);
        if (s == Suit::Hearts || s == Suit::Diamonds) {
            assert(r != 0 && r != 10 && r != 11 && r != 12);
        }
    }
    for (int i = 0; i < ScoundrelState::kRoomSize; ++i) {
        uint8_t c = state.room[i];
        if (c != kNoCard) {
            Suit s = suitOf(c);
            int r = rankOf(c);
            if (s == Suit::Hearts || s == Suit::Diamonds) {
                assert(r != 0 && r != 10 && r != 11 && r != 12);
            }
        }
    }
}

TEST(TestPotionRule) {
    ScoundrelState state;
    state.deal(0);

    state.room[0] = makeTestCard(Suit::Hearts, 4);
    state.room[1] = makeTestCard(Suit::Hearts, 5);
    state.room[2] = makeTestCard(Suit::Spades, 4);
    state.room[3] = makeTestCard(Suit::Spades, 5);

    state.health = 15;

    state.HandleCardTapped(0);
    assert(state.health == 20);
    assert(state.potionUsedThisRoom == true);

    state.health = 15;
    state.HandleCardTapped(1);
    assert(state.health == 15);
    assert(state.room[1] == kNoCard);
}

TEST(TestWeaponRule) {
    ScoundrelState state;
    state.deal(0);

    state.room[0] = makeTestCard(Suit::Diamonds, 4);
    state.room[1] = makeTestCard(Suit::Spades, 6);
    state.room[2] = makeTestCard(Suit::Clubs, 7);

    state.HandleCardTapped(0);
    assert(state.currentWeapon.isEquipped == true);
    assert(state.currentWeapon.weaponCard == makeTestCard(Suit::Diamonds, 4));
    assert(state.currentWeapon.lastMonsterValue == 99);

    state.health = 20;

    state.HandleCardTapped(1);
    assert(state.health == 18);
    assert(state.currentWeapon.lastMonsterValue == 7);

    state.HandleCardTapped(2);
    assert(state.health == 10);
    assert(state.currentWeapon.lastMonsterValue == 7);
}

TEST(TestRoomTransition) {
    ScoundrelState state;
    state.deal(0);

    uint8_t fourthCard = state.room[3];

    state.HandleCardTapped(0);
    state.HandleCardTapped(1);
    state.HandleCardTapped(2);

    assert(state.room[3] == fourthCard);
    assert(state.cardsResolvedThisRoom == 0);
}

TEST(TestFlee) {
    ScoundrelState state;
    state.deal(0);


    int initialDeckCount = state.deckCount;

    assert(state.canFlee == true);
    bool success = state.HandleFleeTapped();
    assert(success == true);

    assert(state.canFlee == false);
    assert(state.deckCount == initialDeckCount);
}

int main() {
    RUN_TEST(TestDeckSlicing);
    RUN_TEST(TestPotionRule);
    RUN_TEST(TestWeaponRule);
    RUN_TEST(TestRoomTransition);
    RUN_TEST(TestFlee);
    return 0;
}
