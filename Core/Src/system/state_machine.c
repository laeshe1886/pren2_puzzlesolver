#include "main.h"  // Für 'piston_moving' (später durch Endschalter ersetzen)

#include "state_machine.h"

#include "motion_planner.h"
#include "step_generator.h"

// Interne Zustände
typedef enum {
  SM_IDLE,
  SM_CALC_TO_PICK,
  SM_MOVE_TO_PICK,
  SM_GRAB_PIECE,
  SM_LIFT_PIECE,
  SM_MOVE_TO_PLACE,
  SM_ROTATE_PIECE,
  SM_RELEASE_PIECE,
  SM_LIFT_EMPTY,
  SM_NEXT_PIECE
} State_e;


// Gekapselte Variablen der State Machine
static State_e current_state;
static CommandDispatcher_t* sm_dispatcher;
static Piston_t* sm_piston;

static PuzzleCommand current_puzzle;
static uint8_t current_piece_idx;
static MoveBlock_t active_move;

void StateMachine_Init(CommandDispatcher_t* dispatcher, Piston_t* piston) {
  sm_dispatcher = dispatcher;
  sm_piston = piston;
  current_state = SM_IDLE;
  current_piece_idx = 0;

  MotionPlanner_Init();
}

void StateMachine_Update(void) {
  switch (current_state) {
    case SM_IDLE:
      if (CommandDispatcher_HasCommand(sm_dispatcher)) {
        // Neues Kommando lokal kopieren
        current_puzzle = *CommandDispatcher_GetCommand(sm_dispatcher);
        current_piece_idx = 0;

        if (current_puzzle.pieces_count > 0) {
          current_state = SM_CALC_TO_PICK;
          // Sende Status: Beschäftigt
          CommandDispatcher_SendAck(sm_dispatcher, Status_STATUS_BUSY, 0);
        }
      }
      break;

    case SM_CALC_TO_PICK: {
      PieceCommand* piece = &current_puzzle.pieces[current_piece_idx];

      // Motion Planner übernimmt die Millimeter-zu-Schritte Rechnung
      active_move = MotionPlanner_PlanMoveToMM(piece->pick_x, piece->pick_y);
      StepGenerator_StartStep(&active_move);

      current_state = SM_MOVE_TO_PICK;
      break;
    }

    case SM_MOVE_TO_PICK:
      if (!StepGenerator_IsBusy()) {
        Piston_Set(sm_piston, PISTON_POS_GRAB);
        current_state = SM_GRAB_PIECE;
      }
      break;

    case SM_GRAB_PIECE:
      if (!piston_moving) {
        Piston_Set(sm_piston, PISTON_POS_MOVE);
        current_state = SM_LIFT_PIECE;
      }
      break;

    case SM_LIFT_PIECE:
      if (!piston_moving) {
        PieceCommand* piece = &current_puzzle.pieces[current_piece_idx];

        // Weg zum Zielort berechnen
        active_move =
            MotionPlanner_PlanMoveToMM(piece->place_x, piece->place_y);
        StepGenerator_StartStep(&active_move);

        current_state = SM_MOVE_TO_PLACE;
      }
      break;

    case SM_MOVE_TO_PLACE:
      if (!StepGenerator_IsBusy()) {
        current_state = SM_ROTATE_PIECE;
      }
      break;

    case SM_ROTATE_PIECE:
      // Hier kommt später der Code für den Rotationsmotor hin
      Piston_Set(sm_piston, PISTON_POS_RELEASE);
      current_state = SM_RELEASE_PIECE;
      break;

    case SM_RELEASE_PIECE:
      if (!piston_moving) {
        Piston_Set(sm_piston, PISTON_POS_MOVE);
        current_state = SM_LIFT_EMPTY;
      }
      break;

    case SM_LIFT_EMPTY:
      if (!piston_moving) {
        PieceCommand* piece = &current_puzzle.pieces[current_piece_idx];

        // Feedback an PC: Ein Teil erfolgreich platziert!
        CommandDispatcher_SendAck(
            sm_dispatcher, Status_STATUS_OK, piece->piece_id);

        current_state = SM_NEXT_PIECE;
      }
      break;

    case SM_NEXT_PIECE:
      current_piece_idx++;

      if (current_piece_idx < current_puzzle.pieces_count) {
        current_state = SM_CALC_TO_PICK;  // Nächstes Teil
      } else {
        // Das ganze Puzzle ist fertig
        CommandDispatcher_SendAck(sm_dispatcher, Status_STATUS_DONE, 0);
        current_state = SM_IDLE;  // Warten auf neues Paket vom PC
      }
      break;
  }
}