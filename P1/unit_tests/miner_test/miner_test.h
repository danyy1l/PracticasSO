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
 * @test Test pow_seek
 * @pre void* to thread_args
 * @return Pointer to POW solution
 */
void test1_pow_seek();
void test2_pow_seek();
void test3_pow_seek();

#endif
