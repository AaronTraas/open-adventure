#include <stdlib.h>
#include "advent.h"
#include "newdb.h"

/*
 * scoring and wrap-up
 */

static long mxscor;	/* ugh..the price for having score() not exit. */

long score(enum termination mode)
/* mode is 'scoregame' if scoring, 'quitgame' if quitting, 'endgame' if died
 * or won */
{
    long score = 0;

    /*  The present scoring algorithm is as follows:
     *     Objective:          Points:        Present total possible:
     *  Getting well into cave   25                    25
     *  Each treasure < chest    12                    60
     *  Treasure chest itself    14                    14
     *  Each treasure > chest    16                   224
     *  Surviving             (MAX-NUM)*10             30
     *  Not quitting              4                     4
     *  Reaching "game.closng"   25                    25
     *  "Closed": Quit/Killed    10
     *            Klutzed        25
     *            Wrong way      30
     *            Success        45                    45
     *  Came to Witt's End        1                     1
     *  Round out the total       2                     2
     *                                       TOTAL:   430
     *  Points can also be deducted for using hints or too many turns, or for
     *  saving intermediate positions. */

    /*  First tally up the treasures.  Must be in building and not broken.
     *  Give the poor guy 2 points just for finding each treasure. */
    mxscor = 0;
    for (int i = 1; i <= NOBJECTS; i++) {
	if (!objects[i].is_treasure)
	    continue;
        if (objects[i].inventory != 0) {
            long k = 12;
            if (i == CHEST)k = 14;
            if (i > CHEST)k = 16;
            if (game.prop[i] >= 0)
                score += 2;
            if (game.place[i] == LOC_BUILDING && game.prop[i] == 0)
                score += k - 2;
            mxscor += k;
        }
    }

    /*  Now look at how he finished and how far he got.  NDEATHS and
     *  game.numdie tell us how well he survived.  game.dflag will tell us
     *  if he ever got suitably deep into the cave.  game.closng still
     *  indicates whether he reached the endgame.  And if he got as far as
     *  "cave closed" (indicated by "game.closed"), then bonus is zero for
     *  mundane exits or 133, 134, 135 if he blew it (so to speak). */
    score += (NDEATHS - game.numdie) * 10;
    mxscor += NDEATHS * 10;
    if (mode == endgame)
        score += 4;
    mxscor += 4;
    if (game.dflag != 0)score += 25;
    mxscor += 25;
    if (game.closng)score += 25;
    mxscor += 25;
    if (game.closed) {
        if (game.bonus == 0)
            score += 10;
        if (game.bonus == SPLATTER_MESSAGE)
            score += 25;
        if (game.bonus == DEFEAT_MESSAGE)
            score += 30;
        if (game.bonus == VICTORY_MESSAGE)
            score += 45;
    }
    mxscor += 45;

    /* Did he come to Witt's End as he should? */
    if (game.place[MAGAZINE] == LOC_WITTSEND)
        score += 1;
    mxscor += 1;

    /* Round it off. */
    score += 2;
    mxscor += 2;

    /* Deduct for hints/turns/saves. Hints < 4 are special; see database desc. */
    for (long i = 0; i < NHINTS; i++) {
        if (game.hinted[i])
            score = score - hints[i].penalty;
    }
    if (game.novice)
        score -= 5;
    if (game.clshnt)
        score -= 10;
    score = score - game.trnluz - game.saved;

    /* Return to score command if that's where we came from. */
    if (mode == scoregame) {
        rspeak(GARNERED_POINTS, score, mxscor, game.turns, game.turns);
    }

    return score;
}

void terminate(enum termination mode)
/* End of game.  Let's tell him all about it. */
{
    long points = score(mode);

    if (points + game.trnluz + 1 >= mxscor && game.trnluz != 0)
        rspeak(TOOK_LONG);
    if (points + game.saved + 1 >= mxscor && game.saved != 0)
        rspeak(WITHOUT_SUSPENDS);
    rspeak(TOTAL_SCORE, points, mxscor, game.turns, game.turns);
    for (long i = 1; i <= (long)NCLASSES; i++) {
        if (classes[i].threshold >= points) {
            speak(classes[i].message);
            i = classes[i].threshold + 1 - points;
            rspeak(NEXT_HIGHER, i, i);
            exit(0);
        }
    }
    rspeak(OFF_SCALE);
    rspeak(NO_HIGHER);
    exit(0);
}

/* end */
