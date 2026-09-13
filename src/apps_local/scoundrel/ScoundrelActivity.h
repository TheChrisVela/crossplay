#pragma once

#include <memory>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"
#include "ScoundrelState.h"
#include "ScoundrelScreens.h"

class ScoundrelActivity final : public Activity {
 public:
  ScoundrelActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Scoundrel", renderer, mappedInput) {}
  ~ScoundrelActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void routeButton(int button);
  void routeCard(int cardIndex);
  void newDeal();

  scoundrel::ScoundrelState state;
  scoundrelui::Layout layout;

  bool flashOnNextPaint = false;
  bool interactionsReady = false;

  toybox::Interactions interactions;
};
