/**
 * @brief Declara los tests para el modulo de archivos y semaforos (files.c)
 *
 * @file files_test.h
 * @author Danyyil Shykerynets & Fernando Blanco
 * @version 2.0
 * @date 2026-04-04
 */

#ifndef _FILES_TEST_H
#define _FILES_TEST_H

/**
 * @test Test inicializacion de semaforos
 * @post Los semáforos se crean correctamente y no devuelven SEM_FAILED
 */
void test1_mutex_init();

/**
 * @test Test de escritura y lectura del target
 * @post Lo que se escribe en el archivo es exactamente lo que se lee
 */
void test1_target_io();

/**
 * @test Test del sistema de votaciones Aprobado
 * @post Con mayoría de 'Y' la función devuelve true y cuenta bien los votos
 */
void test1_voting_accepted();

/**
 * @test Test del sistema de votaciones Rechazado
 * @post Con mayoria de 'N' o sin 'Y' la funcion devuelve false
 */
void test2_voting_rejected();

#endif
