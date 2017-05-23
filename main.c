/*
 * The author - Don Woods - apologises for the style of the code; it
 * is a result of running the original Fortran IV source through a
 * home-brew Fortran-to-C converter.)
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "main.h"

#include "misc.h"

long ABB[186], ATAB[331], ATLOC[186], BLKLIN = true, DFLAG,
		DLOC[7], FIXED[101], HOLDNG,
		KTAB[331], *LINES, LINK[201], LNLENG, LNPOSN,
		PARMS[26], PLACE[101], PTEXT[101], RTEXT[278],
		SETUP = 0, TABSIZ = 330;
signed char INLINE[LINESIZE+1], MAP1[129], MAP2[129];

long ABBNUM, ACTSPK[36], AMBER, ATTACK, AXE, BACK, BATTER, BEAR, BIRD, BLOOD, BONUS,
		 BOTTLE, CAGE, CAVE, CAVITY, CHAIN, CHASM, CHEST, CHLOC, CHLOC2,
		CLAM, CLOCK1, CLOCK2, CLOSED, CLOSNG, CLSHNT, CLSMAX = 12, CLSSES,
		COINS, COND[186], CONDS, CTEXT[13], CVAL[13], DALTLC, DETAIL,
		 DKILL, DOOR, DPRSSN, DRAGON, DSEEN[7], DTOTAL, DWARF, EGGS,
		EMRALD, ENTER, ENTRNC, FIND, FISSUR, FIXD[101], FOOBAR, FOOD,
		GRATE, HINT, HINTED[21], HINTLC[21], HINTS[21][5], HNTMAX,
		HNTSIZ = 20, I, INVENT, IGO, IWEST, J, JADE, K, K2, KEY[186], KEYS, KK,
		KNFLOC, KNIFE, KQ, L, LAMP, LIMIT, LINSIZ = 12500, LINUSE, LL,
		LMWARN, LOC, LOCK, LOCSIZ = 185, LOCSND[186], LOOK, LTEXT[186],
		MAGZIN, MAXDIE, MAXTRS, MESH = 123456789,
		MESSAG, MIRROR, MXSCOR,
		NEWLOC, NOVICE, NUGGET, NUL, NUMDIE, OBJ, OBJSND[101],
		OBJTXT[101], ODLOC[7], OGRE, OIL, OLDLC2, OLDLOC, OLDOBJ, OYSTER,
		PANIC, PEARL, PILLOW, PLAC[101], PLANT, PLANT2, PROP[101], PYRAM,
		RESER, ROD, ROD2, RTXSIZ = 277, RUBY, RUG, SAPPH, SAVED, SAY,
		SCORE, SECT, SIGN, SNAKE, SPK, STEPS, STEXT[186], STICK,
		STREAM, TABNDX, TALLY, THRESH, THROW, TK[21], TRAVEL[886], TRIDNT,
		TRNDEX, TRNLUZ, TRNSIZ = 5, TRNVAL[6], TRNVLS, TROLL, TROLL2, TRVS,
		 TRVSIZ = 885, TTEXT[6], TURNS, URN, V1, V2, VASE, VEND, VERB,
		VOLCAN, VRBSIZ = 35, VRSION = 25, WATER, WD1, WD1X, WD2, WD2X,
		WZDARK = false, ZZWORD;
FILE  *logfp;
bool oldstyle = false;

extern void initialise();
extern void score(long);
extern int action(long);

/*
 * MAIN PROGRAM
 */

static void do_command(void);

int main(int argc, char *argv[]) {
	int ch;

/*  Adventure (rev 2: 20 treasures) */

/*  History: Original idea & 5-treasure version (adventures) by Willie Crowther
 *           15-treasure version (adventure) by Don Woods, April-June 1977
 *           20-treasure version (rev 2) by Don Woods, August 1978
 *		Errata fixed: 78/12/25 */


/*  Options. */

	while ((ch = getopt(argc, argv, "l:o")) != EOF) {
		switch (ch) {
		case 'l':
			logfp = fopen(optarg, "w+");
			if (logfp == NULL)
				fprintf(stderr,
					"advent: can't open logfile %s for write\n",
					optarg);
			break;
		case 'o':
		    oldstyle = true;
		    break;
		}
	}

/* Logical variables:
 *
 *  CLOSED says whether we're all the way closed
 *  CLOSNG says whether it's closing time yet
 *  CLSHNT says whether he's read the clue in the endgame
 *  LMWARN says whether he's been warned about lamp going dim
 *  NOVICE says whether he asked for instructions at start-up
 *  PANIC says whether he's found out he's trapped in the cave
 *  WZDARK says whether the loc he's leaving was dark */

#include "funcs.h"

/*  Read the database if we have not yet done so */

	LINES = (long *)calloc(LINSIZ+1,sizeof(long));
	if(!LINES){
		printf("Not enough memory!\n");
		exit(1);
	}

	MAP2[1] = 0;
	if(!SETUP)initialise();
	if(SETUP > 0) goto L1;

/*  Unlike earlier versions, adventure is no longer restartable.  (This
 *  lets us get away with modifying things such as OBJSND(BIRD) without
 *  having to be able to undo the changes later.)  If a "used" copy is
 *  rerun, we come here and tell the player to run a fresh copy. */

	RSPEAK(201);
	exit(0);

/*  Start-up, dwarf stuff */

L1:	SETUP= -1;
	I=RAN(-1);
	ZZWORD=RNDVOC(3,0)+MESH*2;
	NOVICE=YES(65,1,0);
	NEWLOC=1;
	LOC=1;
	LIMIT=330;
	if(NOVICE)LIMIT=1000;

	for (;;) {
	    do_command();
	}
}

static void do_command(void) {

/*  Can't leave cave once it's closing (except by main office). */

L2:	if(!OUTSID(NEWLOC) || NEWLOC == 0 || !CLOSNG) goto L71;
	RSPEAK(130);
	NEWLOC=LOC;
	if(!PANIC)CLOCK2=15;
	PANIC=true;

/*  See if a dwarf has seen him and has come from where he wants to go.  If so,
 *  the dwarf's blocking his way.  If coming from place forbidden to pirate
 *  (dwarves rooted in place) let him get out (and attacked). */

L71:	if(NEWLOC == LOC || FORCED(LOC) || CNDBIT(LOC,3)) goto L74;
	/* 73 */ for (I=1; I<=5; I++) {
	if(ODLOC[I] != NEWLOC || !DSEEN[I]) goto L73;
	NEWLOC=LOC;
	RSPEAK(2);
	 goto L74;
L73:	/*etc*/ ;
	} /* end loop */
L74:	LOC=NEWLOC;

/*  Dwarf stuff.  See earlier comments for description of variables.  Remember
 *  sixth dwarf is pirate and is thus very different except for motion rules. */

/*  First off, don't let the dwarves follow him into a pit or a wall.  Activate
 *  the whole mess the first time he gets as far as the hall of mists (loc 15).
 *  If NEWLOC is forbidden to pirate (in particular, if it's beyond the troll
 *  bridge), bypass dwarf stuff.  That way pirate can't steal return toll, and
 *  dwarves can't meet the bear.  Also means dwarves won't follow him into dead
 *  end in maze, but c'est la vie.  They'll wait for him outside the dead end. */

	if(LOC == 0 || FORCED(LOC) || CNDBIT(NEWLOC,3)) goto L2000;
	if(DFLAG != 0) goto L6000;
	if(INDEEP(LOC))DFLAG=1;
	 goto L2000;

/*  When we encounter the first dwarf, we kill 0, 1, or 2 of the 5 dwarves.  If
 *  any of the survivors is at loc, replace him with the alternate. */

L6000:	if(DFLAG != 1) goto L6010;
	if(!INDEEP(LOC) || (PCT(95) && (!CNDBIT(LOC,4) || PCT(85)))) goto L2000;
	DFLAG=2;
	for (I=1; I<=2; I++) {
	J=1+RAN(5);
	if(PCT(50))DLOC[J]=0;
	} /* end loop */
	for (I=1; I<=5; I++) {
	if(DLOC[I] == LOC)DLOC[I]=DALTLC;
	ODLOC[I]=DLOC[I];
	} /* end loop */
	RSPEAK(3);
	DROP(AXE,LOC);
	 goto L2000;

/*  Things are in full swing.  Move each dwarf at random, except if he's seen us
 *  he sticks with us.  Dwarves stay deep inside.  If wandering at random,
 *  they don't back up unless there's no alternative.  If they don't have to
 *  move, they attack.  And, of course, dead dwarves don't do much of anything. */

L6010:	DTOTAL=0;
	ATTACK=0;
	STICK=0;
	/* 6030 */ for (I=1; I<=6; I++) {
	if(DLOC[I] == 0) goto L6030;
/*  Fill TK array with all the places this dwarf might go. */
	J=1;
	KK=DLOC[I];
	KK=KEY[KK];
	if(KK == 0) goto L6016;
L6012:	NEWLOC=MOD(IABS(TRAVEL[KK])/1000,1000);
	{long x = J-1;
	if(NEWLOC > 300 || !INDEEP(NEWLOC) || NEWLOC == ODLOC[I] || (J > 1 &&
		NEWLOC == TK[x]) || J >= 20 || NEWLOC == DLOC[I] ||
		FORCED(NEWLOC) || (I == 6 && CNDBIT(NEWLOC,3)) ||
		IABS(TRAVEL[KK])/1000000 == 100) goto L6014;}
	TK[J]=NEWLOC;
	J=J+1;
L6014:	KK=KK+1;
	{long x = KK-1; if(TRAVEL[x] >= 0) goto L6012;}
L6016:	TK[J]=ODLOC[I];
	if(J >= 2)J=J-1;
	J=1+RAN(J);
	ODLOC[I]=DLOC[I];
	DLOC[I]=TK[J];
	DSEEN[I]=(DSEEN[I] && INDEEP(LOC)) || (DLOC[I] == LOC || ODLOC[I] == LOC);
	if(!DSEEN[I]) goto L6030;
	DLOC[I]=LOC;
	if(I != 6) goto L6027;

/*  The pirate's spotted him.  He leaves him alone once we've found chest.  K
 *  counts if a treasure is here.  If not, and tally=1 for an unseen chest, let
 *  the pirate be spotted.  Note that PLACE(CHEST)=0 might mean that he's
 *  thrown it to the troll, but in that case he's seen the chest (PROP=0). */

	if(LOC == CHLOC || PROP[CHEST] >= 0) goto L6030;
	K=0;
	/* 6020 */ for (J=50; J<=MAXTRS; J++) {
/*  Pirate won't take pyramid from plover room or dark room (too easy!). */
	if(J == PYRAM && (LOC == PLAC[PYRAM] || LOC == PLAC[EMRALD])) goto L6020;
	if(TOTING(J)) goto L6021;
L6020:	if(HERE(J))K=1;
	} /* end loop */
	if(TALLY == 1 && K == 0 && PLACE[CHEST] == 0 && HERE(LAMP) && PROP[LAMP]
		== 1) goto L6025;
	if(ODLOC[6] != DLOC[6] && PCT(20))RSPEAK(127);
	 goto L6030;

L6021:	if(PLACE[CHEST] != 0) goto L6022;
/*  Install chest only once, to insure it is the last treasure in the list. */
	MOVE(CHEST,CHLOC);
	MOVE(MESSAG,CHLOC2);
L6022:	RSPEAK(128);
	/* 6023 */ for (J=50; J<=MAXTRS; J++) {
	if(J == PYRAM && (LOC == PLAC[PYRAM] || LOC == PLAC[EMRALD])) goto L6023;
	if(AT(J) && FIXED[J] == 0)CARRY(J,LOC);
	if(TOTING(J))DROP(J,CHLOC);
L6023:	/*etc*/ ;
	} /* end loop */
L6024:	DLOC[6]=CHLOC;
	ODLOC[6]=CHLOC;
	DSEEN[6]=false;
	 goto L6030;

L6025:	RSPEAK(186);
	MOVE(CHEST,CHLOC);
	MOVE(MESSAG,CHLOC2);
	 goto L6024;

/*  This threatening little dwarf is in the room with him! */

L6027:	DTOTAL=DTOTAL+1;
	if(ODLOC[I] != DLOC[I]) goto L6030;
	ATTACK=ATTACK+1;
	if(KNFLOC >= 0)KNFLOC=LOC;
	if(RAN(1000) < 95*(DFLAG-2))STICK=STICK+1;
L6030:	/*etc*/ ;
	} /* end loop */

/*  Now we know what's happening.  Let's tell the poor sucker about it.
 *  Note that various of the "knife" messages must have specific relative
 *  positions in the RSPEAK database. */

	if(DTOTAL == 0) goto L2000;
	SETPRM(1,DTOTAL,0);
	RSPEAK(4+1/DTOTAL);
	if(ATTACK == 0) goto L2000;
	if(DFLAG == 2)DFLAG=3;
	SETPRM(1,ATTACK,0);
	K=6;
	if(ATTACK > 1)K=250;
	RSPEAK(K);
	SETPRM(1,STICK,0);
	RSPEAK(K+1+2/(1+STICK));
	if(STICK == 0) goto L2000;
	OLDLC2=LOC;
	 goto L99;






/*  Describe the current location and (maybe) get next command. */

/*  Print text for current loc. */

L2000:	if(LOC == 0) goto L99;
	KK=STEXT[LOC];
	if(MOD(ABB[LOC],ABBNUM) == 0 || KK == 0)KK=LTEXT[LOC];
	if(FORCED(LOC) || !DARK(0)) goto L2001;
	if(WZDARK && PCT(35)) goto L90;
	KK=RTEXT[16];
L2001:	if(TOTING(BEAR))RSPEAK(141);
	SPEAK(KK);
	K=1;
	if(FORCED(LOC)) goto L8;
	if(LOC == 33 && PCT(25) && !CLOSNG)RSPEAK(7);

/*  Print out descriptions of objects at this location.  If not closing and
 *  property value is negative, tally off another treasure.  Rug is special
 *  case; once seen, its PROP is 1 (dragon on it) till dragon is killed.
 *  Similarly for chain; PROP is initially 1 (locked to bear).  These hacks
 *  are because PROP=0 is needed to get full score. */

	if(DARK(0)) goto L2012;
	ABB[LOC]=ABB[LOC]+1;
	I=ATLOC[LOC];
L2004:	if(I == 0) goto L2012;
	OBJ=I;
	if(OBJ > 100)OBJ=OBJ-100;
	if(OBJ == STEPS && TOTING(NUGGET)) goto L2008;
	if(PROP[OBJ] >= 0) goto L2006;
	if(CLOSED) goto L2008;
	PROP[OBJ]=0;
	if(OBJ == RUG || OBJ == CHAIN)PROP[OBJ]=1;
	TALLY=TALLY-1;
/*  Note: There used to be a test here to see whether the player had blown it
 *  so badly that he could never ever see the remaining treasures, and if so
 *  the lamp was zapped to 35 turns.  But the tests were too simple-minded;
 *  things like killing the bird before the snake was gone (can never see
 *  jewelry), and doing it "right" was hopeless.  E.G., could cross troll
 *  bridge several times, using up all available treasures, breaking vase,
 *  using coins to buy batteries, etc., and eventually never be able to get
 *  across again.  If bottle were left on far side, could then never get eggs
 *  or trident, and the effects propagate.  So the whole thing was flushed.
 *  anyone who makes such a gross blunder isn't likely to find everything
 *  else anyway (so goes the rationalisation). */
L2006:	KK=PROP[OBJ];
	if(OBJ == STEPS && LOC == FIXED[STEPS])KK=1;
	PSPEAK(OBJ,KK);
L2008:	I=LINK[I];
	 goto L2004;

L2009:	K=54;
L2010:	SPK=K;
L2011:	RSPEAK(SPK);

L2012:	VERB=0;
	OLDOBJ=OBJ;
	OBJ=0;

/*  Check if this loc is eligible for any hints.  If been here long enough,
 *  branch to help section (on later page).  Hints all come back here eventually
 *  to finish the loop.  Ignore "HINTS" < 4 (special stuff, see database notes).
		*/

L2600:	if(COND[LOC] < CONDS) goto L2603;
	/* 2602 */ for (HINT=1; HINT<=HNTMAX; HINT++) {
	if(HINTED[HINT]) goto L2602;
	if(!CNDBIT(LOC,HINT+10))HINTLC[HINT]= -1;
	HINTLC[HINT]=HINTLC[HINT]+1;
	if(HINTLC[HINT] >= HINTS[HINT][1]) goto L40000;
L2602:	/*etc*/ ;
	} /* end loop */

/*  Kick the random number generator just to add variety to the chase.  Also,
 *  if closing time, check for any objects being toted with PROP < 0 and set
 *  the prop to -1-PROP.  This way objects won't be described until they've
 *  been picked up and put down separate from their respective piles.  Don't
 *  tick CLOCK1 unless well into cave (and not at Y2). */

L2603:	if(!CLOSED) goto L2605;
	if(PROP[OYSTER] < 0 && TOTING(OYSTER))PSPEAK(OYSTER,1);
	for (I=1; I<=100; I++) {
	if(TOTING(I) && PROP[I] < 0)PROP[I]= -1-PROP[I];
	} /* end loop */
L2605:	WZDARK=DARK(0);
	if(KNFLOC > 0 && KNFLOC != LOC)KNFLOC=0;
	I=RAN(1);
	GETIN(WD1,WD1X,WD2,WD2X);

/*  Every input, check "FOOBAR" flag.  If zero, nothing's going on.  If pos,
 *  make neg.  If neg, he skipped a word, so make it zero. */

L2607:	FOOBAR=(FOOBAR>0 ? -FOOBAR : 0);
	TURNS=TURNS+1;
	if(TURNS != THRESH) goto L2608;
	SPEAK(TTEXT[TRNDEX]);
	TRNLUZ=TRNLUZ+TRNVAL[TRNDEX]/100000;
	TRNDEX=TRNDEX+1;
	THRESH= -1;
	if(TRNDEX <= TRNVLS)THRESH=MOD(TRNVAL[TRNDEX],100000)+1;
L2608:	if(VERB == SAY && WD2 > 0)VERB=0;
	if(VERB == SAY) goto L4090;
	if(TALLY == 0 && INDEEP(LOC) && LOC != 33)CLOCK1=CLOCK1-1;
	if(CLOCK1 == 0) goto L10000;
	if(CLOCK1 < 0)CLOCK2=CLOCK2-1;
	if(CLOCK2 == 0) goto L11000;
	if(PROP[LAMP] == 1)LIMIT=LIMIT-1;
	if(LIMIT <= 30 && HERE(BATTER) && PROP[BATTER] == 0 && HERE(LAMP)) goto
		L12000;
	if(LIMIT == 0) goto L12400;
	if(LIMIT <= 30) goto L12200;
L19999: K=43;
	if(LIQLOC(LOC) == WATER)K=70;
	V1=VOCAB(WD1,-1);
	V2=VOCAB(WD2,-1);
	if(V1 == ENTER && (V2 == STREAM || V2 == 1000+WATER)) goto L2010;
	if(V1 == ENTER && WD2 > 0) goto L2800;
	if((V1 != 1000+WATER && V1 != 1000+OIL) || (V2 != 1000+PLANT && V2 !=
		1000+DOOR)) goto L2610;
	{long x = V2-1000; if(AT(x))WD2=MAKEWD(16152118);}
L2610:	if(V1 == 1000+CAGE && V2 == 1000+BIRD && HERE(CAGE) &&
		HERE(BIRD))WD1=MAKEWD(301200308);
L2620:	if(WD1 != MAKEWD(23051920)) goto L2625;
	IWEST=IWEST+1;
	if(IWEST == 10)RSPEAK(17);
L2625:	if(WD1 != MAKEWD( 715) || WD2 == 0) goto L2630;
	IGO=IGO+1;
	if(IGO == 10)RSPEAK(276);
L2630:	I=VOCAB(WD1,-1);
	if(I == -1) goto L3000;
	K=MOD(I,1000);
	KQ=I/1000+1;
	 switch (KQ-1) { case 0: goto L8; case 1: goto L5000; case 2: goto L4000;
		case 3: goto L2010; }
	BUG(22);

/*  Get second word for analysis. */

L2800:	WD1=WD2;
	WD1X=WD2X;
	WD2=0;
	 goto L2620;

/*  Gee, I don't understand. */

L3000:	SETPRM(1,WD1,WD1X);
	RSPEAK(254);
	 goto L2600;

/* Verb and object analysis moved to separate module. */

L4000:	I=4000; goto Laction;
L4090:	I=4090; goto Laction;
L5000:	I=5000;
Laction:
	switch (action(I)) {
	   case 2: return;
	   case 8: goto L8;
	   case 2000: goto L2000;
	   case 2009: goto L2009;
	   case 2010: goto L2010;
	   case 2011: goto L2011;
	   case 2012: goto L2012;
	   case 2600: goto L2600;
	   case 2607: goto L2607;
	   case 2630: goto L2630;
	   case 2800: goto L2800;
	   case 8000: goto L8000;
	   case 18999: goto L18999;
	   case 19000: goto L19000;
	   }
	BUG(99);

/*  Random intransitive verbs come here.  Clear obj just in case (see "attack").
		*/

L8000:	SETPRM(1,WD1,WD1X);
	RSPEAK(257);
	OBJ=0;
	goto L2600;

/*  Figure out the new location
 *
 *  Given the current location in "LOC", and a motion verb number in "K", put
 *  the new location in "NEWLOC".  The current loc is saved in "OLDLOC" in case
 *  he wants to retreat.  The current OLDLOC is saved in OLDLC2, in case he
 *  dies.  (if he does, NEWLOC will be limbo, and OLDLOC will be what killed
 *  him, so we need OLDLC2, which is the last place he was safe.) */

L8:	KK=KEY[LOC];
	NEWLOC=LOC;
	if(KK == 0)BUG(26);
	if(K == NUL) return;
	if(K == BACK) goto L20;
	if(K == LOOK) goto L30;
	if(K == CAVE) goto L40;
	OLDLC2=OLDLOC;
	OLDLOC=LOC;

L9:	LL=IABS(TRAVEL[KK]);
	if(MOD(LL,1000) == 1 || MOD(LL,1000) == K) goto L10;
	if(TRAVEL[KK] < 0) goto L50;
	KK=KK+1;
	 goto L9;

L10:	LL=LL/1000;
L11:	NEWLOC=LL/1000;
	K=MOD(NEWLOC,100);
	if(NEWLOC <= 300) goto L13;
	if(PROP[K] != NEWLOC/100-3) goto L16;
L12:	if(TRAVEL[KK] < 0)BUG(25);
	KK=KK+1;
	NEWLOC=IABS(TRAVEL[KK])/1000;
	if(NEWLOC == LL) goto L12;
	LL=NEWLOC;
	 goto L11;

L13:	if(NEWLOC <= 100) goto L14;
	if(TOTING(K) || (NEWLOC > 200 && AT(K))) goto L16;
	 goto L12;

L14:	if(NEWLOC != 0 && !PCT(NEWLOC)) goto L12;
L16:	NEWLOC=MOD(LL,1000);
	if(NEWLOC <= 300) return;
	if(NEWLOC <= 500) goto L30000;
	RSPEAK(NEWLOC-500);
	NEWLOC=LOC;
	 return;

/*  Special motions come here.  Labelling convention: statement numbers NNNXX
 *  (XX=00-99) are used for special case number NNN (NNN=301-500). */

L30000: NEWLOC=NEWLOC-300;
	 switch (NEWLOC) { case 1: goto L30100; case 2: goto L30200; case 3: goto
		L30300; }
	BUG(20);

/*  Travel 301.  Plover-alcove passage.  Can carry only emerald.  Note: travel
 *  table must include "useless" entries going through passage, which can never
 *  be used for actual motion, but can be spotted by "go back". */

L30100: NEWLOC=99+100-LOC;
	if(HOLDNG == 0 || (HOLDNG == 1 && TOTING(EMRALD))) return;
	NEWLOC=LOC;
	RSPEAK(117);
	 return;

/*  Travel 302.  Plover transport.  Drop the emerald (only use special travel if
 *  toting it), so he's forced to use the plover-passage to get it out.  Having
 *  dropped it, go back and pretend he wasn't carrying it after all. */

L30200: DROP(EMRALD,LOC);
	 goto L12;

/*  Travel 303.  Troll bridge.  Must be done only as special motion so that
 *  dwarves won't wander across and encounter the bear.  (They won't follow the
 *  player there because that region is forbidden to the pirate.)  If
 *  PROP(TROLL)=1, he's crossed since paying, so step out and block him.
 *  (standard travel entries check for PROP(TROLL)=0.)  Special stuff for bear. */

L30300: if(PROP[TROLL] != 1) goto L30310;
	PSPEAK(TROLL,1);
	PROP[TROLL]=0;
	MOVE(TROLL2,0);
	MOVE(TROLL2+100,0);
	MOVE(TROLL,PLAC[TROLL]);
	MOVE(TROLL+100,FIXD[TROLL]);
	JUGGLE(CHASM);
	NEWLOC=LOC;
	 return;

L30310: NEWLOC=PLAC[TROLL]+FIXD[TROLL]-LOC;
	if(PROP[TROLL] == 0)PROP[TROLL]=1;
	if(!TOTING(BEAR)) return;
	RSPEAK(162);
	PROP[CHASM]=1;
	PROP[TROLL]=2;
	DROP(BEAR,NEWLOC);
	FIXED[BEAR]= -1;
	PROP[BEAR]=3;
	OLDLC2=NEWLOC;
	 goto L99;

/*  End of specials. */

/*  Handle "go back".  Look for verb which goes from LOC to OLDLOC, or to OLDLC2
 *  If OLDLOC has forced-motion.  K2 saves entry -> forced loc -> previous loc. */

L20:	K=OLDLOC;
	if(FORCED(K))K=OLDLC2;
	OLDLC2=OLDLOC;
	OLDLOC=LOC;
	K2=0;
	if(K == LOC)K2=91;
	if(CNDBIT(LOC,4))K2=274;
	if(K2 == 0) goto L21;
	RSPEAK(K2);
	 return;

L21:	LL=MOD((IABS(TRAVEL[KK])/1000),1000);
	if(LL == K) goto L25;
	if(LL > 300) goto L22;
	J=KEY[LL];
	if(FORCED(LL) && MOD((IABS(TRAVEL[J])/1000),1000) == K)K2=KK;
L22:	if(TRAVEL[KK] < 0) goto L23;
	KK=KK+1;
	 goto L21;

L23:	KK=K2;
	if(KK != 0) goto L25;
	RSPEAK(140);
	 return;

L25:	K=MOD(IABS(TRAVEL[KK]),1000);
	KK=KEY[LOC];
	 goto L9;

/*  Look.  Can't give more detail.  Pretend it wasn't dark (though it may "now"
 *  be dark) so he won't fall into a pit while staring into the gloom. */

L30:	if(DETAIL < 3)RSPEAK(15);
	DETAIL=DETAIL+1;
	WZDARK=false;
	ABB[LOC]=0;
	 return;

/*  Cave.  Different messages depending on whether above ground. */

L40:	K=58;
	if(OUTSID(LOC) && LOC != 8)K=57;
	RSPEAK(K);
	 return;

/*  Non-applicable motion.  Various messages depending on word given. */

L50:	SPK=12;
	if(K >= 43 && K <= 50)SPK=52;
	if(K == 29 || K == 30)SPK=52;
	if(K == 7 || K == 36 || K == 37)SPK=10;
	if(K == 11 || K == 19)SPK=11;
	if(VERB == FIND || VERB == INVENT)SPK=59;
	if(K == 62 || K == 65)SPK=42;
	if(K == 17)SPK=80;
	RSPEAK(SPK);
	 return;





/*  "You're dead, Jim."
 *
 *  If the current loc is zero, it means the clown got himself killed.  We'll
 *  allow this maxdie times.  MAXDIE is automatically set based on the number of
 *  snide messages available.  Each death results in a message (81, 83, etc.)
 *  which offers reincarnation; if accepted, this results in message 82, 84,
 *  etc.  The last time, if he wants another chance, he gets a snide remark as
 *  we exit.  When reincarnated, all objects being carried get dropped at OLDLC2
 *  (presumably the last place prior to being killed) without change of props.
 *  the loop runs backwards to assure that the bird is dropped before the cage.
 *  (this kluge could be changed once we're sure all references to bird and cage
 *  are done by keywords.)  The lamp is a special case (it wouldn't do to leave
 *  it in the cave).  It is turned off and left outside the building (only if he
 *  was carrying it, of course).  He himself is left inside the building (and
 *  heaven help him if he tries to xyzzy back into the cave without the lamp!).
 *  OLDLOC is zapped so he can't just "retreat". */

/*  The easiest way to get killed is to fall into a pit in pitch darkness. */

L90:	RSPEAK(23);
	OLDLC2=LOC;

/*  Okay, he's dead.  Let's get on with it. */

L99:	if(CLOSNG) goto L95;
	NUMDIE=NUMDIE+1;
	if(!YES(79+NUMDIE*2,80+NUMDIE*2,54)) score(0);
	if(NUMDIE == MAXDIE) score(0);
	PLACE[WATER]=0;
	PLACE[OIL]=0;
	if(TOTING(LAMP))PROP[LAMP]=0;
	/* 98 */ for (J=1; J<=100; J++) {
	I=101-J;
	if(!TOTING(I)) goto L98;
	K=OLDLC2;
	if(I == LAMP)K=1;
	DROP(I,K);
L98:	/*etc*/ ;
	} /* end loop */
	LOC=3;
	OLDLOC=LOC;
	 goto L2000;

/*  He died during closing time.  No resurrection.  Tally up a death and exit. */

L95:	RSPEAK(131);
	NUMDIE=NUMDIE+1;
	 score(0);




/*  Hints */

/*  Come here if he's been long enough at required loc(s) for some unused hint.
 *  hint number is in variable "hint".  Branch to quick test for additional
 *  conditions, then come back to do neat stuff.  Goto 40010 if conditions are
 *  met and we want to offer the hint.  Goto 40020 to clear HINTLC back to zero,
 *  40030 to take no action yet. */

L40000:    switch (HINT-1) { case 0: goto L40100; case 1: goto L40200; case 2: goto
		L40300; case 3: goto L40400; case 4: goto L40500; case 5: goto
		L40600; case 6: goto L40700; case 7: goto L40800; case 8: goto
		L40900; case 9: goto L41000; }
/*		CAVE  BIRD  SNAKE MAZE  DARK  WITT  URN   WOODS OGRE
 *		JADE */
	BUG(27);

L40010: HINTLC[HINT]=0;
	if(!YES(HINTS[HINT][3],0,54)) goto L2602;
	SETPRM(1,HINTS[HINT][2],HINTS[HINT][2]);
	RSPEAK(261);
	HINTED[HINT]=YES(175,HINTS[HINT][4],54);
	if(HINTED[HINT] && LIMIT > 30)LIMIT=LIMIT+30*HINTS[HINT][2];
L40020: HINTLC[HINT]=0;
L40030:  goto L2602;

/*  Now for the quick tests.  See database description for one-line notes. */

L40100: if(PROP[GRATE] == 0 && !HERE(KEYS)) goto L40010;
	 goto L40020;

L40200: if(PLACE[BIRD] == LOC && TOTING(ROD) && OLDOBJ == BIRD) goto L40010;
	 goto L40030;

L40300: if(HERE(SNAKE) && !HERE(BIRD)) goto L40010;
	 goto L40020;

L40400: if(ATLOC[LOC] == 0 && ATLOC[OLDLOC] == 0 && ATLOC[OLDLC2] == 0 && HOLDNG >
		1) goto L40010;
	 goto L40020;

L40500: if(PROP[EMRALD] != -1 && PROP[PYRAM] == -1) goto L40010;
	 goto L40020;

L40600:  goto L40010;

L40700: if(DFLAG == 0) goto L40010;
	 goto L40020;

L40800: if(ATLOC[LOC] == 0 && ATLOC[OLDLOC] == 0 && ATLOC[OLDLC2] == 0) goto
		L40010;
	 goto L40030;

L40900: I=ATDWRF(LOC);
	if(I < 0) goto L40020;
	if(HERE(OGRE) && I == 0) goto L40010;
	 goto L40030;

L41000: if(TALLY == 1 && PROP[JADE] < 0) goto L40010;
	 goto L40020;





/*  Cave closing and scoring */


/*  These sections handle the closing of the cave.  The cave closes "CLOCK1"
 *  turns after the last treasure has been located (including the pirate's
 *  chest, which may of course never show up).  Note that the treasures need not
 *  have been taken yet, just located.  Hence CLOCK1 must be large enough to get
 *  out of the cave (it only ticks while inside the cave).  When it hits zero,
 *  we branch to 10000 to start closing the cave, and then sit back and wait for
 *  him to try to get out.  If he doesn't within CLOCK2 turns, we close the
 *  cave; if he does try, we assume he panics, and give him a few additional
 *  turns to get frantic before we close.  When CLOCK2 hits zero, we branch to
 *  11000 to transport him into the final puzzle.  Note that the puzzle depends
 *  upon all sorts of random things.  For instance, there must be no water or
 *  oil, since there are beanstalks which we don't want to be able to water,
 *  since the code can't handle it.  Also, we can have no keys, since there is a
 *  grate (having moved the fixed object!) there separating him from all the
 *  treasures.  Most of these problems arise from the use of negative prop
 *  numbers to suppress the object descriptions until he's actually moved the
 *  objects. */

/*  When the first warning comes, we lock the grate, destroy the bridge, kill
 *  all the dwarves (and the pirate), remove the troll and bear (unless dead),
 *  and set "CLOSNG" to true.  Leave the dragon; too much trouble to move it.
 *  from now until CLOCK2 runs out, he cannot unlock the grate, move to any
 *  location outside the cave, or create the bridge.  Nor can he be
 *  resurrected if he dies.  Note that the snake is already gone, since he got
 *  to the treasure accessible only via the hall of the mountain king. Also, he's
 *  been in giant room (to get eggs), so we can refer to it.  Also also, he's
 *  gotten the pearl, so we know the bivalve is an oyster.  *And*, the dwarves
 *  must have been activated, since we've found chest. */

L10000: PROP[GRATE]=0;
	PROP[FISSUR]=0;
	for (I=1; I<=6; I++) {
	DSEEN[I]=false;
	DLOC[I]=0;
	} /* end loop */
	MOVE(TROLL,0);
	MOVE(TROLL+100,0);
	MOVE(TROLL2,PLAC[TROLL]);
	MOVE(TROLL2+100,FIXD[TROLL]);
	JUGGLE(CHASM);
	if(PROP[BEAR] != 3)DSTROY(BEAR);
	PROP[CHAIN]=0;
	FIXED[CHAIN]=0;
	PROP[AXE]=0;
	FIXED[AXE]=0;
	RSPEAK(129);
	CLOCK1= -1;
	CLOSNG=true;
	 goto L19999;

/*  ONCE HE'S PANICKED, AND CLOCK2 HAS RUN OUT, WE COME HERE TO SET UP THE
 *  STORAGE ROOM.  THE ROOM HAS TWO LOCS, HARDWIRED AS 115 (NE) AND 116 (SW).
 *  AT THE NE END, WE PLACE EMPTY BOTTLES, A NURSERY OF PLANTS, A BED OF
 *  OYSTERS, A PILE OF LAMPS, RODS WITH STARS, SLEEPING DWARVES, AND HIM.  AT
 *  THE SW END WE PLACE GRATE OVER TREASURES, SNAKE PIT, COVEY OF CAGED BIRDS,
 *  MORE RODS, AND PILLOWS.  A MIRROR STRETCHES ACROSS ONE WALL.  MANY OF THE
 *  OBJECTS COME FROM KNOWN LOCATIONS AND/OR STATES (E.G. THE SNAKE IS KNOWN TO
 *  HAVE BEEN DESTROYED AND NEEDN'T BE CARRIED AWAY FROM ITS OLD "PLACE"),
 *  MAKING THE VARIOUS OBJECTS BE HANDLED DIFFERENTLY.  WE ALSO DROP ALL OTHER
 *  OBJECTS HE MIGHT BE CARRYING (LEST HE HAVE SOME WHICH COULD CAUSE TROUBLE,
 *  SUCH AS THE KEYS).  WE DESCRIBE THE FLASH OF LIGHT AND TRUNDLE BACK. */

L11000: PROP[BOTTLE]=PUT(BOTTLE,115,1);
	PROP[PLANT]=PUT(PLANT,115,0);
	PROP[OYSTER]=PUT(OYSTER,115,0);
	OBJTXT[OYSTER]=3;
	PROP[LAMP]=PUT(LAMP,115,0);
	PROP[ROD]=PUT(ROD,115,0);
	PROP[DWARF]=PUT(DWARF,115,0);
	LOC=115;
	OLDLOC=115;
	NEWLOC=115;

/*  Leave the grate with normal (non-negative) property.  Reuse sign. */

	I=PUT(GRATE,116,0);
	I=PUT(SIGN,116,0);
	OBJTXT[SIGN]=OBJTXT[SIGN]+1;
	PROP[SNAKE]=PUT(SNAKE,116,1);
	PROP[BIRD]=PUT(BIRD,116,1);
	PROP[CAGE]=PUT(CAGE,116,0);
	PROP[ROD2]=PUT(ROD2,116,0);
	PROP[PILLOW]=PUT(PILLOW,116,0);

	PROP[MIRROR]=PUT(MIRROR,115,0);
	FIXED[MIRROR]=116;

	for (I=1; I<=100; I++) {
	if(TOTING(I))DSTROY(I);
	} /* end loop */

	RSPEAK(132);
	CLOSED=true;
	 return;

/*  Another way we can force an end to things is by having the lamp give out.
 *  When it gets close, we come here to warn him.  We go to 12000 if the lamp
 *  and fresh batteries are here, in which case we replace the batteries and
 *  continue.  12200 is for other cases of lamp dying.  12400 is when it goes
 *  out.  Even then, he can explore outside for a while if desired. */

L12000: RSPEAK(188);
	PROP[BATTER]=1;
	if(TOTING(BATTER))DROP(BATTER,LOC);
	LIMIT=LIMIT+2500;
	LMWARN=false;
	 goto L19999;

L12200: if(LMWARN || !HERE(LAMP)) goto L19999;
	LMWARN=true;
	SPK=187;
	if(PLACE[BATTER] == 0)SPK=183;
	if(PROP[BATTER] == 1)SPK=189;
	RSPEAK(SPK);
	 goto L19999;

L12400: LIMIT= -1;
	PROP[LAMP]=0;
	if(HERE(LAMP))RSPEAK(184);
	 goto L19999;

/*  Oh dear, he's disturbed the dwarves. */

L18999: RSPEAK(SPK);
L19000: RSPEAK(136);
	score(0);
}
