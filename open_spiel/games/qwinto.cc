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

#include <algorithm>
#include <memory>
#include <utility>

#include "open_spiel/abseil-cpp/absl/strings/str_cat.h"
#include "open_spiel/game_parameters.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

namespace open_spiel {
namespace qwinto {
namespace {

inline constexpr int kDefaultMissPoints = -5;
inline constexpr int kDefaultTerminationPoints = -20;

const GameType kGameType{
    /*short_name=*/"qwinto",
    /*long_name=*/"Qwinto",
    GameType::Dynamics::kSimultaneous,
    GameType::ChanceMode::kExplicitStochastic,
    GameType::Information::kPerfectInformation,
    GameType::Utility::kGeneralSum,
    GameType::RewardModel::kTerminal,
    /*max_num_players=*/10,
    /*min_num_players=*/1,
    /*provides_information_state_string=*/false,
    /*provides_information_state_tensor=*/false,
    /*provides_observation_string=*/false,
    /*provides_observation_tensor=*/true,
    /*parameter_specification=*/
    {{"players", GameParameter(kDefaultNumPlayers)}}};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new QwintoGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

std::string phase2str(Phase phase) {
  switch(phase) {
    case Phase::kSelectDice:
      return "Select";
    case Phase::kRollDice:
      return "Roll";
    case Phase::kSubmitPoints:
      return "Submit";
  }

  SpielFatalError(absl::StrCat("Phase ", phase, " is invalid."));
}

std::string dice2str(Die dice) {
  std::string str;

  if(dice & kOrange) {
    absl::StrAppend(&str, "Orange, ");
  }

  if(dice & kYellow) {
    absl::StrAppend(&str, "Yellow, ");
  }

  if(dice & kPurple) {
    absl::StrAppend(&str, "Purple");
  }

  return str;
}

}  // namespace

QwintoState::QwintoState(std::shared_ptr<const Game> game)
    : SimMoveState(game),
      player_{0},
      current_player_{0},
      num_dice_rolls_{0},
      dice_{kInvalidDie},
      dice_outcome_{0},
      phase_{Phase::kSelectDice} {
  scores_.resize(game->NumPlayers() * (kDefaultNumDice * kDefaultNumFields + 1), 0);
  std::fill(scores_.begin(), scores_.end(), 0);
}

int QwintoState::CurrentPlayer() const {
  if (IsTerminal()) {
    return kTerminalPlayerId;
  } else {
    return player_;
  }
}

void QwintoState::DoApplyAction(Action action) {
  if (IsSimultaneousNode()) {
    ApplyFlatJointAction(action);
    return;
  } else if(IsChanceNode()) {
    SPIEL_CHECK_EQ(phase_, Phase::kRollDice);
    dice_outcome_ = action;
    player_ = current_player_;
    return;
  }

  SPIEL_CHECK_GE(player_, 0);
  SPIEL_CHECK_LT(player_, num_players_);

  switch(phase_) {
    case Phase::kSelectDice:
      dice_ = static_cast<Die>(action);
      player_ = kChancePlayerId;
      phase_ = Phase::kRollDice;
      num_dice_rolls_ = 1;
      break;
    case Phase::kRollDice:
      if(action == 0) {
        SPIEL_CHECK_LT(num_dice_rolls_, kDefaultNumDiceRolls);
        ++num_dice_rolls_;
        player_ = kChancePlayerId;
      } else {
        phase_ = Phase::kSubmitPoints;
        player_ = kSimultaneousPlayerId;
      }
      break;
    case Phase::kSubmitPoints:
      SpielFatalError(absl::StrCat("Player ", player_, " is invalid for phase ", phase_));
      break;
  }
}

void QwintoState::DoApplyActions(const std::vector<Action>& actions) {
  // Check the actions are valid.
  SPIEL_CHECK_EQ(actions.size(), num_players_);
  SPIEL_CHECK_EQ(phase_, Phase::kSubmitPoints);

  for(int p = 0; p < num_players_; ++p) {
    if(actions[p] == kActionSkip) {
      SPIEL_CHECK_NE(p, current_player_);
      continue;
    } else if(actions[p] == kActionMiss) {
      SPIEL_CHECK_EQ(p, current_player_);
      scores_[p * (kDefaultNumDice * kDefaultNumFields + 1) + actions[p]] += kDefaultMissPoints;
    } else {
      scores_[p * (kDefaultNumDice * kDefaultNumFields + 1) + actions[p]] = dice_outcome_;
    }
  }

  phase_ = Phase::kSelectDice;
  current_player_ = (current_player_ + 1) % num_players_;
  player_ = current_player_;
}

std::vector<std::pair<Action, double>> QwintoState::ChanceOutcomes() const {
  SPIEL_CHECK_TRUE(IsChanceNode());

  int num_dice = ((dice_ & kOrange) != kInvalidDie) + ((dice_ & kYellow) != kInvalidDie) + ((dice_ & kPurple) != kInvalidDie);

  SPIEL_CHECK_GE(num_dice, 1);
  SPIEL_CHECK_LE(num_dice, kDefaultNumDice);

  std::vector<std::pair<Action, double>> outcomes;

  switch(num_dice) {
    case 1:
      outcomes = {
        {1, 1/6.}, {2, 1/6.}, {3, 1/6.}, {4, 1/6.}, {5, 1/6.}, {6, 1/6.}
      };
      break;
    case 2:
      outcomes = {
        {2, 1/36.}, {3, 2/36.}, {4, 3/36.}, {5, 4/36.}, {6, 5/36.}, {7, 6/36.}, {8, 5/36.}, {9, 4/36.}, {10, 3/36.}, {11, 2/36.}, {12, 1/36.}
      };
      break;
    case 3:
      outcomes = {
        {3, 1/216.}, {4, 3/216.}, {5, 6/216.}, {6, 10/216.}, {7, 15/216.}, {8, 21/216.}, {9, 25/216.}, {10, 27/216.},
        {11, 27/216.}, {12, 25/216.}, {13, 21/216.}, {14, 15/216.}, {15, 10/216.}, {16, 6/216.}, {17, 3/216.}, {18, 1/216.}
      };
      break;
  }

  return outcomes;
}

std::vector<Action> QwintoState::LegalActions(Player player) const {
  if (player == kSimultaneousPlayerId) return LegalFlatJointActions();
  if (player == kChancePlayerId) return LegalChanceOutcomes();
  if (player == kTerminalPlayerId) return std::vector<Action>();

  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);

  std::vector<Action> movelist;

  if(IsTerminal()) {
    return movelist;
  }

  switch(phase_) {
    case Phase::kSelectDice:
      //SPIEL_CHECK_EQ(player, current_player_);
      movelist = {kOrange, kPurple, kYellow, kOrange | kPurple, kOrange | kYellow, kPurple | kYellow, kOrange | kPurple | kYellow};
      break;
    case Phase::kRollDice:
      //SPIEL_CHECK_EQ(player, current_player_);
      if(num_dice_rolls_ < kDefaultNumDiceRolls) {
        movelist.push_back(0);
      }

      movelist.push_back(1);

      break;
    case Phase::kSubmitPoints:
      {
        auto scores = scores_.begin() + player * (kDefaultNumDice * kDefaultNumFields + 1);

        for(int i = 0; i < kDefaultNumDice * kDefaultNumFields; ++i) {
          auto die = kInvalidDie;

          switch(i / kDefaultNumFields) {
            case 0:
              die = kOrange;
              break;
            case 1:
              die = kYellow;
              break;
            case 2:
              die = kPurple;
              break;
          }

          // Skip rows of wrong color
          if((dice_ & die) == kInvalidDie) {
            continue;
          }

          // Skip filled fields
          if(scores[i] != 0) {
            continue;
          }

          // Skip fields with wrong order
          auto row = scores + (i / kDefaultNumFields) * kDefaultNumFields;
          int offset = i % kDefaultNumFields;

          if(!std::all_of(row, row + offset, [&](auto a) { return a < dice_outcome_; }) ||
             !std::all_of(row + offset + 1, row + kDefaultNumFields, [&](auto a) { return a == 0 || a > dice_outcome_; })) {
            continue;
          }

          auto is_valid_column = [&](int c) {
            if(c == 8 || c == 18) {
              return true;
            } else if(c == 13) {
              return scores[22] != dice_outcome_;
            } else if(c == 22) {
              return scores[13] != dice_outcome_;
            } else if(c == 9 || c == 19 || c == 2 || c == 12 || c == 7 || c == 17) {
              return scores[c] != dice_outcome_ && scores[(c + 10) % 20] != dice_outcome_;
            } else if(c == 3 || c == 23) {
              return scores[c] != dice_outcome_ && scores[(c + 20) % 40] != dice_outcome_;
            } else {
              return scores[c] != dice_outcome_ && scores[(c + 10) % 30] != dice_outcome_ && scores[(c + 20) % 30] != dice_outcome_;
            }
          };

          if(!is_valid_column(i)) {
            continue;
          }

          movelist.push_back(i);
        }

        if(player == current_player_) {
          movelist.push_back(kActionMiss);
        } else {
          movelist.push_back(kActionSkip);
        }

        break;
      }
  }

  std::sort(movelist.begin(), movelist.end());

  return movelist;
}

std::string QwintoState::ActionToString(Player player,
                                           Action action_id) const {
  if (player == kSimultaneousPlayerId)
    return FlatJointActionToString(action_id);

  if (player == kChancePlayerId) {
    SPIEL_CHECK_GE(action_id, 1);
    SPIEL_CHECK_LE(action_id, 18);

    return absl::StrCat("Dice outcome ", action_id);
  } else {
    switch(phase_) {
      case Phase::kSelectDice:
        return absl::StrCat("[P", player, "] Dice: ", action_id);
      case Phase::kRollDice:
        return absl::StrCat("[P", player, "] Take outcome: ", action_id);
      case Phase::kSubmitPoints:
        return absl::StrCat("[P", player, "] Field: ", action_id);
    }
  }
}

std::string QwintoState::ToString() const {
  std::string str;

  absl::StrAppend(&str, "Current player: ", current_player_, "\n");
  absl::StrAppend(&str, "Phase: ", phase_, "\n");
  absl::StrAppend(&str, "Dice: ", dice2str(dice_), "\n");
  absl::StrAppend(&str, "Roll: ", dice_outcome_, "\n");

  for(Player p{0}; p < num_players_; ++p) {
    auto scores = scores_.begin() + p * (kDefaultNumDice * kDefaultNumFields + 1);

    absl::StrAppend(&str, "      |", absl::Dec(scores[0], absl::kSpacePad2), "|", absl::Dec(scores[1], absl::kSpacePad2), "|", absl::Dec(scores[2], absl::kSpacePad2), "|  |", absl::Dec(scores[3], absl::kSpacePad2), "|", absl::Dec(scores[4], absl::kSpacePad2), "|", absl::Dec(scores[5], absl::kSpacePad2), "|", absl::Dec(scores[6], absl::kSpacePad2), "|", absl::Dec(scores[7], absl::kSpacePad2), "|", absl::Dec(scores[8], absl::kSpacePad2), "|\n");
    absl::StrAppend(&str, "    ", absl::Dec(scores[9], absl::kSpacePad2), "|", absl::Dec(scores[10], absl::kSpacePad2), "|", absl::Dec(scores[11], absl::kSpacePad2), "|", absl::Dec(scores[12], absl::kSpacePad2), "|", absl::Dec(scores[13], absl::kSpacePad2), "|  |", absl::Dec(scores[14], absl::kSpacePad2), "|", absl::Dec(scores[15], absl::kSpacePad2), "|", absl::Dec(scores[16], absl::kSpacePad2), "|", absl::Dec(scores[17], absl::kSpacePad2), "|\n");
    absl::StrAppend(&str, "|", absl::Dec(scores[18], absl::kSpacePad2), "|", absl::Dec(scores[19], absl::kSpacePad2), "|", absl::Dec(scores[20], absl::kSpacePad2), "|", absl::Dec(scores[21], absl::kSpacePad2), "|  |", absl::Dec(scores[22], absl::kSpacePad2), "|", absl::Dec(scores[23], absl::kSpacePad2), "|", absl::Dec(scores[24], absl::kSpacePad2), "|", absl::Dec(scores[25], absl::kSpacePad2), "|", absl::Dec(scores[26], absl::kSpacePad2), "|\n");
    absl::StrAppend(&str, "Miss: ", scores[27], "\n");

    //absl::StrAppendFormat(&str, "      |% 2u|% 2u|% 2u|  |% 2u|% 2u|% 2u|% 2u|% 2u|% 2u|\n", scores[0], scores[1], scores[2], scores[3], scores[4], scores[5], scores[6], scores[7], scores[8]);
    //absl::StrAppendFormat(&str, "    % 2u|% 2u|% 2u|% 2u|% 2u|  |% 2u|% 2u|% 2u|% 2u|\n", scores[9], scores[10], scores[11], scores[12], scores[13], scores[14], scores[15], scores[16], scores[17]);
    //absl::StrAppendFormat(&str, "|% 2u|% 2u|% 2u|% 2u|  |% 2u|% 2u|% 2u|% 2u|% 2u|\n", scores[18], scores[19], scores[20], scores[21], scores[22], scores[23], scores[24], scores[25], scores[26]);
  }

  return str;
}

bool QwintoState::IsTerminal() const {
  constexpr int offset = kDefaultNumDice * kDefaultNumFields + 1;

  for(Player p = 0; p < num_players_; ++p) {
    if(scores_[(p + 1) * offset - 1] <= kDefaultTerminationPoints) {
      return true;
    }
  }

  return false;
}

std::vector<double> QwintoState::Returns() const {
  if (!IsTerminal()) {
    return std::vector<double>(num_players_, 0.0);
  }

  std::vector<double> returns(num_players_, 0);

  for (Player p = 0; p < num_players_; ++p) {
    auto scores = scores_.begin() + p * (kDefaultNumDice * kDefaultNumFields + 1);

    // Red
    if(int count = std::count_if(scores, scores + kDefaultNumFields, [](int a) { return a > 0; }); count == kDefaultNumFields) {
      returns[p] += scores[kDefaultNumFields - 1];
    } else {
      returns[p] += count;
    }

    scores += kDefaultNumFields;

    // Yellow
    if(int count = std::count_if(scores, scores + kDefaultNumFields, [](int a) { return a > 0; }); count == kDefaultNumFields) {
      returns[p] += scores[kDefaultNumFields - 1];
    } else {
      returns[p] += count;
    }

    scores += kDefaultNumFields;

    // Purple
    if(int count = std::count_if(scores, scores + kDefaultNumFields, [](int a) { return a > 0; }); count == kDefaultNumFields) {
      returns[p] += scores[kDefaultNumFields - 1];
    } else {
      returns[p] += count;
    }

    scores = scores_.begin() + p * (kDefaultNumDice * kDefaultNumFields + 1);

    if(scores[0] > 0 && scores[10] > 0 && scores[20] > 0) {
      returns[p] += scores[20];
    }

    if(scores[1] > 0 && scores[11] > 0 && scores[21] > 0) {
      returns[p] += scores[1];
    }

    if(scores[4] > 0 && scores[14] > 0 && scores[24] > 0) {
      returns[p] += scores[4];
    }

    if(scores[5] > 0 && scores[15] > 0 && scores[25] > 0) {
      returns[p] += scores[15];
    }

    if(scores[6] > 0 && scores[16] > 0 && scores[26] > 0) {
      returns[p] += scores[26];
    }

    // Account for miss points
    returns[p] += scores[kDefaultNumDice * kDefaultNumFields];
  }

  return returns;
}

#if 0
std::string QwintoState::ObservationString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);

  // Perfect info case, show:
  //   - current point card
  //   - everyone's current points
  //   - everyone's current hands
  // Imperfect info case, show:
  //   - current point card
  //   - everyone's current points
  //   - my current hand
  //   - current win sequence
  std::string current_trick =
      absl::StrCat("Current point card: ", CurrentPointValue());
  std::string points_line = "Points: ";
  std::string hands = "";
  std::string win_seq = "Win Sequence: ";
  for (auto p = Player{0}; p < num_players_; ++p) {
    absl::StrAppend(&points_line, points_[p], " ");
  }

  if (impinfo_) {
    // Only my hand
    absl::StrAppend(&hands, "P", player, " hand: ");
    for (int c = 0; c < num_cards_; ++c) {
      if (player_hands_[player][c]) {
        absl::StrAppend(&hands, c + 1, " ");
      }
    }
    absl::StrAppend(&hands, "\n");

    // Show the win sequence.
    for (int i = 0; i < win_sequence_.size(); ++i) {
      absl::StrAppend(&win_seq, win_sequence_[i], " ");
    }
    return absl::StrCat(current_trick, "\n", points_line, "\n", hands, win_seq,
                        "\n");
  } else {
    // Show the hands in the perfect info case.
    for (auto p = Player{0}; p < num_players_; ++p) {
      absl::StrAppend(&hands, "P", p, " hand: ");
      for (int c = 0; c < num_cards_; ++c) {
        if (player_hands_[p][c]) {
          absl::StrAppend(&hands, c + 1, " ");
        }
      }
      absl::StrAppend(&hands, "\n");
    }
    return absl::StrCat(current_trick, "\n", points_line, "\n", hands);
  }
}
#endif

void QwintoState::ObservationTensor(Player player,
                                       std::vector<double>* values) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);

  values->clear();
  values->reserve(game_->ObservationTensorSize());

  // Phase
  values->push_back(phase_ == Phase::kSelectDice);
  values->push_back(phase_ == Phase::kRollDice);
  values->push_back(phase_ == Phase::kSubmitPoints);

  for(int i = 0; i < kDefaultNumDiceRolls; ++i) {
    values->push_back(i == num_dice_rolls_);
  }

  // Dice
  values->push_back(dice_ & kOrange);
  values->push_back(dice_ & kYellow);
  values->push_back(dice_ & kPurple);

  // Dice outcome
  for(int i = 1; i <= 18; ++i) {
    values->push_back(i == dice_outcome_);
  }

  // Current player
  for(int p = 0; p < num_players_; ++p) {
    values->push_back(p == current_player_);
  }

  // Boards
  for(auto s : scores_) {
    values->push_back(s);
  }

  SPIEL_CHECK_EQ(values->size(), game_->ObservationTensorSize());
}

std::unique_ptr<State> QwintoState::Clone() const {
  return std::unique_ptr<State>(new QwintoState(*this));
}

QwintoGame::QwintoGame(const GameParameters& params)
    : Game(kGameType, params),
      num_players_(ParameterValue<int>("players")) {
}

std::unique_ptr<State> QwintoGame::NewInitialState() const {
  return std::unique_ptr<State>(new QwintoState(shared_from_this()));
}

int QwintoGame::MaxChanceOutcomes() const {
  return 16;
}

std::vector<int> QwintoGame::ObservationTensorShape() const {
    return {
      // 1-hot encoding of phase
      3 +
      // 1-hot encoding of dice rolls
      kDefaultNumDiceRolls +
      // 1-hot encoding of selected dice
      kDefaultNumDice +
      // 1-hot encoding of dice outcome
      18 +
      // 1-hot encoding of current player
      num_players_ + 
      // encoding of boards      
      num_players_ * (kDefaultNumDice * kDefaultNumFields + 1)
    };
}

double QwintoGame::MinUtility() const {
  return kDefaultTerminationPoints;
}

double QwintoGame::MaxUtility() const {
  return 3 * 18 + 12 + 11 + 14 + 16 + 18;
}

}  // namespace qwinto
}  // namespace open_spiel
