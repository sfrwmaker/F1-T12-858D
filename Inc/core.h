/*
 * core.h
 *
 *  Created on: 16 sep 2019
 *      Author: Alex
 */

#ifndef CORE_H_
#define CORE_H_

#include "main.h"

#ifndef __cplusplus
typedef	uint8_t	bool;
#endif

// Forward function declaration
bool isACsine(void);

#ifdef __cplusplus
extern "C" {
#endif

void setup(void);
void loop(void);

#ifdef __cplusplus
}
#endif

#endif
