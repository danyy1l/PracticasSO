/**
 * @brief Declara los tests para el modulo miner
 *
 * @file miner_test.h
 * @author Danyyil Shykerynets
 * @version 1.0
 * @date 01-03-2025
 * @Copyright (c) 2026 Author. All Rights Reserved.
 */

#ifndef _MINER_TEST_H
#define _MINER_TEST_H

/**
 * @test Test open_pipes
 * @pre Pointers to file descriptor arrays
 * @post Initialized pipes in fd pointers
 */
void test1_open_pipes();
void test2_open_pipes();

/**
 * @test Test calcular_solucion
 */
void test1_calcular_solucion();

/**
 * @test Test wait_votes (Success)
 */
void test1_wait_votes_success();

/**
 * @test Test wait_votes (fracaso timeout)
 * @post Aborta la espera si el tiempo se agota, aunque falten votos.
 */
void test2_wait_votes_timeout();

/**
 * @test Test wait_more_miners (Success)
 */
void test1_wait_more_miners_success();

/**
 * @test Test wait_more_miners (fracaso timeout)
 * @post La barrera se rompe y sale si el tiempo de vida del minero se acaba
 */
void test2_wait_more_miners_timeout();

#endif
