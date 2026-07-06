#include "CPoint.h"

#ifndef CAI
#define CAI

// Rule knobs for the game variants the engine can host. Populated once in
// the CAi constructor from the canonical server game ID (GridStateFactory):
// Pente=1, Keryo=3, Poof=11, Connect6=13, Boat=15, O-Pente=25; even IDs are
// Speed twins with identical board rules. Legacy callers passing 2 get Keryo.
struct VariantConfig {
	bool capturePairs = true;       // gates capture MECHANICS in dmov/Tree only; Score/Eval
	                                // still score capture patterns unconditionally, so
	                                // Connect6 (false) is not playable until plan §4 lands
	bool captureTriples = false;    // Keryo/O-Pente triple capture
	int  capWinCount = 10;          // captured stones needed to win
	bool poofPairs = false;         // Poof/O-Pente 2-stone poof; not consumed yet
	bool poofTriples = false;       // O-Pente 3-stone poof; not consumed yet
	bool boatWin = false;           // Boat/O-Pente provisional five; not consumed yet
	int  winRowLength = 5;          // Connect6: 6; not consumed yet (pattern tables are 5-based)
	int  stonesPerTurn = 1;         // Connect6: 2; not consumed yet
	bool tournamentOpening = true;  // feeds 'tourn', which the engine currently never reads
};

class CAi {

// Construction
public:
	CAi(int game1, int lvl, bool openingBook1);

// Attributes
protected:
	int level;
	int seat;

    short *scores, *table, *obk;
    int *pFk, *pFh, *pTr, *pTm, *pTo, *pTi, *pfhn;

	unsigned long moveNum;
	static const int size = 19;

	static const int bsize = 912;
	static const int tsize = 943;

	static const int openingBookSize = 600;

	int cp;
	unsigned long tn;

	int obfl, cob, crot, obsize, extnt;
	int p[7], ccc[18][7], sx[362], sy[362];

	int dx[8] = {-1,0,1,-1,1,0,-1,1};
	int dy[8] = {-1,-1,-1,0,1,1,1,0};
	int rotx[8] = {1,1,1,1,-1,-1,-1,-1};
	int roty[8] = {1,1,-1,-1,-1,-1,1,1};
	int rotf[8] = {0,1,0,1,0,1,0,1};
	//int mxnd[] = {0, 20, 18, 16, 14, 12, 12, 12, 12, 12, 8,8,8,8,8,8, 8,8,8};
	int mxvt[19]={0,1,3,4,6,7,8,9,10,12,13,14,15, 18,18,18,18,18,18};
	int mxvf[19]={0,1,4,5,7,8,10,11,13,15,16,17,18, 18,18,18,18,18,18};

	int xoff, yoff, rlct;
	int rlst[800], rrot[800], oscr[openingBookSize], nom[openingBookSize];
	int brd[18][size][size];//, ciel[7][18];
	// 18 levels of 19x19 board
	// each ply of the search is a level
	// 0=empty, 1=player 1 stone, 2=player 2;
	// -1=empty space within 2 spaces of a stone
	// the computer will only consider moving to '-1'.
	int bmove, bscr;



	const int om2[8] = {181,182,162,163,164,165,144,145};
	const int op2[8] = {25,36,77,82,93,95,97,99};
	const int om3[32] = {183,184,202,221,240,260,239,238,237,256,236,235,
	    234,252,215,196,177,176,158,139,120,100,
	    121,122,123,104,124,125,126,108,145,164};

	int mxst=2, gf=0,
	multipbem=0, np=2;

	VariantConfig cfg;
	unsigned long turn;

	int fr, fhn, en, cap1, tourn;
	int mvct, lvl, ppd, vct, ferr, alpha, beta, a1, b1, erfl;
	int sco[7], plv;

	unsigned char *pHashD;
	short int *pHashS;
	int exfl[20], exel[20];
	int maxscr, extent, breadth;
	unsigned int *pHashY, HValX[20], HValY[20],
	TableX[362], TableY[362];
	int bd[19][19], cc[20][7], p1d[24], p2d[24], p3d[24], cap2, cap3;

	CPoint p1xy[24], pxy[24], p2xy[24], p3xy[24];
	CPoint pPxy[17]; int pPd[17]; int capP; // poof: own stones vanishing with the played stone (<=16 with triples)
	int capPf; // poof: per-form referee bonus (pairFired+tripleFired, 0-2) set alongside capP;
	           // consumed only in Score()'s own tail (same call that set it) so, unlike capP,
	           // it needs no Eval() tcapP-style snapshot -- nested rescore Score() calls never read it.
	int rowWin;                           // boat: Score() saw an (unclamped) >=5 row win this call
	int pendN[20]; CPoint pendC[20][9];   // boat: provisional-run cells recorded per ply

	// Connect6 (cfg.stonesPerTurn==2): tn at Move() entry, used by Tree() to
	// rotate the player from the global 0-based stone index via ownerOf().
	int tnRoot;
	int c6FallbackHits = 0;               // Connect6: times the defensive 2nd-stone fallback fired


	int ciel[20][7], mxnd[20], sec[3];

	short int *pAs, *pAt;//*pPbk, *pKbk, *pPNm, *pPOs, *pKNm, *pKOs;


// Operations
public:
	int getMove(int *moves, int count);
	// void setUseOpeningBook(bool book);
	// void setLevel(int lvl);
	// note: no setGame — cfg is derived from the game ID once, in the
	// constructor; construct a fresh CAi to switch variants.

protected:
	static VariantConfig configFor(int gameId);
	// Owner (1/2) of the stone at 0-based global index idx. Connect6 packs two
	// stones per turn (idx%4 in {0,3} -> P1, else P2); every other variant
	// strictly alternates. NOTE: not static (unlike configFor) because it reads
	// cfg.stonesPerTurn; the spec's "member static" wording can't hold here.
	int ownerOf(int idx);
	int Tree();
	int Eval(int x, int y);
	int Score(CPoint pt);
	int Score6(CPoint pt);   // Connect6: direct 6-window eval, bypasses 5-based tables
	int boatRunAxis(int x, int y, int p, int a, CPoint *cells); // boat: maximal own run on one axis
	int boatRun(int x, int y, int p, CPoint *cells);            // boat: first axis whose run is >=5
	int boatRunProof(int x, int y, int p);                      // boat: five has no capturable stone?
	void reset();
	int Move();
	void dmov();
	int cmove();    
	void addMove(int move, int count);
// void Print();

// Implementation
public:
	virtual ~CAi();

protected:

};

#endif
