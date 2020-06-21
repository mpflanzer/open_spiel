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

#ifndef OPEN_SPIEL_GAMES_QWINTO_H_
#define OPEN_SPIEL_GAMES_QWINTO_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "open_spiel/simultaneous_move_game.h"
#include "open_spiel/spiel.h"

// Qwinto, or the Game of Pure Strategy, is a bidding card game where players
// are trying to obtain the most points. In, Qwinto(K), each player has bid
// cards numbered 1-K and a point card deck containing cards numbered 1-K is
// shuffled and set face-down. Each turn, the top point card is revealed, and
// players simultaneously play a bid card; the point card is given to the
// highest bidder or discarded if the bids are equal. For more detail, see:
// https://en.wikipedia.org/wiki/Qwinto
//
// This implementation of Qwinto is slightly more general than the standard
// game. First, more than 2 players can play it. Second, the deck can take on
// pre-determined orders rather than randomly determined. Third, there is an
// option to enable the imperfect information variant described in Sec 3.1.4
// of http://mlanctot.info/files/papers/PhD_Thesis_MarcLanctot.pdf, where only
// the sequences of wins / losses is revealed (not the players' hands).
//
// The returns_type parameter determines how returns (utilities) are defined:
//   - win_loss distributed 1 point divided by number of winners (i.e. players
//     with highest points), and similarly to -1 among losers
//   - point_difference means each player gets utility as number of points
//     collected minus the average over players.
//   - total_points means each player's return is equal to the number of points
//     they collected.
//
// Parameters:
//   "num_cards"     int      The highest bid card, and point card (default: 13)
//   "players"       int      number of players (default: 2)
//   "points_order"  string   "random" (default), "descending", or "ascending"
//   "returns_type"  string   "win_loss" (default), "point_difference", or
//                            "total_points".

namespace open_spiel {
namespace qwinto {

enum Die {
  kInvalidDie = 0,
  kOrange = 1,
  kPurple = 2,
  kYellow = 4,
};

enum class Phase {
  kSelectDice,
  kRollDice,
  kSubmitPoints,
};

inline constexpr int kDefaultNumPlayers = 1;
inline constexpr int kDefaultNumDice = 3;
inline constexpr int kDefaultNumFields = 9;

class QwintoState : public SimMoveState {
 public:
  explicit QwintoState(std::shared_ptr<const Game> game);

  Player CurrentPlayer() const override;
  std::string ActionToString(Player player, Action action_id) const override;
  std::string ToString() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  //std::string ObservationString(Player player) const override;

  void ObservationTensor(Player player,
                         std::vector<double>* values) const override;
  std::unique_ptr<State> Clone() const override;
  std::vector<std::pair<Action, double>> ChanceOutcomes() const override;

  std::vector<Action> LegalActions(Player player) const override;

 protected:
  void DoApplyAction(Action action_id) override;
  void DoApplyActions(const std::vector<Action>& actions) override;

 private:
  Player player_;
  Player current_player_;
  Die dice_;
  int dice_outcome_;
  Phase phase_;
  std::vector<int> scores_;
};

class QwintoGame : public Game {
 public:
  explicit QwintoGame(const GameParameters& params);

  int NumDistinctActions() const override { return kDefaultNumDice * kDefaultNumFields + 1; }
  std::unique_ptr<State> NewInitialState() const override;
  int MaxChanceOutcomes() const override;
  int NumPlayers() const override { return num_players_; }
  double MinUtility() const override;
  double MaxUtility() const override;
  //double UtilitySum() const override { return 0; }
  std::shared_ptr<const Game> Clone() const override {
    return std::shared_ptr<const Game>(new QwintoGame(*this));
  }
  std::vector<int> ObservationTensorShape() const override;
  int MaxGameLength() const override { return 3 * 31; }

 private:
  int num_players_;  // Number of players
};

}  // namespace qwinto
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_QWINTO_H_
