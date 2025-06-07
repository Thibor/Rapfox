#include <stdio.h>
#include <string.h>
#include "windows.h"
#include <time.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <format>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <sys/timeb.h>

using namespace std;

string name = "Rapfox";
#define MAX_DEPTH 100
#define MAX_SCORE 50000

#define MONTH (\
  __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? "01" : "06") \
: __DATE__ [2] == 'b' ? "02" \
: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? "03" : "04") \
: __DATE__ [2] == 'y' ? "05" \
: __DATE__ [2] == 'l' ? "07" \
: __DATE__ [2] == 'g' ? "08" \
: __DATE__ [2] == 'p' ? "09" \
: __DATE__ [2] == 't' ? "10" \
: __DATE__ [2] == 'v' ? "11" \
: "12")
#define DAY (std::string(1,(__DATE__[4] == ' ' ?  '0' : (__DATE__[4]))) + (__DATE__[5]))
#define YEAR ((__DATE__[7]-'0') * 1000 + (__DATE__[8]-'0') * 100 + (__DATE__[9]-'0') * 10 + (__DATE__[10]-'0') * 1)


static void PrintWelcome() {
	cout << name << " " << YEAR << "-" << MONTH << "-" << DAY << endl;
}

// FEN dedug positions
#define defFen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "

// piece encoding
enum pieces { e, P, N, B, R, Q, K, p, n, b, r, q, k, o };

// square encoding
enum squares {
	a8 = 0, b8, c8, d8, e8, f8, g8, h8,
	a7 = 16, b7, c7, d7, e7, f7, g7, h7,
	a6 = 32, b6, c6, d6, e6, f6, g6, h6,
	a5 = 48, b5, c5, d5, e5, f5, g5, h5,
	a4 = 64, b4, c4, d4, e4, f4, g4, h4,
	a3 = 80, b3, c3, d3, e3, f3, g3, h3,
	a2 = 96, b2, c2, d2, e2, f2, g2, h2,
	a1 = 112, b1, c1, d1, e1, f1, g1, h1, no_sq
};

// capture flags
enum capture_flags { all_moves, only_captures };

// castling binary representation
//
//  bin  dec
// 0001    1  white king can castle to the king side
// 0010    2  white king can castle to the queen side
// 0100    4  black king can castle to the king side
// 1000    8  black king can castle to the queen side
//
// examples
// 1111       both sides an castle both directions
// 1001       black king => queen side
//            white king => king side

// castling writes
enum castling { KC = 1, QC = 2, kc = 4, qc = 8 };

// sides to move
enum sides { white, black };

// ascii pieces
string pieces = ".PNBRQKpnbrqk";
string promoted_pieces = "  nbrq  nbrq  ";

void UciCommand(string str);

static int CharToPiece(char c) {
	return pieces.find(c);
}

static char PieceToChar(int p) {
	return pieces[p];
}

static char PieceToPromotion(int p) {
	char c = promoted_pieces[p];
	//if (c == ' ') return '\0';
	return c;
}

int material_score[13] = {
	  0,      // empty square score
	100,      // white pawn score
	310,      // white knight score
	320,      // white bishop score
	500,      // white rook score
   900,      // white queen score
  10000,      // white king score
   -100,      // black pawn score
   -310,      // black knight score
   -320,      // black bishop score
   -500,      // black rook score
  -900,      // black queen score
 -10000,      // black king score

};

// castling rights

/*
							 castle   move     in      in
							  right    map     binary  decimal

		white king  moved:     1111 & 1100  =  1100    12
  white king's rook moved:     1111 & 1110  =  1110    14
 white queen's rook moved:     1111 & 1101  =  1101    13

		 black king moved:     1111 & 0011  =  1011    3
  black king's rook moved:     1111 & 1011  =  1011    11
 black queen's rook moved:     1111 & 0111  =  0111    7

*/

int castling_rights[128] = {
	 7, 15, 15, 15,  3, 15, 15, 11,  o, o, o, o, o, o, o, o,
	15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
	15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
	15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
	15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
	15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
	15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
	13, 15, 15, 15, 12, 15, 15, 14,  o, o, o, o, o, o, o, o
};

// pawn positional score
const int pawn_score[128] =
{
	90,  90,  90,  90,  90,  90,  90,  90,    o, o, o, o, o, o, o, o,
	30,  30,  30,  40,  40,  30,  30,  30,    o, o, o, o, o, o, o, o,
	20,  20,  20,  30,  30,  30,  20,  20,    o, o, o, o, o, o, o, o,
	10,  10,  10,  20,  20,  10,  10,  10,    o, o, o, o, o, o, o, o,
	 5,   5,  10,  20,  20,   5,   5,   5,    o, o, o, o, o, o, o, o,
	 0,   0,   0,   5,   5,   0,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,   0, -10, -10,   0,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,   0,   0,   0,   0,   0,   0,    o, o, o, o, o, o, o, o
};

// knight positional score
const int knight_score[128] =
{
	-5,   0,   0,   0,   0,   0,   0,  -5,    o, o, o, o, o, o, o, o,
	-5,   0,   0,  10,  10,   0,   0,  -5,    o, o, o, o, o, o, o, o,
	-5,   5,  20,  20,  20,  20,   5,  -5,    o, o, o, o, o, o, o, o,
	-5,  10,  20,  30,  30,  20,  10,  -5,    o, o, o, o, o, o, o, o,
	-5,  10,  20,  30,  30,  20,  10,  -5,    o, o, o, o, o, o, o, o,
	-5,   5,  20,  10,  10,  20,   5,  -5,    o, o, o, o, o, o, o, o,
	-5,   0,   0,   0,   0,   0,   0,  -5,    o, o, o, o, o, o, o, o,
	-5, -10,   0,   0,   0,   0, -10,  -5,    o, o, o, o, o, o, o, o
};

// bishop positional score
const int bishop_score[128] =
{
	 0,   0,   0,   0,   0,   0,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,   0,   0,   0,   0,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,   0,  10,  10,   0,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,  10,  20,  20,  10,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,  10,  20,  20,  10,   0,   0,    o, o, o, o, o, o, o, o,
	 0,  10,   0,   0,   0,   0,  10,   0,    o, o, o, o, o, o, o, o,
	 0,  30,   0,   0,   0,   0,  30,   0,    o, o, o, o, o, o, o, o,
	 0,   0, -10,   0,   0, -10,   0,   0,    o, o, o, o, o, o, o, o

};

// rook positional score
const int rook_score[128] =
{
	50,  50,  50,  50,  50,  50,  50,  50,    o, o, o, o, o, o, o, o,
	50,  50,  50,  50,  50,  50,  50,  50,    o, o, o, o, o, o, o, o,
	 0,   0,  10,  20,  20,  10,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,  10,  20,  20,  10,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,  10,  20,  20,  10,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,  10,  20,  20,  10,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,  10,  20,  20,  10,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,   0,  20,  20,   0,   0,   0,    o, o, o, o, o, o, o, o

};

// king positional score
const int king_score[128] =
{
	 0,   0,   0,   0,   0,   0,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   0,   5,   5,   5,   5,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   5,   5,  10,  10,   5,   5,   0,    o, o, o, o, o, o, o, o,
	 0,   5,  10,  20,  20,  10,   5,   0,    o, o, o, o, o, o, o, o,
	 0,   5,  10,  20,  20,  10,   5,   0,    o, o, o, o, o, o, o, o,
	 0,   0,   5,  10,  10,   5,   0,   0,    o, o, o, o, o, o, o, o,
	 0,   5,   5,  -5,  -5,   0,   5,   0,    o, o, o, o, o, o, o, o,
	 0,   0,   5,   0, -15,   0,  10,   0,    o, o, o, o, o, o, o, o

};

// mirror positional score tables for opposite side
const int mirror_score[128] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,    o, o, o, o, o, o, o, o,
	a2, b2, c2, d2, e2, f2, g2, h2,    o, o, o, o, o, o, o, o,
	a3, b3, c3, d3, e3, f3, g3, h3,    o, o, o, o, o, o, o, o,
	a4, b4, c4, d4, e4, f4, g4, h4,    o, o, o, o, o, o, o, o,
	a5, b5, c5, d5, e5, f5, g5, h5,    o, o, o, o, o, o, o, o,
	a6, b6, c6, d6, e6, f6, g6, h6,    o, o, o, o, o, o, o, o,
	a7, b7, c7, d7, e7, f7, g7, h7,    o, o, o, o, o, o, o, o,
	a8, b8, c8, d8, e8, f8, g8, h8,    o, o, o, o, o, o, o, o
};

// chess board representation
int board[128] = {
	r, n, b, q, k, b, n, r,  o, o, o, o, o, o, o, o,
	p, p, p, p, p, p, p, p,  o, o, o, o, o, o, o, o,
	e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
	e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
	e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
	e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
	P, P, P, P, P, P, P, P,  o, o, o, o, o, o, o, o,
	R, N, B, Q, K, B, N, R,  o, o, o, o, o, o, o, o
};

// side to move
int side = white;

// enpassant square
int enpassant = no_sq;

// castling rights (dec 15 => bin 1111 => both kings can castle to both sides)
int castle = 15;

// kings' squares
int king_square[2] = { e1, e8 };

/*
	Move formatting

	0000 0000 0000 0000 0111 1111       source square
	0000 0000 0011 1111 1000 0000       target square
	0000 0011 1100 0000 0000 0000       promoted piece
	0000 0100 0000 0000 0000 0000       capture flag
	0000 1000 0000 0000 0000 0000       double pawn flag
	0001 0000 0000 0000 0000 0000       enpassant flag
	0010 0000 0000 0000 0000 0000       castling

*/

// encode move
#define encode_move(source, target, piece, capture, pawn, enpassant, castling) \
(                          \
    (source) |             \
    (target << 7) |        \
    (piece << 14) |        \
    (capture << 18) |      \
    (pawn << 19) |         \
    (enpassant << 20) |    \
    (castling << 21)       \
)

// decode move's source square
#define get_move_source(move) (move & 0x7f)

// decode move's target square
#define get_move_target(move) ((move >> 7) & 0x7f)

// decode move's promoted piece
#define get_move_piece(move) ((move >> 14) & 0xf)

// decode move's capture flag
#define get_move_capture(move) ((move >> 18) & 0x1)

// decode move's double pawn push flag
#define get_move_pawn(move) ((move >> 19) & 0x1)

// decode move's enpassant flag
#define get_move_enpassant(move) ((move >> 20) & 0x1)

// decode move's castling flag
#define get_move_castling(move) ((move >> 21) & 0x1)

// convert board square indexes to coordinates
char* square_to_coords[] = {
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "i8", "j8", "k8", "l8", "m8", "n8", "o8", "p8",
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "i7", "j7", "k7", "l7", "m7", "n7", "o7", "p7",
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "i6", "j6", "k6", "l6", "m6", "n6", "o6", "p6",
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "i5", "j5", "k5", "l5", "m5", "n5", "o5", "p5",
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "i4", "j4", "k4", "l4", "m4", "n4", "o4", "p4",
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "i3", "j3", "k3", "l3", "m3", "n3", "o3", "p3",
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "i2", "j2", "k2", "l2", "m2", "n2", "o2", "p2",
	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "i1", "j1", "k1", "l1", "m1", "n1", "o1", "p1"
};

// piece move offsets
int knight_offsets[8] = { 33, 31, 18, 14, -33, -31, -18, -14 };
int bishop_offsets[4] = { 15, 17, -15, -17 };
int rook_offsets[4] = { 16, -16, 1, -1 };
int king_offsets[8] = { 16, -16, 1, -1, 15, 17, -15, -17 };

// move list structure
typedef struct {
	// move list
	int moves[256];

	// move count
	int count;
} moves;

/* get_ms() returns the milliseconds elapsed since midnight,
   January 1, 1970. */
int get_time_ms() {
	struct timeb timebuffer;
	ftime(&timebuffer);
	return (timebuffer.time * 1000) + timebuffer.millitm;
}

static vector<string> SplitString(string s) {
	vector<string> words;
	istringstream iss(s);
	string word;
	while (iss >> word)
		words.push_back(word);
	return words;
}

static int GetInt(vector<string> vs, string name, int def) {
	bool r = false;
	for (string s : vs) {
		if (r) return stoi(s);
		r = s == name;
	}
	return def;
}

static void PrintBoard() {
	for (int rank = 0; rank < 8; rank++)
	{
		// loop over board files
		for (int file = 0; file < 16; file++)
		{
			// init square
			int square = rank * 16 + file;

			// print ranks
			if (file == 0)
				printf(" %d  ", 8 - rank);

			// if square is on board
			if (!(square & 0x88))
				printf("%c ", pieces[board[square]]);
		}
		cout << endl;
	}

	// print files
	cout << "    a b c d e f g h" << endl;

	// print board stats
	printf("    Side:     %s\n", (side == white) ? "white" : "black");
	printf("    Castling:  %c%c%c%c\n", (castle & KC) ? 'K' : '-',
		(castle & QC) ? 'Q' : '-',
		(castle & kc) ? 'k' : '-',
		(castle & qc) ? 'q' : '-');
	printf("    Enpassant:   %s\n", (enpassant == no_sq) ? "no" : square_to_coords[enpassant]);
	printf("    King square: %s\n\n", square_to_coords[king_square[side]]);
}

// reset board
static void ResetBoard() {
	for (int rank = 0; rank < 8; rank++) {
		for (int file = 0; file < 16; file++) {
			int square = rank * 16 + file;

			// if square is on board
			if (!(square & 0x88))
				// reset current board square
				board[square] = e;
		}
	}
	side = white;
	castle = 0;
	enpassant = no_sq;
}




/***********************************************\

			 MOVE GENERATOR FUNCTIONS

\***********************************************/

// is square attacked
static inline int is_square_attacked(int square, int side)
{
	// pawn attacks
	if (!side)
	{
		// if target square is on board and is white pawn
		if (!((square + 17) & 0x88) && (board[square + 17] == P))
			return 1;

		// if target square is on board and is white pawn
		if (!((square + 15) & 0x88) && (board[square + 15] == P))
			return 1;
	}

	else
	{
		// if target square is on board and is black pawn
		if (!((square - 17) & 0x88) && (board[square - 17] == p))
			return 1;

		// if target square is on board and is black pawn
		if (!((square - 15) & 0x88) && (board[square - 15] == p))
			return 1;
	}

	// knight attacks
	for (int index = 0; index < 8; index++)
	{
		// init target square
		int target_square = square + knight_offsets[index];

		// lookup target piece
		int target_piece = board[target_square];

		// if target square is on board
		if (!(target_square & 0x88))
		{
			if (!side ? target_piece == N : target_piece == n)
				return 1;
		}
	}

	// king attacks
	for (int index = 0; index < 8; index++)
	{
		// init target square
		int target_square = square + king_offsets[index];

		// lookup target piece
		int target_piece = board[target_square];

		// if target square is on board
		if (!(target_square & 0x88))
		{
			// if target piece is either white or black king
			if (!side ? target_piece == K : target_piece == k)
				return 1;
		}
	}

	// bishop & queen attacks
	for (int index = 0; index < 4; index++)
	{
		// init target square
		int target_square = square + bishop_offsets[index];

		// loop over attack ray
		while (!(target_square & 0x88))
		{
			// target piece
			int target_piece = board[target_square];

			// if target piece is either white or black bishop or queen
			if (!side ? (target_piece == B || target_piece == Q) : (target_piece == b || target_piece == q))
				return 1;

			// break if hit a piece
			if (target_piece)
				break;

			// increment target square by move offset
			target_square += bishop_offsets[index];
		}
	}

	// rook & queen attacks
	for (int index = 0; index < 4; index++)
	{
		// init target square
		int target_square = square + rook_offsets[index];

		// loop over attack ray
		while (!(target_square & 0x88))
		{
			// target piece
			int target_piece = board[target_square];

			// if target piece is either white or black bishop or queen
			if (!side ? (target_piece == R || target_piece == Q) : (target_piece == r || target_piece == q))
				return 1;

			// break if hit a piece
			if (target_piece)
				break;

			// increment target square by move offset
			target_square += rook_offsets[index];
		}
	}

	return 0;
}

// print attack map
static void PrintAttackedSquares(int side) {
	printf("\n");
	printf("    Attacking side: %s\n\n", !side ? "white" : "black");

	// loop over board ranks
	for (int rank = 0; rank < 8; rank++)
	{
		// loop over board files
		for (int file = 0; file < 16; file++)
		{
			// init square
			int square = rank * 16 + file;

			// print ranks
			if (file == 0)
				printf(" %d  ", 8 - rank);

			// if square is on board
			if (!(square & 0x88))
				printf("%c ", is_square_attacked(square, side) ? 'x' : '.');

		}

		// print new line every time new rank is encountered
		printf("\n");
	}

	printf("\n    a b c d e f g h\n\n");
}

// populate move list
static inline void add_move(moves* move_list, int move)
{
	// push move into the move list
	move_list->moves[move_list->count] = move;

	// increment move count
	move_list->count++;
}


// move generator
static inline void generate_moves(moves* move_list)
{
	// reset move count
	move_list->count = 0;

	// loop over all board squares
	for (int square = 0; square < 128; square++)
	{
		// check if the square is on board
		if (!(square & 0x88))
		{
			// white pawn and castling moves
			if (!side)
			{
				// white pawn moves
				if (board[square] == P)
				{
					// init target square
					int to_square = square - 16;

					// quite white pawn moves (check if target square is on board)
					if (!(to_square & 0x88) && !board[to_square])
					{
						// pawn promotions
						if (square >= a7 && square <= h7)
						{
							add_move(move_list, encode_move(square, to_square, Q, 0, 0, 0, 0));
							add_move(move_list, encode_move(square, to_square, R, 0, 0, 0, 0));
							add_move(move_list, encode_move(square, to_square, B, 0, 0, 0, 0));
							add_move(move_list, encode_move(square, to_square, N, 0, 0, 0, 0));
						}

						else
						{
							// one square ahead pawn move
							add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));

							// two squares ahead pawn move
							if ((square >= a2 && square <= h2) && !board[square - 32])
								add_move(move_list, encode_move(square, square - 32, 0, 0, 1, 0, 0));
						}
					}

					// white pawn capture moves
					for (int index = 0; index < 4; index++)
					{
						// init pawn offset
						int pawn_offset = bishop_offsets[index];

						// white pawn offsets
						if (pawn_offset < 0)
						{
							// init target square
							int to_square = square + pawn_offset;

							// check if target square is on board
							if (!(to_square & 0x88))
							{
								// capture pawn promotion
								if (
									(square >= a7 && square <= h7) &&
									(board[to_square] >= 7 && board[to_square] <= 12)
									)
								{
									add_move(move_list, encode_move(square, to_square, Q, 1, 0, 0, 0));
									add_move(move_list, encode_move(square, to_square, R, 1, 0, 0, 0));
									add_move(move_list, encode_move(square, to_square, B, 1, 0, 0, 0));
									add_move(move_list, encode_move(square, to_square, N, 1, 0, 0, 0));
								}

								else
								{
									// casual capture
									if (board[to_square] >= 7 && board[to_square] <= 12)
										add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));

									// enpassant capture
									if (to_square == enpassant)
										add_move(move_list, encode_move(square, to_square, 0, 1, 0, 1, 0));
								}
							}
						}
					}
				}

				// white king castling
				if (board[square] == K)
				{
					// if king side castling is available
					if (castle & KC)
					{
						// make sure there are empty squares between king & rook
						if (!board[f1] && !board[g1])
						{
							// make sure king & next square are not under attack
							if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
								add_move(move_list, encode_move(e1, g1, 0, 0, 0, 0, 1));
						}
					}

					// if queen side castling is available
					if (castle & QC)
					{
						// make sure there are empty squares between king & rook
						if (!board[d1] && !board[b1] && !board[c1])
						{
							// make sure king & next square are not under attack
							if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black))
								add_move(move_list, encode_move(e1, c1, 0, 0, 0, 0, 1));
						}
					}
				}
			}

			// black pawn and castling moves
			else
			{
				// black pawn moves
				if (board[square] == p)
				{
					// init target square
					int to_square = square + 16;

					// quite black pawn moves (check if target square is on board)
					if (!(to_square & 0x88) && !board[to_square])
					{
						// pawn promotions
						if (square >= a2 && square <= h2)
						{
							add_move(move_list, encode_move(square, to_square, q, 0, 0, 0, 0));
							add_move(move_list, encode_move(square, to_square, r, 0, 0, 0, 0));
							add_move(move_list, encode_move(square, to_square, b, 0, 0, 0, 0));
							add_move(move_list, encode_move(square, to_square, n, 0, 0, 0, 0));
						}

						else
						{
							// one square ahead pawn move
							add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));

							// two squares ahead pawn move
							if ((square >= a7 && square <= h7) && !board[square + 32])
								add_move(move_list, encode_move(square, square + 32, 0, 0, 1, 0, 0));
						}
					}

					// black pawn capture moves
					for (int index = 0; index < 4; index++)
					{
						// init pawn offset
						int pawn_offset = bishop_offsets[index];

						// white pawn offsets
						if (pawn_offset > 0)
						{
							// init target square
							int to_square = square + pawn_offset;

							// check if target square is on board
							if (!(to_square & 0x88))
							{
								// capture pawn promotion
								if (
									(square >= a2 && square <= h2) &&
									(board[to_square] >= 1 && board[to_square] <= 6)
									)
								{
									add_move(move_list, encode_move(square, to_square, q, 1, 0, 0, 0));
									add_move(move_list, encode_move(square, to_square, r, 1, 0, 0, 0));
									add_move(move_list, encode_move(square, to_square, b, 1, 0, 0, 0));
									add_move(move_list, encode_move(square, to_square, n, 1, 0, 0, 0));
								}

								else
								{
									// casual capture
									if (board[to_square] >= 1 && board[to_square] <= 6)
										add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));

									// enpassant capture
									if (to_square == enpassant)
										add_move(move_list, encode_move(square, to_square, 0, 1, 0, 1, 0));
								}
							}
						}
					}
				}

				// black king castling
				if (board[square] == k)
				{
					// if king side castling is available
					if (castle & kc)
					{
						// make sure there are empty squares between king & rook
						if (!board[f8] && !board[g8])
						{
							// make sure king & next square are not under attack
							if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
								add_move(move_list, encode_move(e8, g8, 0, 0, 0, 0, 1));
						}
					}

					// if queen side castling is available
					if (castle & qc)
					{
						// make sure there are empty squares between king & rook
						if (!board[d8] && !board[b8] && !board[c8])
						{
							// make sure king & next square are not under attack
							if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white))
								add_move(move_list, encode_move(e8, c8, 0, 0, 0, 0, 1));
						}
					}
				}
			}

			// knight moves
			if (!side ? board[square] == N : board[square] == n)
			{
				// loop over knight move offsets
				for (int index = 0; index < 8; index++)
				{
					// init target square
					int to_square = square + knight_offsets[index];

					// init target piece
					int piece = board[to_square];

					// make sure target square is onboard
					if (!(to_square & 0x88))
					{
						//
						if (
							!side ?
							(!piece || (piece >= 7 && piece <= 12)) :
							(!piece || (piece >= 1 && piece <= 6))
							)
						{
							// on capture
							if (piece)
								add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));

							// on empty square
							else
								add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
						}
					}
				}
			}

			// king moves
			if (!side ? board[square] == K : board[square] == k)
			{
				// loop over king move offsets
				for (int index = 0; index < 8; index++)
				{
					// init target square
					int to_square = square + king_offsets[index];

					// init target piece
					int piece = board[to_square];

					// make sure target square is onboard
					if (!(to_square & 0x88))
					{
						//
						if (
							!side ?
							(!piece || (piece >= 7 && piece <= 12)) :
							(!piece || (piece >= 1 && piece <= 6))
							)
						{
							// on capture
							if (piece)
								add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));

							// on empty square
							else
								add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
						}
					}
				}
			}

			// bishop & queen moves
			if (
				!side ?
				(board[square] == B) || (board[square] == Q) :
				(board[square] == b) || (board[square] == q)
				)
			{
				// loop over bishop & queen offsets
				for (int index = 0; index < 4; index++)
				{
					// init target square
					int to_square = square + bishop_offsets[index];

					// loop over attack ray
					while (!(to_square & 0x88))
					{
						// init target piece
						int piece = board[to_square];

						// if hits own piece
						if (!side ? (piece >= 1 && piece <= 6) : ((piece >= 7 && piece <= 12)))
							break;

						// if hits opponent's piece
						if (!side ? (piece >= 7 && piece <= 12) : ((piece >= 1 && piece <= 6)))
						{
							add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
							break;
						}

						// if steps into an empty squre
						if (!piece)
							add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));

						// increment target square
						to_square += bishop_offsets[index];
					}
				}
			}

			// rook & queen moves
			if (
				!side ?
				(board[square] == R) || (board[square] == Q) :
				(board[square] == r) || (board[square] == q)
				)
			{
				// loop over bishop & queen offsets
				for (int index = 0; index < 4; index++)
				{
					// init target square
					int to_square = square + rook_offsets[index];

					// loop over attack ray
					while (!(to_square & 0x88))
					{
						// init target piece
						int piece = board[to_square];

						// if hits own piece
						if (!side ? (piece >= 1 && piece <= 6) : ((piece >= 7 && piece <= 12)))
							break;

						// if hits opponent's piece
						if (!side ? (piece >= 7 && piece <= 12) : ((piece >= 1 && piece <= 6)))
						{
							add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
							break;
						}

						// if steps into an empty squre
						if (!piece)
							add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));

						// increment target square
						to_square += rook_offsets[index];
					}
				}
			}
		}
	}
}

static void PrintMoves()
{
	moves move_list[1];
	generate_moves(move_list);
	printf("\n    Move     Capture  Double   Enpass   Castling\n\n");

	// loop over moves in a movelist
	for (int index = 0; index < move_list->count; index++)
	{
		int move = move_list->moves[index];
		printf("    %s%s", square_to_coords[get_move_source(move)], square_to_coords[get_move_target(move)]);
		printf("%c    ", get_move_piece(move) ? promoted_pieces[get_move_piece(move)] : ' ');
		printf("%d        %d        %d        %d\n", get_move_capture(move), get_move_pawn(move), get_move_enpassant(move), get_move_castling(move));
	}

	printf("\n    Total moves: %d\n\n", move_list->count);
}

// copy/restore board position macros
#define copy_board()                                \
    int board_copy[128], king_square_copy[2];       \
    int side_copy, enpassant_copy, castle_copy;     \
    memcpy(board_copy, board, 512);                 \
    side_copy = side;                               \
    enpassant_copy = enpassant;                     \
    castle_copy = castle;                           \
    memcpy(king_square_copy, king_square,8);        \

#define take_back()                                 \
    memcpy(board, board_copy, 512);                 \
    side = side_copy;                               \
    enpassant = enpassant_copy;                     \
    castle = castle_copy;                           \
    memcpy(king_square, king_square_copy,8);        \

// make move
static inline int MakeMove(int move, int capture_flag)
{
	// quiet move
	if (capture_flag == all_moves)
	{
		// copy board state
		copy_board();

		// parse move
		int from_square = get_move_source(move);
		int to_square = get_move_target(move);
		int promoted_piece = get_move_piece(move);
		int enpass = get_move_enpassant(move);
		int double_push = get_move_pawn(move);
		int castling = get_move_castling(move);

		// move piece
		board[to_square] = board[from_square];
		board[from_square] = e;

		// pawn promotion
		if (promoted_piece)
			board[to_square] = promoted_piece;

		// enpassant capture
		if (enpass)
			!side ? (board[to_square + 16] = e) : (board[to_square - 16] = e);

		// reset enpassant square
		enpassant = no_sq;

		// double pawn push
		if (double_push)
			!side ? (enpassant = to_square + 16) : (enpassant = to_square - 16);

		// castling
		if (castling)
		{
			// switch target square
			switch (to_square) {
				// white castles king side
			case g1:
				board[f1] = board[h1];
				board[h1] = e;
				break;

				// white castles queen side
			case c1:
				board[d1] = board[a1];
				board[a1] = e;
				break;

				// black castles king side
			case g8:
				board[f8] = board[h8];
				board[h8] = e;
				break;

				// black castles queen side
			case c8:
				board[d8] = board[a8];
				board[a8] = e;
				break;
			}
		}

		// update king square
		if (board[to_square] == K || board[to_square] == k)
			king_square[side] = to_square;

		// update castling rights
		castle &= castling_rights[from_square];
		castle &= castling_rights[to_square];

		// change side
		side ^= 1;

		// take move back if king is under the check
		if (is_square_attacked(!side ? king_square[side ^ 1] : king_square[side ^ 1], side))
		{
			// restore board state
			take_back();

			// illegal move
			return 0;
		}

		else
			// legal move
			return 1;
	}
	else
	{
		// if move is a capture
		if (get_move_capture(move))
			// make capture move
			MakeMove(move, all_moves);

		else
			// move is not a capture
			return 0;
	}
}


/***********************************************\

				  PERFT FUNCTIONS

\***********************************************/

// count nodes
long nodes = 0;

// perft driver
static inline void perft_driver(int depth)
{
	// escape condition
	if (!depth)
	{
		// count current position
		nodes++;
		return;
	}

	// create move list variable
	moves move_list[1];

	// generate moves
	generate_moves(move_list);

	// loop over the generated moves
	for (int move_count = 0; move_count < move_list->count; move_count++)
	{
		// copy board state
		copy_board();

		// make only legal moves
		if (!MakeMove(move_list->moves[move_count], all_moves))
			// skip illegal move
			continue;

		// recursive call
		perft_driver(depth - 1);

		// restore board state
		take_back();
	}
}

// perft test
static inline void PerftTest(int depth)
{
	printf("\n    Performance test:\n\n");

	// init start time
	int start_time = get_time_ms();

	// create move list variable
	moves move_list[1];

	// generate moves
	generate_moves(move_list);

	// loop over the generated moves
	for (int move_count = 0; move_count < move_list->count; move_count++)
	{
		// copy board state
		copy_board();

		// make only legal moves
		if (!MakeMove(move_list->moves[move_count], all_moves))
			// skip illegal move
			continue;

		// cummulative nodes
		long cum_nodes = nodes;

		// recursive call
		perft_driver(depth - 1);

		// old nodes
		long old_nodes = nodes - cum_nodes;

		// restore board state
		take_back();

		// print current move
		printf("    move %d: %s%s%c    %ld\n",
			move_count + 1,
			square_to_coords[get_move_source(move_list->moves[move_count])],
			square_to_coords[get_move_target(move_list->moves[move_count])],
			promoted_pieces[get_move_piece(move_list->moves[move_count])],
			old_nodes
		);
	}

	// print results
	printf("\n    Depth: %d", depth);
	printf("\n    Nodes: %ld", nodes);
	printf("\n     Time: %d ms\n\n", get_time_ms() - start_time);
}


/***********************************************\

			   EVALUATION FUNCTION

\***********************************************/

// evaluation of the position
static inline int evaluate_position()
{
	// init score
	int score = 0;

	// loop over board squares
	for (int square = 0; square < 128; square++)
	{
		// make sure square is on board
		if (!(square & 0x88))
		{
			// init piece
			int piece = board[square];

			// material score evaluation
			score += material_score[piece];

			// pieces evaluation
			switch (piece)
			{
				// white pieces
			case P:
				// positional score
				score += pawn_score[square];

				// double panws penalty
				if (board[square - 16] == P)
					score -= 100;

				break;

			case N: score += knight_score[square]; break;
			case B: score += bishop_score[square]; break;
			case R: score += rook_score[square]; break;
			case K: score += king_score[square]; break;

				// black pieces
			case p:
				// positional score
				score -= pawn_score[mirror_score[square]];

				// double pawns penalty
				if (board[square + 16] == p)
					score += 100;

				break;
			case n: score -= knight_score[mirror_score[square]]; break;
			case b: score -= bishop_score[mirror_score[square]]; break;
			case r: score -= rook_score[mirror_score[square]]; break;
			case k: score -= king_score[mirror_score[square]]; break;
			}

		}
	}

	// return positive score for white & negative for black
	return !side ? score : -score;
}


/***********************************************\

				 SEARCH FUNCTIONS

\***********************************************/

// most valuable victim & less valuable attacker

/*

	(Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
		Pawn   105    205    305    405    505    605
	  Knight   104    204    304    404    504    604
	  Bishop   103    203    303    403    503    603
		Rook   102    202    302    402    502    602
	   Queen   101    201    301    401    501    601
		King   100    200    300    400    500    600

*/

static int mvv_lva[13][13] = {
	0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,
	0, 105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	0, 104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	0, 103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	0, 102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	0, 101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	0, 100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	0, 105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	0, 104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	0, 103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	0, 102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	0, 101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	0, 100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

// killer moves [id][ply]
int killer_moves[2][64];

// history moves [piece][square]
int history_moves[13][128];

// PV moves
int pv_table[64][64];
int pv_length[64];

// half move
int ply = 0;

// score move for move ordering
static inline int score_move(int move){
	if (pv_table[0][ply] == move)
		// score 20000 ( search it first )
		return 20000;

	// init current move score
	int score;

	// score MVV LVA (scores 0 for quiete moves)
	score = mvv_lva[board[get_move_source(move)]][board[get_move_target(move)]];

	// on capture
	if (get_move_capture(move))
	{
		// add 10000 to current score
		score += 10000;
	}

	// on quiete move
	else {
		// on 1st killer move
		if (killer_moves[0][ply] == move)
			// score 9000
			score = 9000;

		// on 2nd killer move
		else if (killer_moves[1][ply] == move)
			// score 8000
			score = 8000;

		// on history move (previous alpha's best score)
		else
			// score with history depth
			score = history_moves[board[get_move_source(move)]][get_move_target(move)] + 7000;
	}

	// return move score
	return score;
}

static inline void sort_moves(moves* move_list)
{
	int move_scores[0xff];

	// init move scores array
	for (int count = 0; count < move_list->count; count++)
		// score move
		move_scores[count] = score_move(move_list->moves[count]);

	// loop over current move score
	for (int current = 0; current < move_list->count; current++)
	{
		// loop over next move score
		for (int next = current + 1; next < move_list->count; next++)
		{
			// order moves descending
			if (move_scores[current] < move_scores[next])
			{
				// swap scores
				int temp_score = move_scores[current];
				move_scores[current] = move_scores[next];
				move_scores[next] = temp_score;

				// swap corresponding moves
				int temp_move = move_list->moves[current];
				move_list->moves[current] = move_list->moves[next];
				move_list->moves[next] = temp_move;
			}
		}
	}
}
#define NODE_CHECK 9000
bool stop = false;
int g_max_depth = 0;
long nodeCheck = NODE_CHECK;
clock_t start_time=0;
clock_t g_max_time = 0;

// quiescence search
static inline int SearchQuiescence(int alpha, int beta, int depth){
	if (++nodes > nodeCheck)
	{
		nodeCheck = nodes + NODE_CHECK;
		stop = clock() - start_time > g_max_time;
	}
	if (stop)
		return 0;
	int eval = evaluate_position();
	if (eval >= beta)
		return beta;
	if (eval > alpha)
		alpha = eval;

	// create move list variable
	moves move_list[1];

	// generate moves
	generate_moves(move_list);

	// move ordering
	sort_moves(move_list);

	// loop over the generated moves
	for (int count = 0; count < move_list->count; count++)
	{
		// copy board state
		copy_board();

		// increment ply
		ply++;

		// make only legal moves
		if (!MakeMove(move_list->moves[count], only_captures))
		{
			// decrement ply
			ply--;

			// skip illegal move
			continue;
		}

		// recursive call
		int score = -SearchQuiescence(-beta, -alpha, depth);

		// restore board state
		take_back();

		// decrement ply
		ply--;

		//  fail hard beta-cutoff
		if (score >= beta)
			return beta;

		// alpha acts like max in MiniMax
		if (score > alpha)
			alpha = score;
	}

	// return alpha score
	return alpha;
}

// negamax search
static inline int SearchAlpha(int alpha, int beta, int depth)
{
	int legal_moves = 0;

	// best move
	int best_so_far = 0;

	// old alpha
	int old_alpha = alpha;

	// PV length
	pv_length[ply] = ply;

	// escape condition
	if (!depth)
		// search for calm position before evaluation
		return SearchQuiescence(alpha, beta, depth);

	if (++nodes > nodeCheck)
	{
		nodeCheck = nodes + NODE_CHECK;
		stop = clock() - start_time > g_max_time;
	}
	if (stop)
		return 0;

	// is king in check?
	int in_check = is_square_attacked(king_square[side], side ^ 1);

	// increase depth if king is in check
	if (in_check)
		depth++;

	// create move list variable
	moves move_list[1];

	// generate moves
	generate_moves(move_list);

	// move ordering
	sort_moves(move_list);

	// loop over the generated moves
	for (int count = 0; count < move_list->count; count++)
	{
		// copy board state
		copy_board();

		// increment ply
		ply++;

		// make only legal moves
		if (!MakeMove(move_list->moves[count], all_moves))
		{
			// decrement ply
			ply--;

			// skip illegal move
			continue;
		}

		// increment legal moves
		legal_moves++;

		// recursive call
		int score = -SearchAlpha(-beta, -alpha, depth - 1);

		// restore board state
		take_back();

		// decrement ply
		ply--;

		//  fail hard beta-cutoff
		if (score >= beta)
		{
			// update killer moves
			killer_moves[1][ply] = killer_moves[0][ply];
			killer_moves[0][ply] = move_list->moves[count];

			return beta;
		}

		// alpha acts like max in MiniMax
		if (score > alpha)
		{
			// update history score
			history_moves[board[get_move_source(move_list->moves[count])]][get_move_target(move_list->moves[count])] += depth;

			// set alpha score
			alpha = score;

			// store PV move
			pv_table[ply][ply] = move_list->moves[count];

			for (int i = ply + 1; i < pv_length[ply + 1]; i++)
				pv_table[ply][i] = pv_table[ply + 1][i];

			pv_length[ply] = pv_length[ply + 1];

			// store current best move
			if (!ply)
				best_so_far = move_list->moves[count];
		}
	}
	if (!legal_moves)
	{
		if (in_check)
			return -MAX_SCORE + ply;
		else
			return 0;
	}
	return alpha;
}

// search position
static void SearchIterate(int depth, int time)
{
	// init nodes count
	stop = false;
	nodes = 0;
	nodeCheck = NODE_CHECK;
	start_time = clock();
	// clear PV, killer and history moves
	memset(pv_table, 0, 16384);  // sizeof(pv_table)
	memset(killer_moves, 0, 512);  // sizeof(killer_moves)
	memset(history_moves, 0, 6656);  // sizeof(history_moves)

	// best score
	int score;

	// iterative deepening
	for (int current_depth = 1; current_depth <= depth; current_depth++)
	{
		// search position with current depth 3
		score = SearchAlpha(-MAX_SCORE,MAX_SCORE, current_depth);
		if (stop)
			break;
		clock_t elapsed = clock() - start_time;
		int del = MAX_SCORE - MAX_DEPTH;
		if (score > del)
			cout << "info score mate " << (MAX_SCORE - score + 1) / 2 << " depth " << current_depth << " time " << elapsed << " nodes " << nodes << " pv ";
		else if (score < -del)
			cout << "info score mate " << (-MAX_SCORE - score) / 2 << " depth " << current_depth << " time " << elapsed << " nodes " << nodes << " pv ";
		else
			cout << "info score cp " << score << " depth " << current_depth << " time " << elapsed << " nodes " << nodes << " pv ";
		// output best move
		//printf("info score cp %d depth %d nodes %ld pv ", score, current_depth, nodes);

		// print PV line
		for (int i = 0; i < pv_length[0]; i++)
		{
			cout << square_to_coords[get_move_source(pv_table[0][i])]
				<< square_to_coords[get_move_target(pv_table[0][i])]
					<< PieceToPromotion(get_move_piece(pv_table[0][i]));
		}
		cout << endl;
		if (elapsed > time / 8)
			break;
	}

	// print best move
	cout << "bestmove "
		<< square_to_coords[get_move_source(pv_table[0][0])]
		<< square_to_coords[get_move_target(pv_table[0][0])]
			<< PieceToPromotion(get_move_piece(pv_table[0][0]))
				<< endl;
}


/***********************************************\

			  UCI PROTOCOL FUNCTIONS

\***********************************************/

// parse move (from UCI)
int ParseMove(const char* move_str)
{
	// init move list
	moves move_list[1];

	// generate moves
	generate_moves(move_list);

	// parse move string
	int parse_from = (move_str[0] - 'a') + (8 - (move_str[1] - '0')) * 16;
	int parse_to = (move_str[2] - 'a') + (8 - (move_str[3] - '0')) * 16;
	int prom_piece = 0;

	// init move to encode
	int move;

	// loop over generated moves
	for (int count = 0; count < move_list->count; count++)
	{
		// pick up move
		move = move_list->moves[count];

		// if input move is present in the move list
		if (get_move_source(move) == parse_from && get_move_target(move) == parse_to)
		{
			// init promoted piece
			prom_piece = get_move_piece(move);

			// if promoted piece is present compare it with promoted piece from user input
			if (prom_piece)
			{
				if ((prom_piece == N || prom_piece == n) && move_str[4] == 'n')
					return move;

				else if ((prom_piece == B || prom_piece == b) && move_str[4] == 'b')
					return move;

				else if ((prom_piece == R || prom_piece == r) && move_str[4] == 'r')
					return move;

				else if ((prom_piece == Q || prom_piece == q) && move_str[4] == 'q')
					return move;

				continue;
			}

			// return move to make on board
			return move;
		}
	}

	// return illegal move
	return 0;
}

static void SetFen(vector<string> fen) {
	ResetBoard();
	int sq = 0;
	string ele = fen[0];
	for (char c : ele) {
		switch (c) {
		case 'p':board[sq++] = p; break;
		case 'n':board[sq++] = n; break;
		case 'b':board[sq++] = b; break;
		case 'r':board[sq++] = r; break;
		case 'q':board[sq++] = q; break;
		case 'k':king_square[black] = sq; board[sq++] = k; break;
		case 'P':board[sq++] = P; break;
		case 'N':board[sq++] = N; break;
		case 'B':board[sq++] = B; break;
		case 'R':board[sq++] = R; break;
		case 'Q':board[sq++] = Q; break;
		case 'K':king_square[white] = sq; board[sq++] = K; break;
		case '1': sq += 1; break;
		case '2': sq += 2; break;
		case '3': sq += 3; break;
		case '4': sq += 4; break;
		case '5': sq += 5; break;
		case '6': sq += 6; break;
		case '7': sq += 7; break;
		case '8': sq += 8; break;
		case '/': sq += 8; break;
		}
	}
	ele = fen[1];
	side = (ele == "w") ? white : black;
	ele = fen[2];
	for (char c : ele)
		switch (c)
		{
		case 'K':
			castle |= KC;
			break;
		case 'Q':
			castle |= QC;
			break;
		case 'k':
			castle |= kc;
			break;
		case 'q':
			castle |= qc;
			break;
		}
	if (fen[3][0] != '-')
	{
		int file = fen[3][0] - 'a';
		int rank = 7 - (fen[3][1] - '1');
		enpassant = rank * 16 + file;
	}
}

static void SetFen(string fen) {
	vector<string> v = SplitString(fen);
	SetFen(v);
}

static void ParsePosition(vector<string> commands) {
	vector<string> fen = {};
	vector<string> moves = {};
	int mark = 0;
	for (int i = 1; i < commands.size(); i++) {
		if (mark == 1)
			fen.push_back(commands[i]);
		if (mark == 2)
			moves.push_back(commands[i]);
		if (commands[i] == "fen")
			mark = 1;
		else if (commands[i] == "moves")
			mark = 2;
	}
	if (fen.size() != 6)
		fen = SplitString(defFen);
	SetFen(fen);
	for (string m : moves)
	{
		int move = ParseMove(m.c_str());
		MakeMove(move, all_moves);
	}

}

static void UciQuit() {
	exit(0);
}

static void UciTest() {
	UciCommand("position startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 f8e7 d2d4 e5d4 f3d4 g8f6 d4c6 d7c6 d1d8 e7d8 f2f3 b7b5 a4b3 c6c5 c2c4 b5c4 b3c4 c8e6 b1a3 e8g8 c4e6 f7e6 c1e3 d8e7 a1c1 a8b8 b2b3 f6d7 e1g1 b8d8 a3c4 f8e8 e3f4 d8c8 f1d1 e8d8 c4a5 d7f6 d1d8 c8d8 a5c6 d8e8 c6e7 e8e7 c1c5 f6e8 f4e5 e7d7 c5a5 d7d1 g1f2 d1d2 f2g3 c7c5 a5a6 g8f7 a6a7 f7f8 a2a4 h7h5 a4a5 h5h4 g3h4 d2g2 a5a6");
	UciCommand("go depth 5");
}

void UciCommand(string line) {
	vector<string> commands = SplitString(line);
	fflush(stdout);
	if (commands[0] == "uci") {
		cout << "id name " << name << endl;
		cout << "uciok" << endl;
	}
	else if (commands[0] == "isready") {
		cout << "readyok" << endl;
	}
	else if (commands[0] == "position")
		ParsePosition(commands);
	else if (commands[0] == "go") {
		g_max_depth = GetInt(commands, "depth", 0xff);
		g_max_time = GetInt(commands, side == white ? "wtime" : "btime", 0) / 30;
		if (!g_max_time)
			g_max_time = GetInt(commands, "movetime", 0xffffff);
		SearchIterate(g_max_depth, g_max_time);
	}
	else if (commands[0] == "print")
		PrintBoard();
	else if (commands[0] == "test")
		UciTest();
	else if (commands[0] == "quit")
		UciQuit();
}

static void UciLoop() {
	string line;
	while (true) {
		getline(cin, line);
		UciCommand(line);
	}
}

// main driver
int main() {
	PrintWelcome();
	UciLoop();
	return 0;
}