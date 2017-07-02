#include <stdlib.h>
#include <stdbool.h>
#include "advent.h"
#include "dungeon.h"

static int fill(token_t, token_t);

static void state_change(long obj, long state)
{
    game.prop[obj] = state;
    pspeak(obj, change, state, true);
}

static int attack(struct command_t *command)
/*  Attack.  Assume target if unambiguous.  "Throw" also links here.
 *  Attackable objects fall into two categories: enemies (snake,
 *  dwarf, etc.)  and others (bird, clam, machine).  Ambiguous if 2
 *  enemies, or no enemies but 2 others. */
{
    vocab_t verb = command->verb;
    vocab_t obj = command->obj;

    if (obj == INTRANSITIVE) {
        return GO_UNKNOWN;
    }
    long spk = actions[verb].message;
    if (obj == NO_OBJECT || obj == INTRANSITIVE) {
        int changes = 0;
        if (atdwrf(game.loc) > 0) {
            obj = DWARF;
            ++changes;
        }
        if (HERE(SNAKE)) {
            obj = SNAKE;
            ++changes;
        }
        if (AT(DRAGON) && game.prop[DRAGON] == DRAGON_BARS) {
            obj = DRAGON;
            ++changes;
        }
        if (AT(TROLL)) {
            obj = TROLL;
            ++changes;
        }
        if (AT(OGRE)) {
            obj = OGRE;
            ++changes;
        }
        if (HERE(BEAR) && game.prop[BEAR] == UNTAMED_BEAR) {
            obj = BEAR;
            ++changes;
        }
        /* check for low-priority targets */
        if (obj == NO_OBJECT) {
            /* Can't attack bird or machine by throwing axe. */
            if (HERE(BIRD) && verb != THROW) {
                obj = BIRD;
                ++changes;
            }
            if (HERE(VEND) && verb != THROW) {
                obj = VEND;
                ++changes;
            }
            /* Clam and oyster both treated as clam for intransitive case;
             * no harm done. */
            if (HERE(CLAM) || HERE(OYSTER)) {
                obj = CLAM;
                ++changes;
            }
        }
        if (changes >= 2)
            return GO_UNKNOWN;

    }
    if (obj == BIRD) {
        if (game.closed) {
            rspeak(UNHAPPY_BIRD);
            return GO_CLEAROBJ;
        }
        DESTROY(BIRD);
        spk = BIRD_DEAD;
    } else if (obj == VEND) {
        state_change(VEND,
                     game.prop[VEND] == VEND_BLOCKS ? VEND_UNBLOCKS : VEND_BLOCKS);
        return GO_CLEAROBJ;
    }

    if (obj == NO_OBJECT)
        spk = NO_TARGET;
    if (obj == CLAM || obj == OYSTER)
        spk = SHELL_IMPERVIOUS;
    if (obj == SNAKE)
        spk = SNAKE_WARNING;
    if (obj == DWARF)
        spk = BARE_HANDS_QUERY;
    if (obj == DWARF && game.closed)
        return GO_DWARFWAKE;
    if (obj == DRAGON)
        spk = ALREADY_DEAD;
    if (obj == TROLL)
        spk = ROCKY_TROLL;
    if (obj == OGRE)
        spk = OGRE_DODGE;
    if (obj == OGRE && atdwrf(game.loc) > 0) {
        rspeak(spk);
        rspeak(KNIFE_THROWN);
        DESTROY(OGRE);
        int dwarves = 0;
        for (int i = 1; i < PIRATE; i++) {
            if (game.dloc[i] == game.loc) {
                ++dwarves;
                game.dloc[i] = LOC_LONGWEST;
                game.dseen[i] = false;
            }
        }
        spk = (dwarves > 1) ? OGRE_PANIC1 : OGRE_PANIC2;
    } else if (obj == BEAR) {
        switch (game.prop[BEAR]) {
        case UNTAMED_BEAR:
            spk = BEAR_HANDS;
            break;
        case SITTING_BEAR:
            spk = BEAR_CONFUSED;
            break;
        case CONTENTED_BEAR:
            spk = BEAR_CONFUSED;
            break;
        case BEAR_DEAD:
            spk = ALREADY_DEAD;
            break;
        }
    } else if (obj == DRAGON && game.prop[DRAGON] == DRAGON_BARS) {
        /*  Fun stuff for dragon.  If he insists on attacking it, win!
         *  Set game.prop to dead, move dragon to central loc (still
         *  fixed), move rug there (not fixed), and move him there,
         *  too.  Then do a null motion to get new description. */
        rspeak(BARE_HANDS_QUERY);
        if (silent_yes()) {
            // FIXME: setting wd1 is a workaround for broken logic
            command->wd1 = token_to_packed("Y");
        } else {
            // FIXME: setting wd1 is a workaround for broken logic
            command->wd1 = token_to_packed("N");
            return GO_CHECKFOO;
        }
        state_change(DRAGON, DRAGON_DEAD);
        game.prop[RUG] = RUG_FLOOR;
        /* FIXME: Arithmetic on location values */
        int k = (objects[DRAGON].plac + objects[DRAGON].fixd) / 2;
        move(DRAGON + NOBJECTS, -1);
        move(RUG + NOBJECTS, 0);
        move(DRAGON, k);
        move(RUG, k);
        drop(BLOOD, k);
        for (obj = 1; obj <= NOBJECTS; obj++) {
            if (game.place[obj] == objects[DRAGON].plac || game.place[obj] == objects[DRAGON].fixd)
                move(obj, k);
        }
        game.loc = k;
        return GO_MOVE;
    }

    rspeak(spk);
    return GO_CLEAROBJ;
}

static int bigwords(token_t foo)
/*  FEE FIE FOE FOO (AND FUM).  Advance to next state if given in proper order.
 *  Look up foo in special section of vocab to determine which word we've got.
 *  Last word zips the eggs back to the giant room (unless already there). */
{
    char word[6];
    packed_to_token(foo, word);
    int k = (int) get_special_vocab_id(word);
    int spk = NOTHING_HAPPENS;
    if (game.foobar != 1 - k) {
        if (game.foobar != 0 && game.loc == LOC_GIANTROOM)
            spk = START_OVER;
        rspeak(spk);
        return GO_CLEAROBJ;
    } else {
        game.foobar = k;
        if (k != 4) {
            rspeak(OK_MAN);
            return GO_CLEAROBJ;
        }
        game.foobar = 0;
        if (game.place[EGGS] == objects[EGGS].plac || (TOTING(EGGS) && game.loc == objects[EGGS].plac)) {
            rspeak(spk);
            return GO_CLEAROBJ;
        } else {
            /*  Bring back troll if we steal the eggs back from him before
             *  crossing. */
            if (game.place[EGGS] == LOC_NOWHERE && game.place[TROLL] == LOC_NOWHERE && game.prop[TROLL] == TROLL_UNPAID)
                game.prop[TROLL] = TROLL_PAIDONCE;
            k = EGGS_DONE;
            if (HERE(EGGS))
                k = EGGS_VANISHED;
            if (game.loc == objects[EGGS].plac)
                k = EGGS_HERE;
            move(EGGS, objects[EGGS].plac);
            pspeak(EGGS, look, k, true);
            return GO_CLEAROBJ;
        }
    }
}

static int bivalve(token_t verb, token_t obj)
/* Clam/oyster actions */
{
    int spk;
    bool is_oyster = (obj == OYSTER);
    spk = is_oyster ? OYSTER_OPENS : PEARL_FALLS;
    if (TOTING(obj))
        spk = is_oyster ? DROP_OYSTER : DROP_CLAM;
    if (!TOTING(TRIDENT))
        spk = is_oyster ? OYSTER_OPENER : CLAM_OPENER;
    if (verb == LOCK)
        spk = HUH_MAN;
    if (spk == PEARL_FALLS) {
        DESTROY(CLAM);
        drop(OYSTER, game.loc);
        drop(PEARL, LOC_CULDESAC);
    }
    rspeak(spk);
    return GO_CLEAROBJ;
}

static void blast(void)
/*  Blast.  No effect unless you've got dynamite, which is a neat trick! */
{
    if (game.prop[ROD2] < 0 || !game.closed)
        rspeak(REQUIRES_DYNAMITE);
    else {
        game.bonus = VICTORY_MESSAGE;
        if (game.loc == LOC_NE)
            game.bonus = DEFEAT_MESSAGE;
        if (HERE(ROD2))
            game.bonus = SPLATTER_MESSAGE;
        rspeak(game.bonus);
        terminate(endgame);
    }
}

static int vbreak(token_t verb, token_t obj)
/*  Break.  Only works for mirror in repository and, of course, the vase. */
{
    if (obj == MIRROR) {
        if (game.closed) {
            rspeak(BREAK_MIRROR);
            return GO_DWARFWAKE;
        } else {
            rspeak(TOO_FAR);
            return GO_CLEAROBJ;
        }
    }
    if (obj == VASE && game.prop[VASE] == VASE_WHOLE) {
        if (TOTING(VASE))
            drop(VASE, game.loc);
        state_change(VASE, VASE_BROKEN);
        game.fixed[VASE] = -1;
        return GO_CLEAROBJ;
    }
    rspeak(actions[verb].message);
    return (GO_CLEAROBJ);
}

static int brief(void)
/*  Brief.  Intransitive only.  Suppress long descriptions after first time. */
{
    game.abbnum = 10000;
    game.detail = 3;
    rspeak(BRIEF_CONFIRM);
    return GO_CLEAROBJ;
}

static int vcarry(token_t verb, token_t obj)
/*  Carry an object.  Special cases for bird and cage (if bird in cage, can't
 *  take one without the other).  Liquids also special, since they depend on
 *  status of bottle.  Also various side effects, etc. */
{
    int spk;
    if (obj == INTRANSITIVE) {
        /*  Carry, no object given yet.  OK if only one object present. */
        if (game.atloc[game.loc] == 0 ||
            game.link[game.atloc[game.loc]] != 0 ||
            atdwrf(game.loc) > 0)
            return GO_UNKNOWN;
        obj = game.atloc[game.loc];
    }

    if (TOTING(obj)) {
        rspeak(ALREADY_CARRYING);
        return GO_CLEAROBJ;
    }
    spk = YOU_JOKING;
    if (obj == PLANT && game.prop[PLANT] <= 0)
        spk = DEEP_ROOTS;
    if (obj == BEAR && game.prop[BEAR] == SITTING_BEAR)
        spk = BEAR_CHAINED;
    if (obj == CHAIN && game.prop[BEAR] != UNTAMED_BEAR)
        spk = STILL_LOCKED;
    if (obj == URN)
        spk = URN_NOBUDGE;
    if (obj == CAVITY)
        spk = DOUGHNUT_HOLES;
    if (obj == BLOOD)
        spk = FEW_DROPS;
    if (obj == RUG && game.prop[RUG] == RUG_HOVER)
        spk = RUG_HOVERS;
    if (obj == SIGN)
        spk = HAND_PASSTHROUGH;
    if (obj == MESSAG) {
        rspeak(REMOVE_MESSAGE);
        DESTROY(MESSAG);
        return GO_CLEAROBJ;
    }
    if (game.fixed[obj] != 0) {
        rspeak(spk);
        return GO_CLEAROBJ;
    }
    if (obj == WATER || obj == OIL) {
        if (!HERE(BOTTLE) || LIQUID() != obj) {
            if (TOTING(BOTTLE) && game.prop[BOTTLE] == EMPTY_BOTTLE)
                return (fill(verb, BOTTLE));
            else {
                if (game.prop[BOTTLE] != EMPTY_BOTTLE)
                    spk = BOTTLE_FULL;
                if (!TOTING(BOTTLE))
                    spk = NO_CONTAINER;
                rspeak(spk);
                return GO_CLEAROBJ;
            }
        }
        obj = BOTTLE;
    }

    spk = CARRY_LIMIT;
    if (game.holdng >= INVLIMIT) {
        rspeak(spk);
        return GO_CLEAROBJ;
    } else if (obj == BIRD && game.prop[BIRD] != BIRD_CAGED && -1 - game.prop[BIRD] != BIRD_CAGED) {
        if (game.prop[BIRD] == BIRD_FOREST_UNCAGED) {
            DESTROY(BIRD);
            rspeak(BIRD_CRAP);
            return GO_CLEAROBJ;
        }
        if (!TOTING(CAGE))
            spk = CANNOT_CARRY;
        if (TOTING(ROD))
            spk = BIRD_EVADES;
        if (spk == CANNOT_CARRY || spk == BIRD_EVADES) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        game.prop[BIRD] = BIRD_CAGED;
    }
    if ((obj == BIRD || obj == CAGE) && (game.prop[BIRD] == BIRD_CAGED || -1 - game.prop[BIRD] == 1))
        carry(BIRD + CAGE - obj, game.loc);
    carry(obj, game.loc);
    if (obj == BOTTLE && LIQUID() != 0)
        game.place[LIQUID()] = CARRIED;
    if (GSTONE(obj) && game.prop[obj] != 0) {
        game.prop[obj] = STATE_GROUND;
        game.prop[CAVITY] = CAVITY_EMPTY;
    }
    rspeak(OK_MAN);
    return GO_CLEAROBJ;
}

static int chain(token_t verb)
/* Do something to the bear's chain */
{
    int spk;
    if (verb != LOCK) {
        spk = CHAIN_UNLOCKED;
        if (game.prop[BEAR] == UNTAMED_BEAR)
            spk = BEAR_BLOCKS;
        if (game.prop[CHAIN] == CHAIN_HEAP)
            spk = ALREADY_UNLOCKED;
        if (spk != CHAIN_UNLOCKED) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        game.prop[CHAIN] = CHAIN_HEAP;
        game.fixed[CHAIN] = CHAIN_HEAP;
        if (game.prop[BEAR] != BEAR_DEAD)
            game.prop[BEAR] = CONTENTED_BEAR;
        /* FIXME: Arithmetic on state numbers */
        game.fixed[BEAR] = 2 - game.prop[BEAR];
    } else {
        spk = CHAIN_LOCKED;
        if (game.prop[CHAIN] != CHAIN_HEAP)
            spk = ALREADY_LOCKED;
        if (game.loc != objects[CHAIN].plac)
            spk = NO_LOCKSITE;
        if (spk != CHAIN_LOCKED) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        game.prop[CHAIN] = CHAIN_FIXED;
        if (TOTING(CHAIN))
            drop(CHAIN, game.loc);
        game.fixed[CHAIN] = -1;
    }
    rspeak(spk);
    return GO_CLEAROBJ;
}

static int discard(token_t verb, token_t obj, bool just_do_it)
/*  Discard object.  "Throw" also comes here for most objects.  Special cases for
 *  bird (might attack snake or dragon) and cage (might contain bird) and vase.
 *  Drop coins at vending machine for extra batteries. */
{
    int spk = actions[verb].message;
    if (!just_do_it) {
        if (TOTING(ROD2) && obj == ROD && !TOTING(ROD))
            obj = ROD2;
        if (!TOTING(obj)) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        if (obj == BIRD && HERE(SNAKE)) {
            rspeak(BIRD_ATTACKS);
            if (game.closed)
                return GO_DWARFWAKE;
            DESTROY(SNAKE);
            /* Set game.prop for use by travel options */
            game.prop[SNAKE] = SNAKE_CHASED;

        } else if ((GSTONE(obj) && AT(CAVITY) && game.prop[CAVITY] != CAVITY_FULL)) {
            rspeak(GEM_FITS);
            game.prop[obj] = 1;
            game.prop[CAVITY] = CAVITY_FULL;
            if (HERE(RUG) && ((obj == EMERALD && game.prop[RUG] != RUG_HOVER) || (obj == RUBY &&
                              game.prop[RUG] == RUG_HOVER))) {
                spk = RUG_RISES;
                if (TOTING(RUG))
                    spk = RUG_WIGGLES;
                if (obj == RUBY)
                    spk = RUG_SETTLES;
                rspeak(spk);
                if (spk != RUG_WIGGLES) {
                    /* FIXME: Arithmetic on state numbers */
                    int k = 2 - game.prop[RUG];
                    game.prop[RUG] = k;
                    if (k == 2)
                        k = objects[SAPPH].plac;
                    move(RUG + NOBJECTS, k);
                }
            }
        } else if (obj == COINS && HERE(VEND)) {
            DESTROY(COINS);
            drop(BATTERY, game.loc);
            pspeak(BATTERY, look, FRESH_BATTERIES, true);
            return GO_CLEAROBJ;
        } else if (obj == BIRD && AT(DRAGON) && game.prop[DRAGON] == DRAGON_BARS) {
            rspeak(BIRD_BURNT);
            DESTROY(BIRD);
            return GO_CLEAROBJ;
        } else if (obj == BEAR && AT(TROLL)) {
            rspeak(TROLL_SCAMPERS);
            move(TROLL, LOC_NOWHERE);
            move(TROLL + NOBJECTS, LOC_NOWHERE);
            move(TROLL2, objects[TROLL].plac);
            move(TROLL2 + NOBJECTS, objects[TROLL].fixd);
            juggle(CHASM);
            game.prop[TROLL] = TROLL_GONE;
        } else if (obj != VASE || game.loc == objects[PILLOW].plac) {
            rspeak(OK_MAN);
        } else {
            game.prop[VASE] = VASE_BROKEN;
            if (AT(PILLOW))
                game.prop[VASE] = VASE_WHOLE;
            pspeak(VASE, look, game.prop[VASE] + 1, true);
            if (game.prop[VASE] != VASE_WHOLE)
                game.fixed[VASE] = -1;
        }
    }
    int k = LIQUID();
    if (k == obj)
        obj = BOTTLE;
    if (obj == BOTTLE && k != 0)
        game.place[k] = LOC_NOWHERE;
    if (obj == CAGE && game.prop[BIRD] == BIRD_CAGED)
        drop(BIRD, game.loc);
    drop(obj, game.loc);
    if (obj != BIRD)
        return GO_CLEAROBJ;
    game.prop[BIRD] = BIRD_UNCAGED;
    if (FOREST(game.loc))
        game.prop[BIRD] = BIRD_FOREST_UNCAGED;
    return GO_CLEAROBJ;
}

static int drink(token_t verb, token_t obj)
/*  Drink.  If no object, assume water and look for it here.  If water is in
 *  the bottle, drink that, else must be at a water loc, so drink stream. */
{
    if (obj == NO_OBJECT && LIQLOC(game.loc) != WATER && (LIQUID() != WATER || !HERE(BOTTLE)))
        return GO_UNKNOWN;
    if (obj != BLOOD) {
        if (obj != NO_OBJECT && obj != WATER) {
            rspeak(RIDICULOUS_ATTEMPT);
        } else if (LIQUID() == WATER && HERE(BOTTLE)) {
            game.prop[BOTTLE] = EMPTY_BOTTLE;
            game.place[WATER] = LOC_NOWHERE;
            rspeak(BOTTLE_EMPTY);
        } else {
            rspeak(actions[verb].message);
        }
    } else {
        DESTROY(BLOOD);
        state_change(DRAGON, DRAGON_BLOODLESS);
        game.blooded = true;
    }
    return GO_CLEAROBJ;
}

static int eat(token_t verb, token_t obj)
/*  Eat.  Intransitive: assume food if present, else ask what.  Transitive: food
 *  ok, some things lose appetite, rest are ridiculous. */
{
    int spk = actions[verb].message;
    if (obj == INTRANSITIVE) {
        if (!HERE(FOOD))
            return GO_UNKNOWN;
        DESTROY(FOOD);
        spk = THANKS_DELICIOUS;
    } else {
        if (obj == FOOD) {
            DESTROY(FOOD);
            spk = THANKS_DELICIOUS;
        }
        if (obj == BIRD || obj == SNAKE || obj == CLAM || obj == OYSTER || obj ==
            DWARF || obj == DRAGON || obj == TROLL || obj == BEAR || obj ==
            OGRE)
            spk = LOST_APPETITE;
    }
    rspeak(spk);
    return GO_CLEAROBJ;
}

static int extinguish(token_t verb, int obj)
/* Extinguish.  Lamp, urn, dragon/volcano (nice try). */
{
    if (obj == INTRANSITIVE) {
        if (HERE(LAMP) && game.prop[LAMP] == LAMP_BRIGHT)
            obj = LAMP;
        if (HERE(URN) && game.prop[URN] == URN_LIT)
            obj = URN;
        if (obj == INTRANSITIVE ||
            (HERE(LAMP) && game.prop[LAMP] == LAMP_BRIGHT &&
             HERE(URN) && game.prop[URN] == URN_LIT))
            return GO_UNKNOWN;
    }

    if (obj == URN) {
        if (game.prop[URN] != URN_EMPTY) {
            state_change(URN, URN_DARK);
        } else {
            pspeak(URN, change, URN_DARK, true);
        }

    } else if (obj == LAMP) {
        state_change(LAMP, LAMP_DARK);
        rspeak(DARK(game.loc) ? PITCH_DARK : NO_MESSAGE);
    } else if (obj == DRAGON || obj == VOLCANO) {
        rspeak(BEYOND_POWER);

    } else {
        rspeak(actions[verb].message);
    }
    return GO_CLEAROBJ;
}

static int feed(token_t verb, token_t obj)
/*  Feed.  If bird, no seed.  Snake, dragon, troll: quip.  If dwarf, make him
 *  mad.  Bear, special. */
{
    int spk = actions[verb].message;
    if (obj == BIRD) {
        rspeak(BIRD_PINING);
        return GO_CLEAROBJ;
    } else if (obj == SNAKE || obj == DRAGON || obj == TROLL) {
        spk = NOTHING_EDIBLE;
        if (obj == DRAGON && game.prop[DRAGON] != DRAGON_BARS)
            spk = RIDICULOUS_ATTEMPT;
        if (obj == TROLL)
            spk = TROLL_VICES;
        if (obj == SNAKE && !game.closed && HERE(BIRD)) {
            DESTROY(BIRD);
            spk = BIRD_DEVOURED;
        }
    } else if (obj == DWARF) {
        if (HERE(FOOD)) {
            game.dflag += 2;
            spk = REALLY_MAD;
        }
    } else if (obj == BEAR) {
        if (game.prop[BEAR] == UNTAMED_BEAR)
            spk = NOTHING_EDIBLE;
        if (game.prop[BEAR] == BEAR_DEAD)
            spk = RIDICULOUS_ATTEMPT;
        if (HERE(FOOD)) {
            DESTROY(FOOD);
            game.prop[BEAR] = SITTING_BEAR;
            game.fixed[AXE] = 0;
            game.prop[AXE] = 0;
            spk = BEAR_TAMED;
        }
    } else if (obj == OGRE) {
        if (HERE(FOOD))
            spk = OGRE_FULL;
    } else {
        spk = AM_GAME;
    }
    rspeak(spk);
    return GO_CLEAROBJ;
}

int fill(token_t verb, token_t obj)
/*  Fill.  Bottle or urn must be empty, and liquid available.  (Vase
 *  is nasty.) */
{
    int k;
    int spk = actions[verb].message;
    if (obj == VASE) {
        spk = ARENT_CARRYING;
        if (LIQLOC(game.loc) == 0)
            spk = FILL_INVALID;
        if (LIQLOC(game.loc) == 0 || !TOTING(VASE)) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        rspeak(SHATTER_VASE);
        game.prop[VASE] = VASE_BROKEN;
        game.fixed[VASE] = -1;
        return (discard(verb, obj, true));
    } else if (obj == URN) {
        spk = FULL_URN;
        if (game.prop[URN] != URN_EMPTY) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        spk = FILL_INVALID;
        k = LIQUID();
        if (k == 0 || !HERE(BOTTLE)) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        game.place[k] = LOC_NOWHERE;
        game.prop[BOTTLE] = EMPTY_BOTTLE;
        if (k == OIL)
            game.prop[URN] = URN_DARK;
        spk = WATER_URN + game.prop[URN];
        rspeak(spk);
        return GO_CLEAROBJ;
    } else if (obj != NO_OBJECT && obj != BOTTLE) {
        rspeak(spk);
        return GO_CLEAROBJ;
    } else if (obj == NO_OBJECT && !HERE(BOTTLE))
        return GO_UNKNOWN;
    spk = BOTTLED_WATER;
    if (LIQLOC(game.loc) == 0)
        spk = NO_LIQUID;
    if (HERE(URN) && game.prop[URN] != URN_EMPTY)
        spk = URN_NOPOUR;
    if (LIQUID() != 0)
        spk = BOTTLE_FULL;
    if (spk == BOTTLED_WATER) {
        /* FIXME: Arithmetic on property values */
        game.prop[BOTTLE] = MOD(conditions[game.loc], 4) / 2 * 2;
        k = LIQUID();
        if (TOTING(BOTTLE))
            game.place[k] = CARRIED;
        if (k == OIL)
            spk = BOTTLED_OIL;
    }
    rspeak(spk);
    return GO_CLEAROBJ;
}

static int find(token_t verb, token_t obj)
/* Find.  Might be carrying it, or it might be here.  Else give caveat. */
{
    int spk = actions[verb].message;
    if (AT(obj) ||
        (LIQUID() == obj && AT(BOTTLE)) ||
        obj == LIQLOC(game.loc) ||
        (obj == DWARF && atdwrf(game.loc) > 0))
        spk = YOU_HAVEIT;
    if (game.closed)
        spk = NEEDED_NEARBY;
    if (TOTING(obj))
        spk = ALREADY_CARRYING;
    rspeak(spk);
    return GO_CLEAROBJ;
}

static int fly(token_t verb, token_t obj)
/* Fly.  Snide remarks unless hovering rug is here. */
{
    int spk = actions[verb].message;
    if (obj == INTRANSITIVE) {
        if (game.prop[RUG] != RUG_HOVER)
            spk = RUG_NOTHING2;
        if (!HERE(RUG))
            spk = FLAP_ARMS;
        if (spk == RUG_NOTHING2 || spk == FLAP_ARMS) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        obj = RUG;
    }

    if (obj != RUG) {
        rspeak(spk);
        return GO_CLEAROBJ;
    }
    spk = RUG_NOTHING1;
    if (game.prop[RUG] != RUG_HOVER) {
        rspeak(spk);
        return GO_CLEAROBJ;
    }
    game.oldlc2 = game.oldloc;
    game.oldloc = game.loc;
    /* FIXME: Arithmetic on location values */
    game.newloc = game.place[RUG] + game.fixed[RUG] - game.loc;
    spk = RUG_GOES;
    if (game.prop[SAPPH] >= 0)
        spk = RUG_RETURNS;
    rspeak(spk);
    return GO_TERMINATE;
}

static int inven(void)
/* Inventory. If object, treat same as find.  Else report on current burden. */
{
    int spk = NO_CARRY;
    for (int i = 1; i <= NOBJECTS; i++) {
        if (i == BEAR || !TOTING(i))
            continue;
        if (spk == NO_CARRY)
            rspeak(NOW_HOLDING);
        pspeak(i, touch, -1, false);
        spk = NO_MESSAGE;
    }
    if (TOTING(BEAR))
        spk = TAME_BEAR;
    rspeak(spk);
    return GO_CLEAROBJ;
}

static int light(token_t verb, token_t obj)
/*  Light.  Applicable only to lamp and urn. */
{
    if (obj == INTRANSITIVE) {
        if (HERE(LAMP) && game.prop[LAMP] == LAMP_DARK && game.limit >= 0)
            obj = LAMP;
        if (HERE(URN) && game.prop[URN] == URN_DARK)
            obj =  URN;
        if (obj == INTRANSITIVE ||
            (HERE(LAMP) && game.prop[LAMP] == LAMP_DARK && game.limit >= 0 &&
             HERE(URN) && game.prop[URN] == URN_DARK))
            return GO_UNKNOWN;
    }

    if (obj == URN) {
        state_change(URN, game.prop[URN] == URN_EMPTY ? URN_EMPTY : URN_LIT);
        return GO_CLEAROBJ;
    } else {
        if (obj != LAMP) {
            rspeak(actions[verb].message);
            return GO_CLEAROBJ;
        }
        if (game.limit < 0) {
            rspeak(LAMP_OUT);
            return GO_CLEAROBJ;
        }
        state_change(LAMP, LAMP_BRIGHT);
        if (game.wzdark)
            return GO_TOP;
        else
            return GO_CLEAROBJ;
    }
}

static int listen(void)
/*  Listen.  Intransitive only.  Print stuff based on objsnd/locsnd. */
{
    long k;
    int spk = ALL_SILENT;
    k = locations[game.loc].sound;
    if (k != SILENT) {
        rspeak(k);
        if (locations[game.loc].loud)
            return GO_CLEAROBJ;
        else
            spk = NO_MESSAGE;
    }
    for (int i = 1; i <= NOBJECTS; i++) {
        if (!HERE(i) || objects[i].sounds[0] == NULL || game.prop[i] < 0)
            continue;
        int mi =  game.prop[i];
        if (i == BIRD)
            mi += 3 * game.blooded;
        long packed_zzword = token_to_packed(game.zzword);
        pspeak(i, hear, mi, true, packed_zzword);
        spk = NO_MESSAGE;
        /* FIXME: Magic number, sensitive to bird state logic */
        if (i == BIRD && game.prop[i] == 5)
            DESTROY(BIRD);
    }
    rspeak(spk);
    return GO_CLEAROBJ;
}

static int lock(token_t verb, token_t obj)
/* Lock, unlock, no object given.  Assume various things if present. */
{
    int spk = actions[verb].message;
    if (obj == INTRANSITIVE) {
        spk = NOTHING_LOCKED;
        if (HERE(CLAM))
            obj = CLAM;
        if (HERE(OYSTER))
            obj = OYSTER;
        if (AT(DOOR))
            obj = DOOR;
        if (AT(GRATE))
            obj = GRATE;
        if (HERE(CHAIN))
            obj = CHAIN;
        if (obj == NO_OBJECT || obj == INTRANSITIVE) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
    }

    /*  Lock, unlock object.  Special stuff for opening clam/oyster
     *  and for chain. */
    if (obj == CLAM || obj == OYSTER)
        return bivalve(verb, obj);
    if (obj == DOOR)
        spk = RUSTY_DOOR;
    if (obj == DOOR && game.prop[DOOR] == DOOR_UNRUSTED)
        spk = OK_MAN;
    if (obj == CAGE)
        spk = NO_LOCK;
    if (obj == KEYS)
        spk = CANNOT_UNLOCK;
    if (obj == GRATE || obj == CHAIN) {
        spk = NO_KEYS;
        if (HERE(KEYS)) {
            if (obj == CHAIN)
                return chain(verb);
            if (game.closng) {
                spk = EXIT_CLOSED;
                if (!game.panic)
                    game.clock2 = PANICTIME;
                game.panic = true;
            } else {
                state_change(GRATE, (verb == LOCK) ? GRATE_CLOSED : GRATE_OPEN);
                return GO_CLEAROBJ;
            }
        }
    }
    rspeak(spk);
    return GO_CLEAROBJ;
}

static int pour(token_t verb, token_t obj)
/*  Pour.  If no object, or object is bottle, assume contents of bottle.
 *  special tests for pouring water or oil on plant or rusty door. */
{
    int spk = actions[verb].message;
    if (obj == BOTTLE || obj == NO_OBJECT)
        obj = LIQUID();
    if (obj == NO_OBJECT)
        return GO_UNKNOWN;
    if (!TOTING(obj)) {
        rspeak(spk);
        return GO_CLEAROBJ;
    }
    spk = CANT_POUR;
    if (obj != OIL && obj != WATER) {
        rspeak(spk);
        return GO_CLEAROBJ;
    }
    if (HERE(URN) && game.prop[URN] == URN_EMPTY)
        return fill(verb, URN);
    game.prop[BOTTLE] = EMPTY_BOTTLE;
    game.place[obj] = LOC_NOWHERE;
    spk = GROUND_WET;
    if (!(AT(PLANT) || AT(DOOR))) {
        rspeak(spk);
        return GO_CLEAROBJ;
    }
    if (!AT(DOOR)) {
        spk = SHAKING_LEAVES;
        if (obj != WATER) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        pspeak(PLANT, look, game.prop[PLANT] + 3, true);
        game.prop[PLANT] = MOD(game.prop[PLANT] + 1, 3);
        game.prop[PLANT2] = game.prop[PLANT];
        return GO_MOVE;
    } else {
        state_change(DOOR, (obj == OIL) ? DOOR_UNRUSTED : DOOR_RUSTED);
        return GO_CLEAROBJ;
    }
}

static int quit(void)
/*  Quit.  Intransitive only.  Verify intent and exit if that's what he wants. */
{
    if (yes(arbitrary_messages[REALLY_QUIT], arbitrary_messages[OK_MAN], arbitrary_messages[OK_MAN]))
        terminate(quitgame);
    return GO_CLEAROBJ;
}

static int read(struct command_t command)
/*  Read.  Print stuff based on objtxt.  Oyster (?) is special case. */
{
    if (command.obj == INTRANSITIVE) {
        command.obj = 0;
        for (int i = 1; i <= NOBJECTS; i++) {
            if (HERE(i) && objects[i].texts[0] != NULL && game.prop[i] >= 0)
                command.obj = command.obj * NOBJECTS + i;
        }
        if (command.obj > NOBJECTS || command.obj == 0 || DARK(game.loc))
            return GO_UNKNOWN;
    }

    if (DARK(game.loc)) {
        rspeak(NO_SEE, command.wd1, command.wd1x);
    } else if (command.obj == OYSTER && !game.clshnt && game.closed) {
        game.clshnt = yes(arbitrary_messages[CLUE_QUERY], arbitrary_messages[WAYOUT_CLUE], arbitrary_messages[OK_MAN]);
    } else if (objects[command.obj].texts[0] == NULL || game.prop[command.obj] < 0) {
        rspeak(actions[command.verb].message);
    } else
        pspeak(command.obj, study, game.prop[command.obj], true);
    return GO_CLEAROBJ;
}

static int reservoir(void)
/*  Z'ZZZ (word gets recomputed at startup; different each game). */
{
    if (!AT(RESER) && game.loc != game.fixed[RESER] - 1) {
        rspeak(NOTHING_HAPPENS);
        return GO_CLEAROBJ;
    } else {
        pspeak(RESER, look, game.prop[RESER] + 1, true);
        game.prop[RESER] = 1 - game.prop[RESER];
        if (AT(RESER))
            return GO_CLEAROBJ;
        else {
            game.oldlc2 = game.loc;
            game.newloc = 0;
            rspeak(NOT_BRIGHT);
            return GO_TERMINATE;
        }
    }
}

static int rub(token_t verb, token_t obj)
/* Rub.  Yields various snide remarks except for lit urn. */
{
    if (obj == URN && game.prop[URN] == URN_LIT) {
        DESTROY(URN);
        drop(AMBER, game.loc);
        game.prop[AMBER] = AMBER_IN_ROCK;
        --game.tally;
        drop(CAVITY, game.loc);
        rspeak(URN_GENIES);
    } else if (obj != LAMP) {
        rspeak(PECULIAR_NOTHING);
    } else {
        rspeak(actions[verb].message);
    }
    return GO_CLEAROBJ;
}

static int say(struct command_t *command)
/* Say.  Echo WD2 (or WD1 if no WD2 (SAY WHAT?, etc.).)  Magic words override. */
{
    long a = command->wd1, b = command->wd1x;
    if (command->wd2 > 0) {
        a = command->wd2;
        b = command->wd2x;
        command->wd1 = command->wd2;
    }
    char word1[6];
    packed_to_token(command->wd1, word1);
    int wd = (int) get_vocab_id(word1);
    /* FIXME: magic numbers */
    if (wd == MOTION_WORD(XYZZY) || wd == MOTION_WORD(PLUGH) || wd == MOTION_WORD(PLOVER) || wd == ACTION_WORD(GIANTWORDS) || wd == ACTION_WORD(PART)) {
        /* FIXME: scribbles on the interpreter's command block */
        wordclear(&command->wd2);
        return GO_LOOKUP;
    }
    rspeak(OKEY_DOKEY, a, b);
    return GO_CLEAROBJ;
}

static int throw_support(long spk)
{
    rspeak(spk);
    drop(AXE, game.loc);
    return GO_MOVE;
}

static int throw (struct command_t *command)
/*  Throw.  Same as discard unless axe.  Then same as attack except
 *  ignore bird, and if dwarf is present then one might be killed.
 *  (Only way to do so!)  Axe also special for dragon, bear, and
 *  troll.  Treasures special for troll. */
{
    if (TOTING(ROD2) && command->obj == ROD && !TOTING(ROD))
        command->obj = ROD2;
    if (!TOTING(command->obj)) {
        rspeak(actions[command->verb].message);
        return GO_CLEAROBJ;
    }
    if (objects[command->obj].is_treasure && AT(TROLL)) {
        /*  Snarf a treasure for the troll. */
        drop(command->obj, LOC_NOWHERE);
        move(TROLL, LOC_NOWHERE);
        move(TROLL + NOBJECTS, LOC_NOWHERE);
        drop(TROLL2, objects[TROLL].plac);
        drop(TROLL2 + NOBJECTS, objects[TROLL].fixd);
        juggle(CHASM);
        rspeak(TROLL_SATISFIED);
        return GO_CLEAROBJ;
    }
    if (command->obj == FOOD && HERE(BEAR)) {
        /* But throwing food is another story. */
        command->obj = BEAR;
        return (feed(command->verb, command->obj));
    }
    if (command->obj != AXE)
        return (discard(command->verb, command->obj, false));
    else {
        if (atdwrf(game.loc) <= 0) {
            if (AT(DRAGON) && game.prop[DRAGON] == DRAGON_BARS)
                return throw_support(DRAGON_SCALES);
            if (AT(TROLL))
                return throw_support(TROLL_RETURNS);
            else if (AT(OGRE))
                return throw_support(OGRE_DODGE);
            else if (HERE(BEAR) && game.prop[BEAR] == UNTAMED_BEAR) {
                /* This'll teach him to throw the axe at the bear! */
                drop(AXE, game.loc);
                game.fixed[AXE] = -1;
                juggle(BEAR);
                state_change(AXE, AXE_LOST);
                return GO_CLEAROBJ;
            }
            command->obj = NO_OBJECT;
            return (attack(command));
        }

        if (randrange(NDWARVES + 1) < game.dflag) {
            return throw_support(DWARF_DODGES);
        } else {
            long i = atdwrf(game.loc);
            game.dseen[i] = false;
            game.dloc[i] = 0;
            return throw_support((++game.dkill == 1)
                                 ? DWARF_SMOKE : KILLED_DWARF);
        }
    }
}

static int wake(token_t verb, token_t obj)
/* Wake.  Only use is to disturb the dwarves. */
{
    if (obj != DWARF || !game.closed) {
        rspeak(actions[verb].message);
        return GO_CLEAROBJ;
    } else {
        rspeak(PROD_DWARF);
        return GO_DWARFWAKE;
    }
}

static token_t birdspeak(void)
{
    switch (game.prop[BIRD]) {
    case BIRD_UNCAGED:
    case BIRD_FOREST_UNCAGED:
        return FREE_FLY;
    case BIRD_CAGED:
        return CAGE_FLY;
    }
}

static int wave(token_t verb, token_t obj)
/* Wave.  No effect unless waving rod at fissure or at bird. */
{
    int spk = actions[verb].message;
    if ((!TOTING(obj)) && (obj != ROD || !TOTING(ROD2)))
        spk = ARENT_CARRYING;
    if (obj != ROD ||
        !TOTING(obj) ||
        (!HERE(BIRD) && (game.closng || !AT(FISSURE)))) {
        rspeak(spk);
        return GO_CLEAROBJ;
    }

    if (HERE(BIRD))
        spk = birdspeak();
    if (spk == FREE_FLY && game.loc == game.place[STEPS] && game.prop[JADE] < 0) {
        drop(JADE, game.loc);
        game.prop[JADE] = 0;
        --game.tally;
        spk = NECKLACE_FLY;
        rspeak(spk);
        return GO_CLEAROBJ;
    } else {
        if (game.closed) {
            rspeak(spk);
            return GO_DWARFWAKE;
        }
        if (game.closng || !AT(FISSURE)) {
            rspeak(spk);
            return GO_CLEAROBJ;
        }
        if (HERE(BIRD))
            rspeak(spk);

        /* FIXME: Arithemetic on property values */
        game.prop[FISSURE] = 1 - game.prop[FISSURE];
        pspeak(FISSURE, look, 2 - game.prop[FISSURE], true);
        return GO_CLEAROBJ;
    }
}



int action(struct command_t *command)
/*  Analyse a verb.  Remember what it was, go back for object if second word
 *  unless verb is "say", which snarfs arbitrary second word.
 */
{
    if (command->part == unknown) {
        /*  Analyse an object word.  See if the thing is here, whether
         *  we've got a verb yet, and so on.  Object must be here
         *  unless verb is "find" or "invent(ory)" (and no new verb
         *  yet to be analysed).  Water and oil are also funny, since
         *  they are never actually dropped at any location, but might
         *  be here inside the bottle or urn or as a feature of the
         *  location. */
        if (HERE(command->obj))
            /* FALL THROUGH */;
        else if (command->obj == GRATE) {
            if (game.loc == LOC_START || game.loc == LOC_VALLEY || game.loc == LOC_SLIT)
                command->obj = DPRSSN;
            if (game.loc == LOC_COBBLE || game.loc == LOC_DEBRIS || game.loc == LOC_AWKWARD ||
                game.loc == LOC_BIRD || game.loc == LOC_PITTOP)
                command->obj = ENTRNC;
        } else if (command->obj == DWARF && atdwrf(game.loc) > 0)
            /* FALL THROUGH */;
        else if ((LIQUID() == command->obj && HERE(BOTTLE)) || command->obj == LIQLOC(game.loc))
            /* FALL THROUGH */;
        else if (command->obj == OIL && HERE(URN) && game.prop[URN] != 0) {
            command->obj = URN;
            /* FALL THROUGH */;
        } else if (command->obj == PLANT && AT(PLANT2) && game.prop[PLANT2] != 0) {
            command->obj = PLANT2;
            /* FALL THROUGH */;
        } else if (command->obj == KNIFE && game.knfloc == game.loc) {
            game.knfloc = -1;
            rspeak(KNIVES_VANISH);
            return GO_CLEAROBJ;
        } else if (command->obj == ROD && HERE(ROD2)) {
            command->obj = ROD2;
            /* FALL THROUGH */;
        } else if ((command->verb == FIND || command->verb == INVENTORY) && command->wd2 <= 0)
            /* FALL THROUGH */;
        else {
            rspeak(NO_SEE, command->wd1, command->wd1x);
            return GO_CLEAROBJ;
        }

        if (command->wd2 > 0)
            return GO_WORD2;
        if (command->verb != 0)
            command->part = transitive;
    }

    switch (command->part) {
    case intransitive:
        if (command->wd2 > 0 && command->verb != SAY)
            return GO_WORD2;
        if (command->verb == SAY)
            command->obj = command->wd2;
        if (command->obj == 0 || command->obj == INTRANSITIVE) {
            /*  Analyse an intransitive verb (ie, no object given yet). */
            switch (command->verb) {
            case CARRY:
                return vcarry(command->verb, INTRANSITIVE);
            case  DROP:
                return GO_UNKNOWN;
            case  SAY:
                return GO_UNKNOWN;
            case  UNLOCK:
                return lock(command->verb, INTRANSITIVE);
            case  NOTHING: {
                rspeak(OK_MAN);
                return (GO_CLEAROBJ);
            }
            case  LOCK:
                return lock(command->verb, INTRANSITIVE);
            case  LIGHT:
                return light(command->verb, INTRANSITIVE);
            case  EXTINGUISH:
                return extinguish(command->verb, INTRANSITIVE);
            case  WAVE:
                return GO_UNKNOWN;
            case  TAME:
                return GO_UNKNOWN;
            case GO: {
                rspeak(actions[command->verb].message);
                return GO_CLEAROBJ;
            }
            case ATTACK:
                return attack(command);
            case POUR:
                return pour(command->verb, command->obj);
            case EAT:
                return eat(command->verb, INTRANSITIVE);
            case DRINK:
                return drink(command->verb, command->obj);
            case RUB:
                return GO_UNKNOWN;
            case THROW:
                return GO_UNKNOWN;
            case QUIT:
                return quit();
            case FIND:
                return GO_UNKNOWN;
            case INVENTORY:
                return inven();
            case FEED:
                return GO_UNKNOWN;
            case FILL:
                return fill(command->verb, command->obj);
            case BLAST:
                blast();
                return GO_CLEAROBJ;
            case SCORE:
                score(scoregame);
                return GO_CLEAROBJ;
            case GIANTWORDS:
                return bigwords(command->wd1);
            case BRIEF:
                return brief();
            case READ:
                command->obj = INTRANSITIVE;
                return read(*command);
            case BREAK:
                return GO_UNKNOWN;
            case WAKE:
                return GO_UNKNOWN;
            case SAVE:
                return suspend();
            case RESUME:
                return resume();
            case FLY:
                return fly(command->verb, INTRANSITIVE);
            case LISTEN:
                return listen();
            case PART:
                return reservoir();
            default:
                BUG(INTRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST); // LCOV_EXCL_LINE
            }
        }
    /* FALLTHRU */
    case transitive:
        /*  Analyse a transitive verb. */
        switch (command->verb) {
        case  CARRY:
            return vcarry(command->verb, command->obj);
        case  DROP:
            return discard(command->verb, command->obj, false);
        case  SAY:
            return say(command);
        case  UNLOCK:
            return lock(command->verb, command->obj);
        case  NOTHING: {
            rspeak(OK_MAN);
            return (GO_CLEAROBJ);
        }
        case  LOCK:
            return lock(command->verb, command->obj);
        case LIGHT:
            return light(command->verb, command->obj);
        case EXTINGUISH:
            return extinguish(command->verb, command->obj);
        case WAVE:
            return wave(command->verb, command->obj);
        case TAME: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case GO: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case ATTACK:
            return attack(command);
        case POUR:
            return pour(command->verb, command->obj);
        case EAT:
            return eat(command->verb, command->obj);
        case DRINK:
            return drink(command->verb, command->obj);
        case RUB:
            return rub(command->verb, command->obj);
        case THROW:
            return throw (command);
        case QUIT: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case FIND:
            return find(command->verb, command->obj);
        case INVENTORY:
            return find(command->verb, command->obj);
        case FEED:
            return feed(command->verb, command->obj);
        case FILL:
            return fill(command->verb, command->obj);
        case BLAST:
            blast();
            return GO_CLEAROBJ;
        case SCORE: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case GIANTWORDS: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case BRIEF: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case READ:
            return read(*command);
        case BREAK:
            return vbreak(command->verb, command->obj);
        case WAKE:
            return wake(command->verb, command->obj);
        case SAVE: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case RESUME: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case FLY:
            return fly(command->verb, command->obj);
        case LISTEN: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case PART:
            return reservoir();
        default:
            BUG(TRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST); // LCOV_EXCL_LINE
        }
    case unknown:
        /* Unknown verb, couldn't deduce object - might need hint */
        rspeak(WHAT_DO, command->wd1, command->wd1x);
        return GO_CHECKHINT;
    default:
        BUG(SPEECHPART_NOT_TRANSITIVE_OR_INTRANSITIVE_OR_UNKNOWN); // LCOV_EXCL_LINE
    }
}
