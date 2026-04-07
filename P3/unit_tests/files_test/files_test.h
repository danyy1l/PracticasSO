/**
 * @brief Declara los tests para el modulo de archivos y semaforos (files.c)
 *
 * @file files_test.h
 * @author Danyyil Shykerynets & Fernando Blanco
 * @version 2.0
 * @date 2026-04-06
 */

#ifndef _FILES_TEST_H
#define _FILES_TEST_H

/**
 * @test Test open_pipes
 * @post Los descriptores de archivo se asignan correctamente
 */
void test1_open_pipes();

/**
 * @test Test close_pipes
 * @post Las tuberías se cierran y los descriptores se resetean
 */
void test2_close_pipes();

/**
 * @test Test inicialización de semáforos
 */
void test3_mutex_init();

/**
 * @test Test lectura y escritura de PIDs
 */
void test4_pids_io();

/**
 * @test Test del sistema de votaciones
 */
void test5_voting_system();

#endif
