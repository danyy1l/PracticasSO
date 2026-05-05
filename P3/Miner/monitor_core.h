/**
 * @file monitor_core.h
 * @brief Cabeceras de las funciones principales del monitor y comprobador
 * @author Fernando Blanco & Danyyil Shykerynets
 *
 * @version 1.0
 * @Copyright (c) 2026 Author. All Rights Reserved.
 */
#ifndef _MONITOR_CORE_H
#define _MONITOR_CORE_H

#include "shared.h"
#include "types.h"
#include <signal.h>

/* Bandera global para detener el sistema de forma segura desde el main o
 * handlers */
extern volatile sig_atomic_t stop_monitor;

/**
 * @brief Configura las señales del proceso monitor (SIGINT)
 */
void setup_interrupt(void);

/**
 * @brief Logica del proceso padre (Productor)
 * Valida mensajes y escribe en el buffer
 */
void comprobador(u64 lag_comprobador, SharedMinerData *miner_shm,
                 SharedMonitorData *monitor_shm, mqd_t miner_mq);

/**
 * @brief Logica del proceso hijo (Consumidor)
 * Lee del buffer e imprime resultados
 */
void monitor(u64 lag_monitor, SharedMonitorData *monitor_shm);

#endif
