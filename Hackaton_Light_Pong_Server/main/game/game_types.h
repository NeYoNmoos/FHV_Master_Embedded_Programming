/**
 * @file game_types.h
 * @brief Game data type definitions
 */

#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Game score structure
     */
    typedef struct
    {
        uint8_t score_1;
        uint8_t score_2;
    } game_score_t;

#ifdef __cplusplus
}
#endif

#endif // GAME_TYPES_H
