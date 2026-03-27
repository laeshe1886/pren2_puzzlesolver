#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__

#include "command_dispatcher.h"
#include "piston.h"

/**
 * @brief Initialisiert die State Machine und ihre internen Variablen.
 * * @param dispatcher Zeiger auf den Command Dispatcher (UART/Nanopb)
 * @param piston Zeiger auf das Piston-Modul (Greifer)
 */
void StateMachine_Init(CommandDispatcher_t* dispatcher, Piston_t* piston);

/**
 * @brief Muss zyklisch in der Hauptschleife aufgerufen werden.
 * Steuert den Ablauf, blockiert nicht.
 */
void StateMachine_Update(void);

#endif /* __STATE_MACHINE_H__ */