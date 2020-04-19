#include "CPoint.h"

#ifndef CAI
#define CAI

class CAi {

// Construction
public:
	CAi(int game1, int lvl, bool openingBook1);

// Attributes
protected: 
	int game;
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

	int mxst=2, gf=0, Kgame=0, //1 = K-pente
	multipbem=0, np=2;
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


	int ciel[20][7], mxnd[20], sec[3];

	short int *pAs, *pAt;//*pPbk, *pKbk, *pPNm, *pPOs, *pKNm, *pKOs;


// Operations
public:
	int getMove(int *moves, int count);
	// void setUseOpeningBook(bool book);
	// void setLevel(int lvl);
	// void setGame(int g);

protected: 
	int Tree();
	int Eval(int x, int y);
	int Score(CPoint pt);  
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
