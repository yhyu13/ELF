//
// Copyright (c) 2016-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
//
#pragma once

#include <stdio.h>
#include <memory.h>
#include "common.h"

// This can be changed to 9x9
constexpr int BOARD_SIZE = 19;

// 19x19 only
#define STAR_ON19(i, j) ( ((i) == 3 || (i) == 9 || (i) == 15) && ((j) == 3 || (j) == 9 || (j) == 15) )
// 9x9 only
#define STAR_ON9(i, j) ( ( ((i) == 2 || (i) == 6) && ((j) == 2 || (j) == 6) ) || (i == 4 && j == 4) )

#define STAR STAR_ON19

constexpr int BOARD_MARGIN = 1;
constexpr int BOARD_EXPAND_SIZE = BOARD_SIZE + 2;
constexpr int NUM_INTERSECTION = BOARD_SIZE*BOARD_SIZE;

typedef unsigned char Status;
typedef unsigned char ShowChoice;
#define SHOW_NONE 0
#define SHOW_LAST_MOVE 1
#define SHOW_ROWS 2
#define SHOW_COLS 3
#define SHOW_ALL 4
#define SHOW_ALL_ROWS_COLS 5

typedef struct {
  // Group id of which the stone belongs to.
  Stone color;
  // id == 0 means that it is an empty intersection. id == MAX_GROUP means it is the border.
  unsigned char id;
  // The next location on the board.
  Coord next;
  // History information, what is the last time that the stone has placed.
  unsigned short last_placed;
} Info;

typedef struct {
  Stone color;
  Coord start;
  short stones;
  short liberties;
} Group;

typedef struct {
  Coord c;
  Stone player;
  short ids[4];
  Stone colors[4];
  short group_liberties[4];
  short liberty;
} GroupId4;

// How many live groups can possibly be there in a game?
// We use 173 so that sizeof(MBoard) <= 4096. This is important for atomic data transmission using pipe.
#define MAX_GROUP 173
/*
Next step
1. No PASS handling. need to add.
2. A move violation would not clear simple_ko. Need to fix.
*/

// Maximum possible value of coords.
constexpr int BOUND_COORD = BOARD_EXPAND_SIZE * BOARD_EXPAND_SIZE;

// Board
typedef struct {
  // Board
  Info _infos[BOARD_EXPAND_SIZE * BOARD_EXPAND_SIZE];

  // Group info
  Group _groups[MAX_GROUP];
  // Number of groups, including group 0 (empty intersection). So for empty board, _num_groups == 1.
  short _num_groups;

  // After each play, the number of possible empty groups cannot exceed 4.
  // unsigned short _empty_group_ids[4];
  // int _next_empty_group;

  // Capture info
  short _b_cap;
  short _w_cap;

  // Passes in rollout (B-W) for score parity
  short _rollout_passes;

  // Last played location
  Coord _last_move;
  Coord _last_move2;
  Coord _last_move3;
  Coord _last_move4;

  // These record the group ids that change during play. (Useful for external function to modify their records accordingly).
  // _removed_group_ids recorded all the group ids that are to be killed,
  // _num_group_removed is the number of groups to be replaced, it is always in [0, 4].
  unsigned char _removed_group_ids[4];
  unsigned char _num_group_removed;

  // Simple ko point.
  // _ko_age. _ko_age == 0 mean the ko is recent and cannot be violated. ko_age_ increases when play continues,
  // until it is refreshed by a new ko.
  unsigned short _ko_age;
  Coord _simple_ko;
  Stone _simple_ko_color;

  // Next player.
  Stone _next_player;

  // The current ply number, it will be increase after each play.
  // The initial ply number is 1.
  short _ply;
  // uint64_t hash for the current board situatons. If we want to enable it, then make MAX_GROUP smaller.
  // uint64_t hash;
} Board;

// Save all candidate moves.
typedef struct {
  const Board *board;
  Coord moves[BOARD_SIZE*BOARD_SIZE];
  int num_moves;
} AllMoves;

#define OPPONENT(p) ((Stone)(S_WHITE + S_BLACK - (int)(p)))
#define HAS_STONE(s) ( ((s) == S_BLACK) || ((s) == S_WHITE) )
#define EMPTY(s) ( (s) == S_EMPTY )
#define ONBOARD(s) ( (s) != S_OFF_BOARD )

// It means the intersection has no stone, or the group id is null.
#define G_EMPTY(id) ( (id) == 0 )
#define G_ONBOARD(id) ( (id) != MAX_GROUP )
#define G_HAS_STONE(id) ( (id) > 0 && (id) < MAX_GROUP )

// Coordinate/Move
// X first, then y.
#define X(c) ( (c) % BOARD_EXPAND_SIZE - 1 )
#define Y(c) ( (c) / BOARD_EXPAND_SIZE - 1 )
#define ON_BOARD(x, y) ( (x) >= 0 && (x) < BOARD_SIZE && (y) >= 0 && (y) < BOARD_SIZE )
#define EXTENDOFFSETXY(x, y)  ( (y) * BOARD_EXPAND_SIZE + (x) )
#define OFFSETXY(x, y)  ( ((y) + BOARD_MARGIN) * BOARD_EXPAND_SIZE + (x) + BOARD_MARGIN )

// Warning, the export offset is different from internal representation
// Internal representation: y * stride + x
// External representation: x * stride + y
#define EXPORT_OFFSET_XY(x, y) ( (x) * BOARD_SIZE + (y) )
#define EXPORT_OFFSET(c) ( (X(c)) * BOARD_SIZE + (Y(c)) )
#define EXPORT_X(a) ((a) / BOARD_SIZE)
#define EXPORT_Y(a) ((a) % BOARD_SIZE)
#define EXPORT_OFFSET_PLANE(c, plane) ( (X(c)) * BOARD_SIZE + (Y(c)) + BOARD_SIZE * BOARD_SIZE * (plane) )

#define ATXY(b, x, y) b[OFFSETXY(x, y)]
#define AT(b, c) b[c]

// Faster access to neighbors.
// L/R means x-1/x+1
// T/B means y-1/y+1
#define L(c) ((c) - 1)
#define R(c) ((c) + 1)
#define T(c) ((c) - BOARD_EXPAND_SIZE)
#define B(c) ((c) + BOARD_EXPAND_SIZE)
#define LT(c) ((c) - 1 - BOARD_EXPAND_SIZE)
#define LB(c) ((c) - 1 + BOARD_EXPAND_SIZE)
#define RT(c) ((c) + 1 - BOARD_EXPAND_SIZE)
#define RB(c) ((c) + 1 + BOARD_EXPAND_SIZE)
#define LL(c) ((c) - 2)
#define RR(c) ((c) + 2)
#define TT(c) ((c) - 2*BOARD_EXPAND_SIZE)
#define BB(c) ((c) + 2*BOARD_EXPAND_SIZE)
#define NEIGHBOR4(c1, c2) (abs((c1) - (c2)) == 1 || abs((c1) - (c2)) == BOARD_EXPAND_SIZE)
#define NEIGHBOR8(c1, c2) (abs((c1) - (c2)) == 1 || abs(abs((c1) - (c2)) - BOARD_EXPAND_SIZE) < 2)

// Left, top, right, bottom
static const int delta4[4] = { -1, - BOARD_EXPAND_SIZE, +1, BOARD_EXPAND_SIZE };

// LT, LB, RT, RB
static const int diag_delta4[4] = {  -1 - BOARD_EXPAND_SIZE, -1 + BOARD_EXPAND_SIZE, 1 - BOARD_EXPAND_SIZE, 1 + BOARD_EXPAND_SIZE };

static const int delta8[8] = { -1, - BOARD_EXPAND_SIZE, +1, BOARD_EXPAND_SIZE, -1 - BOARD_EXPAND_SIZE, -1 + BOARD_EXPAND_SIZE, 1 - BOARD_EXPAND_SIZE, 1 + BOARD_EXPAND_SIZE };

// Loop through the group link table
#define TRAVERSE(b, id, c) \
  for (Coord c = b->_groups[id].start; c != 0; c = b->_infos[c].next) { \

#define ENDTRAVERSE }

// Local check
#define FOR4(c, ii, cc) \
  for (int ii = 0; ii < 4; ++ii) { \
    Coord cc = c + delta4[ii]; \

#define ENDFOR4 }

#define FORDIAG4(c, ii, cc) \
for (int ii = 0; ii < 4; ++ii) { \
  Coord cc = c + diag_delta4[ii]; \

#define ENDFORDIAG4 }

#define FOR8(c, ii, cc) \
  for (int ii = 0; ii < 8; ++ii) { \
    Coord cc = c + delta8[ii]; \

#define ENDFOR8 }

// Traverse all board.
#define ALLBOARD(b) \
for (int j = BOARD_SIZE - 1; j >= 0; --j) { \
  for (int i = 0; i < BOARD_SIZE; ++i) { \
        Coord c = OFFSETXY(i, j); \


#define ENDALLBOARD \
    } \
    printf("\n"); \
  } \

//
#define ALL_EXPAND_BOARD(b) \
for (int j = BOARD_EXPAND_SIZE - 1; j >= 0; --j) { \
  for (int i = 0; i < BOARD_EXPAND_SIZE; ++i) { \
      Coord c = EXTENDOFFSETXY(i, j); \


#define END_ALL_EXPAND_BOARD \
    } \
    printf("\n"); \
  } \
//

// Zero based.
extern inline Coord GetCoord(int x, int y) { return OFFSETXY(x, y); }

void ClearBoard(Board *board);
void CopyBoard(Board *dst, const Board* src);
bool CompareBoard(const Board *b1, const Board *b2);
// Return true if the move is valid and can be played, if so, properly set up ids
// Otherwise return false.
bool TryPlay(const Board *board, int x, int y, Stone player, GroupId4 *ids);
// Simple version of it.
bool TryPlay2(const Board *board, Coord m, GroupId4 *ids);

// Actually play the game. If return true, then the game ended (either by PASS + PASS or by RESIGN)
bool Play(Board *board, const GroupId4 *ids);

// Place handicap stone.
bool PlaceHandicap(Board *board, int x, int y, Stone player);

// Undo pass, currently we only support undo at most 2 passes.
// Return true if last_move_ is pass.
// After Undo, last_move4 is not usable.
bool UndoPass(Board *board);

// A region [left, right) * [top, bottom).
typedef struct {
  int left, top, right, bottom;
} Region;

bool IsIn(const Region *region, Coord c);
void Expand(Region *region, Coord c);
bool GroupInRegion(const Board *board, short group_idx, const Region *r);

// Pretty slow. Need some improvements.
// Find all valid moves excluding self-atari.
void FindAllCandidateMoves(const Board* board, Stone player, int self_atari_thres, AllMoves *all_moves);
void FindAllCandidateMovesInRegion(const Board* board, const Region *r, Stone player, int self_atari_thres, AllMoves *all_moves);

// Find all valid moves including self-atari.
void FindAllValidMoves(const Board* board, Stone player, AllMoves *all_moves);
void ShowBoardFancy(const Board *board, ShowChoice choice);
void ShowBoard2Buf(const Board *board, ShowChoice choice, char *buf);
void ShowBoard(const Board *board, ShowChoice choice);
void DumpBoard(const Board *board);
void VerifyBoard(Board* board);

// The following two are useful for tsumego
// Find all valid moves within region. Useful for tsumego solver.
void FindAllValidMovesInRegion(const Board *board, const Region *region, AllMoves *all_moves);
void GetBoardBBox(const Board *board, Region *region);

// Given a region surrounding the L&D problem, guess who is the attacker.
Stone GuessLDAttacker(const Board *board, const Region *r);

void GetAllStones(const Board *board, AllMoves *black, AllMoves *white);

// Get the group remove/replace sequence.
// Return the length of remove/replace sequence.
int GetGroupReplaceSeq(const Board *board, unsigned char removed[4], unsigned char replaced[4]);
// Convert the board id from old (before the previous move was taken) and the new.
unsigned char BoardIdOld2New(const Board *board, unsigned char id);

// Check at least one group of player lives within the region.
// If region == NULL, then search the entire board.
bool OneGroupLives(const Board *board, Stone player, const Region *region);

// Some function to check whether a move is valid.
// If num_stones != NULL, then num_stones will be assigned to the number of stones after merging.
bool IsSelfAtari(const Board *board, const GroupId4 *ids, Coord c, Stone player, int *num_stones);
bool IsSelfAtariXY(const Board *board, const GroupId4 *ids, int x, int y, Stone player, int *num_stones);

// Find liberties of a certain group
bool find_only_liberty(const Board *b, short id, Coord *m);
bool find_two_liberties(const Board *b, short id, Coord m[2]);

// Ladder check.
// Return 0 if no ladder. Otherwise return the depth of ladder.
int CheckLadder(const Board *board, const GroupId4 *ids, Stone player);
// Whether the move will lead to a simple ko.
bool IsMoveGivingSimpleKo(const Board *board, const GroupId4 *ids, Stone player);
// Get the current simple ko location.
Coord GetSimpleKoLocation(const Board *board, Stone *player);

// Check if the game has ended
bool IsGameEnd(const Board *board);

void GetAllEmptyLocations(const Board* board, AllMoves *all_moves);

bool IsEye(const Board *board, Coord c, Stone player);
bool IsSemiEye(const Board *board, Coord c, Stone player, Coord *move);
bool IsFakeEye(const Board *board, Coord c, Stone player);
bool IsTrueEye(const Board *board, Coord c, Stone player);
bool IsTrueEyeXY(const Board *board, int x, int y, Stone player);
Stone GetEyeColor(const Board *board, Coord c);

typedef int GoRule;
#define RULE_CHINESE 0
#define RULE_JAPANESE 1

// Some definition for Board ownership
#define S_DAME 3

// DEAD/ALIVE/UNKNOWN are bits superimposed to the existing stone.
#define S_UNKNOWN 4
#define S_DEAD 8
#define S_ALIVE 16

// Compute board scores (no KOMI included)
// The score is used after almost all intersections of the board are filled.
float GetFastScore(const Board *board, const int rule);
// Get the official score. deadgroups is an array with num_group element.
// If deadgroups is NULL, then all groups are alive.
// If territory is not NULL, will also return the territory (S_BLACK/S_WHITE/S_DAME)
// For now I just implemented the Chinese rule.
//   1. Replace deadstone with opponent live stone (for faster flood-fill). But these replaced stones will be deducted from the final score.
//   2. An empty intersection is considered a black/white territory when it is only connected with black/white live stones.
//   3. Black score = Black territory + black live stones.
//   4. White score = White territory + white live stones.
float GetTrompTaylorScore(const Board *board, const Stone *group_stats, Stone *territory);

// Get features.
bool GetLibertyMap(const Board* board, Stone player, float* data);
bool GetLibertyMap3(const Board* board, Stone player, float* data);
bool GetLibertyMap3binary(const Board* board, Stone player, float* data);
bool GetStones(const Board* board, Stone player, float *data);
bool GetSimpleKo(const Board* board, Stone player, float *data);
bool GetHistory(const Board* board, Stone player, float *data);
bool GetDistanceMap(const Board* board, Stone player, float *data);

// Some utility functions.
char *get_move_str(Coord m, Stone player, char *buf);
void util_show_move(Coord m, Stone player, char *buf);

