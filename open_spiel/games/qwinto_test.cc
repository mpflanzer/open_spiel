// Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "open_spiel/games/qwinto.h"

#include "open_spiel/game_parameters.h"
#include "open_spiel/game_transforms/turn_based_simultaneous_game.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/tests/basic_tests.h"

namespace open_spiel {
namespace qwinto {
namespace {

namespace testing = open_spiel::testing;

void BasicQwintoTests() {
  testing::LoadGameTest("qwinto");
  testing::ChanceOutcomesTest(*LoadGame("qwinto"));
  testing::RandomSimTest(*LoadGame("qwinto"), 100);
  for (Player players = 1; players <= 4; players++) {
      testing::RandomSimTest(
          *LoadGame("qwinto",
                    {{"players", GameParameter(players)}}),
          10);
  }
}

void LegalActionsValidAtEveryState() {
  GameParameters params;
  std::shared_ptr<const Game> game = LoadGameAsTurnBased("qwinto", params);
  testing::RandomSimTest(*game, /*num_sims=*/10);
}

}  // namespace
}  // namespace qwinto
}  // namespace open_spiel

int main(int argc, char **argv) {
  open_spiel::qwinto::BasicQwintoTests();
  open_spiel::qwinto::LegalActionsValidAtEveryState();
}
