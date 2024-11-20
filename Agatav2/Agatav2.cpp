#include <iostream>
#include <unordered_map>
#include <chrono>
#include <winsock2.h>
#include "sock.h"


// define bitboard data type
#define U64 unsigned long long

// macro to use faster builtin c++ bitcount system
#define count_bits(bitboard) __popcnt64(bitboard)

// FEN debug positions
/*
Una stringa FEN definisce una particolare posizione nel corso di una partita, descrivendola interamente in una linea di testo, ed utilizzando solo i caratteri ASCII. 
Un file di testo che contenga solo stringhe FEN dovrebbe recare l'estensione ".fen".

Una stringa è composta di 6 campi, a loro volta separati da uno spazio. Usualmente è indicata tra parentesi quadre. I campi sono:

1. Posizione dei pezzi: viene descritta ogni singola traversa, a partire dalla ottava fino alla prima; per ciascuna traversa, sono descritti i contenuti di ciascuna casa, 
a partire dalla colonna "a" ed a finire con quella "h". I pezzi del Bianco sono indicati usando le iniziali (inglesi) dei pezzi stessi in maiuscolo ("KQRBNP"); quelli del Nero, 
con le stesse lettere, ma in minuscolo ("kqrbnp") ad eccezione del cavallo 'knight' che viene indicato con la lettera N o n. Il numero di case vuote tra un pezzo e l'altro o dai 
bordi della scacchiera è indicato con le cifre da 1 a 8. Se una traversa non contiene pezzi o pedoni, sarà descritta con un 8. Il segno " / " è usato per separare le traverse una dall'altra.
2. Giocatore che ha la mossa: "w" indica che la mossa è al Bianco, "b" che tocca al Nero.
3. Possibilità di arroccare: se nessuno dei due giocatori può arroccare, si indica "-". Altrimenti, possono essere utilizzare una o più lettere: "K" (Il Bianco può arroccare corto), 
"Q" (Il Bianco può arroccare lungo), "k" (il Nero può arroccare corto), e/o "q" (il Nero può arroccare lungo).
4. Possibilità di catturare en passant: se non è possibile effettuare alcuna cattura en passant, si indica "-". Se un pedone ha appena effettuato la sua prima mossa di due case e c'è 
la possibilità di catturarlo en passant, verrà indicata la casa "alle spalle" del pedone stesso.
5. Numero delle semimosse: il numero di semimosse dall'ultima spinta di pedone o dall'ultima cattura. È usato per determinare se si può chiedere patta per la regola delle cinquanta mosse.
6. Numero di mosse: il numero complessivo di mosse della partita. Inizia da 1, ed aumenta di una unità dopo ogni mossa del Nero.
*/
char empty_board[] = "8/8/8/8/8/8/8/8 b - - ";
char start_position[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
char tricky_position[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 ";
char killer_position[] = "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1";
char cmk_position[] = "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 ";


/**********************************\
 ==================================

            Chess board

 ==================================
\**********************************/
// board squares
enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};
const char* square_to_coordinates[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "no_sq"
};

// encode pieces
enum { P, N, B, R, Q, K, p, n, b, r, q, k };

// colors
enum { white, black, both };

//for magics
enum { rook, bishop };

//castling rights binary encoding
/*

    bin  dec

   0001    1  white king can castle to the king side
   0010    2  white king can castle to the queen side
   0100    4  black king can castle to the king side
   1000    8  black king can castle to the queen side

   examples

   1111       both sides an castle both directions
   1001       black king => queen side
              white king => king side

*/
enum { wk = 1, wq = 2, bk = 4, bq = 8 };

//ASCII pieces
char ascii_pieces[] = "PNBRQKpnbrqk";

//convert ASCII character pieces to encoded constants
std::unordered_map<char, int> char_pieces = {
    {'P', P},
    {'N', N},
    {'B', B},
    {'R', R},
    {'Q', Q},
    {'K', K},
    {'p', p},
    {'n', n},
    {'b', b},
    {'r', r},
    {'q', q},
    {'k', k}
};

// promoted pieces
std::unordered_map<char, char> promoted_pieces = {
    {Q, 'q'},
    {R, 'r'},
    {B, 'b'},
    {N, 'n'},
    {q, 'q'},
    {r, 'r'},
    {b, 'b'},
    {n, 'n'}
};

//piece bitboards
U64 bitboards[12];

//occupancy bitboards
U64 occupancies[3];

//side to move
int side;

//enpassant square
int enpassant = no_sq;

//castling rights
int castle;

/**********************************\
 ==================================

          Bit manipulations

 ==================================
\**********************************/

//macros
#define setSquare(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define getSquare(bitboard, square) ((bitboard) & (1ULL << (square)))
#define popSquare(bitboard, square) ((bitboard) &= ~(1ULL << (square)))
bool isOccupied(U64 bitboard, int square) {
    return bitboard & (1ULL << square);  // Controlla se il bit è 1
}

//least significant bit index
static inline int get_ls1b_index(unsigned long long bitboard){

    if (bitboard) return count_bits((bitboard & (~bitboard + 1)) - 1);
   
    return -1;
}

/**********************************\
 ==================================

           Random numbers

 ==================================
\**********************************/

unsigned int random_state = 1804289383; //to ensure every time grn funct is used a new num get generated

//generate 32-bit pseudo legal numbers
unsigned int get_random_U32_number(){
    unsigned int number = random_state;

    // XOR shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;
    random_state = number;

    return number;
}

// generate 64-bit pseudo legal numbers
U64 get_random_U64_number(){
    U64 n1, n2, n3, n4;

    n1 = (U64)(get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(get_random_U32_number()) & 0xFFFF;
    n3 = (U64)(get_random_U32_number()) & 0xFFFF;
    n4 = (U64)(get_random_U32_number()) & 0xFFFF;

    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

// generate magic number candidate
U64 generate_magic_number(){
    return get_random_U64_number() & get_random_U64_number() & get_random_U64_number();
}

/**********************************\
 ==================================

              Utility

 ==================================
\**********************************/

//printing
void printBitboard(U64 bitboard) {
    std::cout << "\n\n\n";
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;

            if (file == 0) std::cout << 8 - rank << "  ";

            std::cout << (isOccupied(bitboard, square) ? "1 " : ". ");
        }
        std::cout << "\n";
    }
    std::cout << "\n   a b c d e f g h\n";
}
void print_board() {
    std::cout << "\n";

    // Loop over board ranks
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;

            if (file == 0) {
                std::cout << "  " << (8 - rank) << " ";
            }

            int piece = -1;

            for (int bb_piece = P; bb_piece <= k; bb_piece++) {
                if (getSquare(bitboards[bb_piece], square)) {
                    piece = bb_piece;
                }
            }
            std::cout << " " << (piece == -1 ? '.' : ascii_pieces[piece]);
            // Uncomment the following line for Unicode representation instead
            // std::cout << " " << (piece == -1 ? "." : unicode_pieces[piece]);
        }
        std::cout << "\n";
    }
    std::cout << "\n     a b c d e f g h\n\n";

    // Print side to move
    std::cout << "     Side:     " << (side == 0 ? "white" : "black") << "\n";

    // Print en passant square
    std::cout << "     Enpassant:   " << (enpassant != no_sq ? square_to_coordinates[enpassant] : "no") << "\n";

    // Print castling rights
    std::cout << "     Castling:  "
        << ((castle & wk) ? 'K' : '-')
        << ((castle & wq) ? 'Q' : '-')
        << ((castle & bk) ? 'k' : '-')
        << ((castle & bq) ? 'q' : '-')
        << "\n\n";
}

//parsing FEN
void parse_fen(char* fen){
    //reset board position (bitboards)
    memset(bitboards, 0ULL, sizeof(bitboards));
    //reset occupancies (bitboards)
    memset(occupancies, 0ULL, sizeof(occupancies));

    //reset game state
    side = 0;
    enpassant = no_sq;
    castle = 0;

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++){
        for (int file = 0; file < 8; file++){
            int square = rank * 8 + file;

            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')){
                int piece = char_pieces[*fen];
                setSquare(bitboards[piece], square);
                *fen++;
            }

            //match empty square numbers within FEN string
            if (*fen >= '0' && *fen <= '9'){
                int offset = *fen - '0';
                int piece = -1;

                for (int bb_piece = P; bb_piece <= k; bb_piece++){
                    if (getSquare(bitboards[bb_piece], square)) piece = bb_piece;
                }

                if (piece == -1) file--;

                file += offset;

                *fen++;
            }

            //match rank separator
            if (*fen == '/') *fen++;
        }
    }

    //go to parsing side to move (increment pointer to FEN string)
    *fen++;

    //parse side to move
    (*fen == 'w') ? (side = white) : (side = black);

    //go to parsing castling rights
    fen += 2;

    //parse castling rights
    while (*fen != ' '){
        switch (*fen){
        case 'K': castle |= wk; break;
        case 'Q': castle |= wq; break;
        case 'k': castle |= bk; break;
        case 'q': castle |= bq; break;
        case '-': break;
        }
        *fen++;
    }

    //got to parsing enpassant square (increment pointer to FEN string)
    *fen++;

    //parse enpassant square
    if (*fen != '-'){
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');

        //enpassant square
        enpassant = rank * 8 + file;

    } else enpassant = no_sq;

    //loop over white pieces bitboards
    for (int piece = P; piece <= K; piece++)  occupancies[white] |= bitboards[piece];

    //loop over black pieces bitboards
    for (int piece = p; piece <= k; piece++) occupancies[black] |= bitboards[piece];

    //init all occupancies
    occupancies[both] |= occupancies[white];
    occupancies[both] |= occupancies[black];
}

/**********************************\
 ==================================

              Attacks

 ==================================
\**********************************/

//consts
const U64 not_a_file = 18374403900871474942ULL;
const U64 not_h_file = 9187201950435737471ULL;
const U64 not_hg_file = 4557430888798830399ULL;
const U64 not_ab_file = 18229723555195321596ULL;

//bishop relevant occupancy
const int bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

//rook relevant occupancy
const int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

//pawns
U64 pawn_attacks[2][64];
U64 mask_pawn_attacks(int square, int side) {

    U64 bitboard = 0ULL;
    U64 attacks = 0ULL;

    setSquare(bitboard, square);

    //white
    if (side == white) {
        if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    }

    //black
    else {
        if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
        if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    }

    return attacks;


    return 0;
}

//knights
U64 knight_attacks[64];
U64 mask_knight_attacks(int square) {

    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;

    setSquare(bitboard, square);

    if ((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & not_hg_file) attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);
    if ((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
    if ((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
    if ((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
    if ((bitboard << 6) & not_hg_file) attacks |= (bitboard << 6);

    return attacks;
}

//king
U64 king_attacks[64];
U64 mask_king_attacks(int square) {

    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;

    setSquare(bitboard, square);

    if (bitboard >> 8) attacks |= (bitboard >> 8);
    if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
    if ((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);
    if (bitboard << 8) attacks |= (bitboard << 8);
    if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
    if ((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);

    return attacks;
}

//bishop masks
U64 bishop_masks[64];
U64 mask_bishop_attacks(int square) {
    //no borders

    U64 attacks = 0ULL;

    int r, f;

    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f));
    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f));

    return attacks;
}
U64 bishop_attacks_on_the_fly(int square, U64 block){
    
    U64 attacks = 0ULL;

    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++){
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }

    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++){
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }

    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--){
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }

    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--){
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }

    return attacks;
}

//rook masks
U64 rook_masks[64];
U64 mask_rook_attacks(int square) {
    //no borders

    U64 attacks = 0ULL;

    int r, f;

    //target
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf));
    for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf));
    for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f));
    for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f));

    return attacks;
}
U64 rook_attacks_on_the_fly(int square, U64 block){
    
    U64 attacks = 0ULL;

    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1; r <= 7; r++){
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block) break;
    }

    for (r = tr - 1; r >= 0; r--){
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block) break;
    }

    for (f = tf + 1; f <= 7; f++){
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block) break;
    }

    for (f = tf - 1; f >= 0; f--){
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block) break;
    }

    return attacks;
}

//occupancies
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask){
    U64 occupancy = 0ULL;

    for (int count = 0; count < bits_in_mask; count++){

        int square = get_ls1b_index(attack_mask);

        //pop LS1B  attack map
        popSquare(attack_mask, square);

        if (index & (1 << count)) occupancy |= (1ULL << square);
    }

    return occupancy;
}

//bishop attacks && rook attacks
U64 bishop_attacks[64][512];
U64 rook_attacks[64][4096];

//attacks initializer
void init_leapers_attacks() {
    for (int sq = 0; sq < 64; sq++) {
        //pawns
        pawn_attacks[white][sq] = mask_pawn_attacks(sq, white);
        pawn_attacks[black][sq] = mask_pawn_attacks(sq, black);

        //knight
        knight_attacks[sq] = mask_knight_attacks(sq);

        //king
        king_attacks[sq] = mask_king_attacks(sq);
    }
}

/**********************************\
 ==================================

           Magic Numbers

 ==================================
\**********************************/

//magics already generated
U64 rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL
};
U64 bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL
};

//USED TO GENERATE MAGICS
//find appropriate mnums
U64 find_magic_number(int square, int relevant_bits, int bishop){
    U64 occupancies[4096];
    U64 attacks[4096];
    U64 used_attacks[4096];

    // init attack mask for a current piece
    U64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);

    int occupancy_indicies = 1 << relevant_bits;

    // loop over occupancy indicies
    for (int index = 0; index < occupancy_indicies; index++){
        occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);

        // init attacks
        attacks[index] = bishop ? bishop_attacks_on_the_fly(square, occupancies[index]) : rook_attacks_on_the_fly(square, occupancies[index]);
    }

    // test magic numbers loop
    for (int random_count = 0; random_count < 100000000; random_count++){
        // generate magic number candidate
        U64 magic_number = generate_magic_number();

        // skip inappropriate magic numbers
        if (count_bits((attack_mask * magic_number) & 0xFF00000000000000) < 6) continue;

        // init used attacks
        memset(used_attacks, 0ULL, sizeof(used_attacks));

        // init index & fail flag
        int index, fail;

        // test magic index loop
        for (index = 0, fail = 0; !fail && index < occupancy_indicies; index++)
        {
            // init magic index
            int magic_index = (int)((occupancies[index] * magic_number) >> (64 - relevant_bits));

            // if magic index works
            if (used_attacks[magic_index] == 0ULL)
                // init used attacks
                used_attacks[magic_index] = attacks[index];

            // otherwise
            else if (used_attacks[magic_index] != attacks[index])
                // magic index doesn't work
                fail = 1;
        }

        // if magic number works return it
        if (!fail) return magic_number;
    }

    // if magic number doesn't work
    printf("  Magic number fails!\n");
    return 0ULL;
}
// init magic numbers
void init_magic_numbers(){
    // loop over 64 board squares
    for (int square = 0; square < 64; square++) {
        // init rook magic numbers
        rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], rook);
    }
        

    // loop over 64 board squares
    for (int square = 0; square < 64; square++) {
        // init bishop magic numbers
        bishop_magic_numbers[square] = find_magic_number(square, bishop_relevant_bits[square], bishop);
    }
        
}

//init slider pieces attacks table
void init_sliders_attacks(int bishop) {
    for (int square = 0; square < 64; square++){
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        // init current mask
        U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
        int relevant_bits_count = count_bits(attack_mask);

        // init occupancy indicies
        int occupancy_indicies = (1 << relevant_bits_count);

        // loop over occupancy indicies
        for (int index = 0; index < occupancy_indicies; index++){
            if (bishop){
                // init current occupancy variation
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);
                bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
            }
            else{
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);
                rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);

            }
        }
    }
}

// get bishop attacks
static inline U64 get_bishop_attacks(int square, U64 occupancy){
    // get bishop attacks assuming current board occupancy
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_numbers[square];
    occupancy >>= 64 - bishop_relevant_bits[square];

    // return bishop attacks
    return bishop_attacks[square][occupancy];
}

// get rook attacks
static inline U64 get_rook_attacks(int square, U64 occupancy){
    // get bishop attacks assuming current board occupancy
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_numbers[square];
    occupancy >>= 64 - rook_relevant_bits[square];

    // return rook attacks
    return rook_attacks[square][occupancy];
}

// get queen attacks
static inline U64 get_queen_attacks(int square, U64 occupancy) {
    return (get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy));
}

/**********************************\
 ==================================

             MoveList

 ==================================
\**********************************/

//encode and decode moves to keep track of status of the board
/*
          binary move bits                               hexidecimal constants

    0000 0000 0000 0000 0011 1111    source square       0x3f
    0000 0000 0000 1111 1100 0000    target square       0xfc0
    0000 0000 1111 0000 0000 0000    piece               0xf000
    0000 1111 0000 0000 0000 0000    promoted piece      0xf0000
    0001 0000 0000 0000 0000 0000    capture flag        0x100000
    0010 0000 0000 0000 0000 0000    double push flag    0x200000
    0100 0000 0000 0000 0000 0000    enpassant flag      0x400000
    1000 0000 0000 0000 0000 0000    castling flag       0x800000
*/

//ENCODING MACROS
//encode move
#define encode_move(source, target, piece, promoted, capture, double, enpassant, castling) \
    (source) |          \
    (target << 6) |     \
    (piece << 12) |     \
    (promoted << 16) |  \
    (capture << 20) |   \
    (double << 21) |    \
    (enpassant << 22) | \
    (castling << 23)    \
//extract source square
#define get_move_source(move) (move & 0x3f)
//extract target square
#define get_move_target(move) ((move & 0xfc0) >> 6)
//extract piece
#define get_move_piece(move) ((move & 0xf000) >> 12)
//extract promoted piece
#define get_move_promoted(move) ((move & 0xf0000) >> 16)
//extract capture flag
#define get_move_capture(move) (move & 0x100000)
//extract double pawn push flag
#define get_move_double(move) (move & 0x200000)
//extract enpassant flag
#define get_move_enpassant(move) (move & 0x400000)
//extract castling flag
#define get_move_castling(move) (move & 0x800000)

//MOVES
typedef struct {
    int moves[256];
    int count;
} moves;

static inline void add_move(moves* move_list, int move) {
    move_list->moves[move_list->count] = move;
    move_list->count++;
}

//debug
void print_move(int move) {
    std::cout << square_to_coordinates[get_move_source(move)]
        << square_to_coordinates[get_move_target(move)]
        << promoted_pieces[get_move_promoted(move)];
}
void print_move_list(moves* move_list) {
    std::cout << "\n    move    piece   capture   double    enpass    castling\n\n";
    for (int move_count = 0; move_count < move_list->count; move_count++) {
        int move = move_list->moves[move_count];
        std::cout << "    "
            << square_to_coordinates[get_move_source(move)]
            << square_to_coordinates[get_move_target(move)]
            << promoted_pieces[get_move_promoted(move)] << "   "
            << ascii_pieces[get_move_piece(move)] << "       "
            << (get_move_capture(move) ? 1 : 0) << "         "
            << (get_move_double(move) ? 1 : 0) << "         "
            << (get_move_enpassant(move) ? 1 : 0) << "         "
            << (get_move_castling(move) ? 1 : 0) << '\n';
    }
    std::cout << "\n\n    Total number of moves: " << move_list->count << "\n\n";
}


/**********************************\
 ==================================

          Move generator

 ==================================
\**********************************/

//macros
//save board state
#define copy_board()                                                      \
    U64 bitboards_copy[12], occupancies_copy[3];                          \
    int side_copy, enpassant_copy, castle_copy;                           \
    memcpy(bitboards_copy, bitboards, 96);                                \
    memcpy(occupancies_copy, occupancies, 24);                            \
    side_copy = side, enpassant_copy = enpassant, castle_copy = castle;   \
//restore board state
#define take_back()                                                       \
    memcpy(bitboards, bitboards_copy, 96);                                \
    memcpy(occupancies, occupancies_copy, 24);                            \
    side = side_copy, enpassant = enpassant_copy, castle = castle_copy;   \

//attacked squares
static inline int is_square_attacked(int square, int side) {
    //attacked by white pawns
    if ((side == white) && (pawn_attacks[black][square] & bitboards[P])) return 1;

    //attacked by black pawns
    if ((side == black) && (pawn_attacks[white][square] & bitboards[p])) return 1;

    //attacked by knights
    if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return 1;

    //attacked by bishops
    if (get_bishop_attacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;

    //attacked by rooks
    if (get_rook_attacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;

    //attacked by bishops
    if (get_queen_attacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;

    //attacked by kings
    if (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return 1;

    //by default return false
    return 0;
}
void print_attacked_squares(int side) {
    std::cout << "\n";
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            if (!file) std::cout << "  " << 8 - rank << "  ";
            std::cout << (is_square_attacked(square, side) ? "1 " : ". ");
        }
        std::cout << "\n";
    }
    std::cout << "\n     a b c d e f g h\n\n";
}

//moves types
enum {all_moves, only_captures};

/*
                           castling   move     in      in
                              right update     binary  decimal

 king & rooks didn't move:     1111 & 1111  =  1111    15

        white king  moved:     1111 & 1100  =  1100    12
  white king's rook moved:     1111 & 1110  =  1110    14
 white queen's rook moved:     1111 & 1101  =  1101    13

         black king moved:     1111 & 0011  =  1011    3
  black king's rook moved:     1111 & 1011  =  1011    11
 black queen's rook moved:     1111 & 0111  =  0111    7

*/
// castling rights update constants
const int castling_rights[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

//actual make move function: 1 = move made correctly, 0 = not a legal move
static inline int make_move(int move, int move_flag){
    //quite moves
    if (move_flag == all_moves){
        //save board
        copy_board();

        //parse move
        int source_square = get_move_source(move);
        int target_square = get_move_target(move);
        int piece = get_move_piece(move);
        int promoted = get_move_promoted(move);
        int promoted_piece = get_move_promoted(move);
        int capture = get_move_capture(move);
        int double_push = get_move_double(move);
        int enpass = get_move_enpassant(move);
        int castling = get_move_castling(move);

        //move piece
        popSquare(bitboards[piece], source_square);
        setSquare(bitboards[piece], target_square);

        //handle capture
        if (capture){
            int start_piece, end_piece;

            //white to move
            if (side == white){
                start_piece = p;
                end_piece = k;
            }

            //black to move
            else{
                start_piece = P;
                end_piece = K;
            }

            //loop over bitboards opposite to the current side to move
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++){
                //if there's a piece on the target square
                if (getSquare(bitboards[bb_piece], target_square))
                {
                    //remove it from corresponding bitboard
                    popSquare(bitboards[bb_piece], target_square);
                    break;
                }
            }
        }

        //handle pawn promotions
        if (promoted) {
            //remove the pawn
            popSquare(bitboards[(side == white) ? P : p], target_square);

            //put promoted piece on the board
            setSquare(bitboards[promoted_piece], target_square);
        }

        //handle enpassant
        if (enpass) {
            (side == white) ? popSquare(bitboards[p], target_square + 8) : popSquare(bitboards[P], target_square - 8);
        }
        //reset enpassant because you can do it only the move after
        enpassant = no_sq;

        //handle double pawn push to set the enpassant sq
        if (double_push){
            (side == white) ? (enpassant = target_square + 8) : (enpassant = target_square - 8);
        }

        // handle castling moves (hardcoded)
        if (castling) {
            switch (target_square){
                    // white castles king side
                case (g1):
                    popSquare(bitboards[R], h1);
                    setSquare(bitboards[R], f1);
                    break;

                    // white castles queen side
                case (c1):
                    popSquare(bitboards[R], a1);
                    setSquare(bitboards[R], d1);
                    break;

                    // black castles king side
                case (g8):
                    popSquare(bitboards[r], h8);
                    setSquare(bitboards[r], f8);
                    break;

                    // black castles queen side
                case (c8):
                    popSquare(bitboards[r], a8);
                    setSquare(bitboards[r], d8);
                    break;
            }
        }

        //update castling rights
        castle &= castling_rights[source_square];
        castle &= castling_rights[target_square];

        //update castling rights
        castle &= castling_rights[source_square];
        castle &= castling_rights[target_square];

        //next lines reset the bitboards and reupdate them to the copied occupancies modified. It can be improved
        //reset occupancies
        memset(occupancies, 0ULL, 24);
        //loop over white pieces bitboards
        for (int bb_piece = P; bb_piece <= K; bb_piece++) occupancies[white] |= bitboards[bb_piece];
        //loop over black pieces bitboards
        for (int bb_piece = p; bb_piece <= k; bb_piece++) occupancies[black] |= bitboards[bb_piece];
        //update both sides occupancies
        occupancies[both] |= occupancies[white];
        occupancies[both] |= occupancies[black];

        //change side
        side ^= 1;

        //make sure that king is not in check
        if (is_square_attacked((side == white) ? get_ls1b_index(bitboards[k]) : get_ls1b_index(bitboards[K]), side)){
            take_back();
            //return illegal move
            return 0;
        }

        return 1;
    }
    

    //capture moves
    else{
        // make sure move is the capture
        if (get_move_capture(move)) make_move(move, all_moves);

        // otherwise the move is not a capture
        else
            // don't make it
            return 0;
    }
}

static inline void generate_moves(moves* move_list) {

    // init move count
    move_list->count = 0;

    int source_square, target_square;
    U64 bitboard, attacks;

    // loop over all the bitboards
    for (int piece = P; piece <= k; piece++) {
        bitboard = bitboards[piece];

        // generate white pawns & white king castling moves
        if (side == white) {
            // pick up white pawn bitboards index
            if (piece == P) {
                while (bitboard) {
                    //source square
                    source_square = get_ls1b_index(bitboard);
                    //target square
                    target_square = source_square - 8;

                    //generate quite pawn moves
                    if (!(target_square < a8) && !getSquare(occupancies[both], target_square)) {
                        //pawn promotion
                        if (source_square >= a7 && source_square <= h7) {
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
                        }

                        else {
                            //one square ahead pawn move
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            //two squares ahead pawn move
                            if ((source_square >= a2 && source_square <= h2) && !getSquare(occupancies[both], target_square - 8))
                                add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    //init pawn attacks bitboard
                    attacks = pawn_attacks[side][source_square] & occupancies[black];

                    //generate pawn captures
                    while (attacks) {
                        //init target square
                        target_square = get_ls1b_index(attacks);

                        //pawn promotion
                        if (source_square >= a7 && source_square <= h7) {
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
                        }

                        else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                        popSquare(attacks, target_square);
                    }

                    //generate enpassant captures
                    if (enpassant != no_sq) {
                        //lookup pawn attacks and bitwise AND with enpassant square (bit)
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

                        //make sure enpassant capture available
                        if (enpassant_attacks) {
                            // init enpassant capture target square
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop ls1b from piece bitboard copy
                    popSquare(bitboard, source_square);
                }
            }

            //castling
            if (piece == K) {
                //king side castling is available
                if (castle & wk) {
                    //make sure square between king and king's rook are empty
                    if (!getSquare(occupancies[both], f1) && !getSquare(occupancies[both], g1)) {
                        //make sure king and the f1 squares are not under attacks
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black)) add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                    }
                }

                //queen side castling is available
                if (castle & wq) {
                    //make sure square between king and queen's rook are empty
                    if (!getSquare(occupancies[both], d1) && !getSquare(occupancies[both], c1) && !getSquare(occupancies[both], b1)) {
                        //make sure king and the d1 squares are not under attacks
                        if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black)) add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        // generate black pawns & black king castling moves (same white reasoning)
        else {
            if (piece == p) {
                while (bitboard) {
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square + 8;
                    if (!(target_square > h1) && !getSquare(occupancies[both], target_square)) {
                        if (source_square >= a2 && source_square <= h2) {
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }
                        else {
                            //one square ahead pawn move
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead pawn move
                            if ((source_square >= a7 && source_square <= h7) && !getSquare(occupancies[both], target_square + 8))
                                add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                        }
                    }
                    attacks = pawn_attacks[side][source_square] & occupancies[white];
                    while (attacks) {
                        target_square = get_ls1b_index(attacks);

                        if (source_square >= a2 && source_square <= h2) {
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        }

                        else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        popSquare(attacks, target_square);
                    }
                    if (enpassant != no_sq) {
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        if (enpassant_attacks) {
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    popSquare(bitboard, source_square);
                }
            }
            if (piece == k) {
                if (castle & bk) {
                    if (!getSquare(occupancies[both], f8) && !getSquare(occupancies[both], g8)) {
                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white)) add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                    }
                }
                if (castle & bq) {
                    if (!getSquare(occupancies[both], d8) && !getSquare(occupancies[both], c8) && !getSquare(occupancies[both], b8)) {
                        if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white)) add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        // genarate knight moves
        if ((side == white) ? piece == N : piece == n) {
            //loop over source squares of piece bitboard copy
            while (bitboard) {
                //init source square
                source_square = get_ls1b_index(bitboard);

                //init piece attacks to get set of target squares
                attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                //loop over target squares available from generated attacks
                while (attacks) {
                    //init target square
                    target_square = get_ls1b_index(attacks);

                    //quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    //capture move
                    else  add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    popSquare(attacks, target_square);
                }
                //pop ls1b of the current piece bitboard copy
                popSquare(bitboard, source_square);
            }
        }

        // generate bishop moves
        if ((side == white) ? piece == B : piece == b) {
            //loop over source squares of piece bitboard copy
            while (bitboard) {
                //init source square
                source_square = get_ls1b_index(bitboard);

                //init piece attacks in order to get set of target squares
                attacks = get_bishop_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                //loop over target squares available from generated attacks
                while (attacks) {
                    target_square = get_ls1b_index(attacks);

                    //quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    //capture move
                    else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    popSquare(attacks, target_square);
                }
                //pop ls1b of the current piece bitboard copy
                popSquare(bitboard, source_square);
            }
        }

        // generate rook moves
        if ((side == white) ? piece == R : piece == r) {
            while (bitboard) {
                source_square = get_ls1b_index(bitboard);
                attacks = get_rook_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks) {
                    target_square = get_ls1b_index(attacks);
                    //quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    //capture
                    else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    popSquare(attacks, target_square);
                }
                popSquare(bitboard, source_square);
            }
        }

        // generate queen moves
        if ((side == white) ? piece == Q : piece == q) {
            while (bitboard) {
                source_square = get_ls1b_index(bitboard);
                attacks = get_queen_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks) {
                    target_square = get_ls1b_index(attacks);
                    // quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    // capture
                    else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    popSquare(attacks, target_square);
                }
                popSquare(bitboard, source_square);
            }
        }

        // generate king moves
        if ((side == white) ? piece == K : piece == k) {
            while (bitboard) {
                source_square = get_ls1b_index(bitboard);
                attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks) {
                    target_square = get_ls1b_index(attacks);
                    //quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    //capture
                    else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    popSquare(attacks, target_square);
                }
                popSquare(bitboard, source_square);
            }
        }
    }
}

// DOES NOT ADD TO MOVELIST IT IS JUST USEFOUL FOR DEBUGGING 
//gen all moves
static inline void print_generate_moves() {
    int source_square, target_square;
    U64 bitboard, attacks;

    // loop over all the bitboards
    for (int piece = P; piece <= k; piece++){
        bitboard = bitboards[piece];

        // generate white pawns & white king castling moves
        if (side == white){
            // pick up white pawn bitboards index
            if (piece == P){
                while (bitboard){
                    //source square
                    source_square = get_ls1b_index(bitboard);
                    //target square
                    target_square = source_square - 8;

                    //generate quite pawn moves
                    if (!(target_square < a8) && !getSquare(occupancies[both], target_square)){
                        //pawn promotion
                        if (source_square >= a7 && source_square <= h7){
                            std::cout << "pawn promotion: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "q" << std::endl;
                            std::cout << "pawn promotion: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "r" << std::endl;
                            std::cout << "pawn promotion: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "b" << std::endl;
                            std::cout << "pawn promotion: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "n" << std::endl;
                        }

                        else{
                           //one square ahead pawn move
                           std::cout << "pawn push: "<< square_to_coordinates[source_square]
                               << square_to_coordinates[target_square] << "\n";

                            //two squares ahead pawn move
                            if ((source_square >= a2 && source_square <= h2) && !getSquare(occupancies[both], target_square - 8))
                                std::cout << "double pawn push: " << square_to_coordinates[source_square]
                                << square_to_coordinates[target_square - 8] << "\n";
                        }
                    }

                    //init pawn attacks bitboard
                    attacks = pawn_attacks[side][source_square] & occupancies[black];

                    //generate pawn captures
                    while (attacks){
                        //init target square
                        target_square = get_ls1b_index(attacks);

                        //pawn promotion
                        if (source_square >= a7 && source_square <= h7){
                            std::cout << "pawn promotion capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "q" << std::endl;
                            std::cout << "pawn promotion capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "r" << std::endl;
                            std::cout << "pawn promotion capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "b" << std::endl;
                            std::cout << "pawn promotion capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "n" << std::endl;
                        }

                        else std::cout << "pawn capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << std::endl;

                        popSquare(attacks, target_square);
                    }

                    //generate enpassant captures
                    if (enpassant != no_sq){
                        //lookup pawn attacks and bitwise AND with enpassant square (bit)
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

                        //make sure enpassant capture available
                        if (enpassant_attacks){
                            // init enpassant capture target square
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            std::cout << "pawn enpassant capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_enpassant] << std::endl;
                        }
                    }

                    // pop ls1b from piece bitboard copy
                    popSquare(bitboard, source_square);
                }
            }

            //castling
            if (piece == K){
                //king side castling is available
                if (castle & wk){
                    //make sure square between king and king's rook are empty
                    if (!getSquare(occupancies[both], f1) && !getSquare(occupancies[both], g1)){
                        //make sure king and the f1 squares are not under attacks
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black)) std::cout << "castling move: e1g1\n";
                    }
                }

                //queen side castling is available
                if (castle & wq){
                    //make sure square between king and queen's rook are empty
                    if (!getSquare(occupancies[both], d1) && !getSquare(occupancies[both], c1) && !getSquare(occupancies[both], b1)){
                        //make sure king and the d1 squares are not under attacks
                        if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black)) std::cout << "castling move: e1c1\n";
                    }
                }
            }
        }

        // generate black pawns & black king castling moves (same white reasoning)
        else{
            if (piece == p){
                while (bitboard){
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square + 8;
                    if (!(target_square > h1) && !getSquare(occupancies[both], target_square)){
                        if (source_square >= a2 && source_square <= h2){
                            std::cout << "pawn promotion: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "q" << std::endl;
                            std::cout << "pawn promotion: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "r" << std::endl;
                            std::cout << "pawn promotion: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "b" << std::endl;
                            std::cout << "pawn promotion: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "n" << std::endl;
                        }
                        else{
                            //one square ahead pawn move
                            std::cout << "pawn push: " << square_to_coordinates[source_square]
                                << square_to_coordinates[target_square] << "\n";

                            // two squares ahead pawn move
                            if ((source_square >= a7 && source_square <= h7) && !getSquare(occupancies[both], target_square + 8))
                                std::cout << "double pawn push:" << square_to_coordinates[source_square]
                                << square_to_coordinates[target_square + 8] << "\n";
                        }
                    }
                    attacks = pawn_attacks[side][source_square] & occupancies[white];
                    while (attacks){
                        target_square = get_ls1b_index(attacks);

                        if (source_square >= a2 && source_square <= h2){
                            std::cout << "pawn promotion capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "q" << std::endl;
                            std::cout << "pawn promotion capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "r" << std::endl;
                            std::cout << "pawn promotion capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "b" << std::endl;
                            std::cout << "pawn promotion capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "n" << std::endl;
                        }

                        else std::cout << "pawn capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_square] << std::endl;
                        popSquare(attacks, target_square);
                    }
                    if (enpassant != no_sq){
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        if (enpassant_attacks){
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            std::cout << "pawn enpassant capture: " << square_to_coordinates[source_square] << square_to_coordinates[target_enpassant] << std::endl;
                        }
                    }
                    popSquare(bitboard, source_square);
                }
            }
            if (piece == k) {
                if (castle & bk) {
                    if (!getSquare(occupancies[both], f8) && !getSquare(occupancies[both], g8)) {
                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white)) std::cout << "castling move: e8g8\n";
                    }
                }
                if (castle & bq) {
                    if (!getSquare(occupancies[both], d8) && !getSquare(occupancies[both], c8) && !getSquare(occupancies[both], b8)) {
                        if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white)) std::cout << "castling move: e8c8\n";
                    }
                }
            }
        }

        // genarate knight moves
        if ((side == white) ? piece == N : piece == n){
            //loop over source squares of piece bitboard copy
            while (bitboard){
                //init source square
                source_square = get_ls1b_index(bitboard);

                //init piece attacks to get set of target squares
                attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                //loop over target squares available from generated attacks
                while (attacks){
                    //init target square
                    target_square = get_ls1b_index(attacks);

                    //quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece quiet move" << std::endl;

                    //capture move
                    else  std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece capture" << std::endl;

                    popSquare(attacks, target_square);
                }
                //pop ls1b of the current piece bitboard copy
                popSquare(bitboard, source_square);
            }
        }

        // generate bishop moves
        if ((side == white) ? piece == B : piece == b){
            //loop over source squares of piece bitboard copy
            while (bitboard){
                // init source square
                source_square = get_ls1b_index(bitboard);

                //init piece attacks in order to get set of target squares
                attacks = get_bishop_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks){
                    target_square = get_ls1b_index(attacks);

                    //quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece quiet move" << std::endl;

                    //capture move
                    else std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece capture" << std::endl;
                    popSquare(attacks, target_square);
                }


                // pop ls1b of the current piece bitboard copy
                popSquare(bitboard, source_square);
            }
        }

        // generate rook moves
        if ((side == white) ? piece == R : piece == r){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                attacks = get_rook_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    //quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece quiet move" << std::endl;
                    //capture
                    else std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece capture" << std::endl;

                    popSquare(attacks, target_square);
                }
                popSquare(bitboard, source_square);
            }
        }

        // generate queen moves
        if ((side == white) ? piece == Q : piece == q){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                attacks = get_queen_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    // quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece quiet move" << std::endl;
                    // capture
                    else std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece capture" << std::endl;
                    popSquare(attacks, target_square);
                }
                popSquare(bitboard, source_square);
            }
        }

        // generate king moves
        if ((side == white) ? piece == K : piece == k){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    //quite move
                    if (!getSquare(((side == white) ? occupancies[black] : occupancies[white]), target_square)) std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece quiet move" << std::endl;
                    //capture
                    else std::cout << square_to_coordinates[source_square] << square_to_coordinates[target_square] << "  piece capture" << std::endl;
                    popSquare(attacks, target_square);
                }
                popSquare(bitboard, source_square);
            }
        }
    }
}

/**********************************\
 ==================================

         Test Performance

 ==================================
\**********************************/

long nodes;
static inline void perft_driver(int depth) {
    if (depth == 0){
        nodes++;
        return;
    }

    moves move_list[1];
    generate_moves(move_list);

    //loop over generated moves
    for (int move_count = 0; move_count < move_list->count; move_count++){
        copy_board();

        // make move
        if (!make_move(move_list->moves[move_count], all_moves))
            // skip to the next move
            continue;

        perft_driver(depth - 1);

        take_back();
    }
}

//debug
long long get_time_ms() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    return epoch.count();
}
void perft_test(int depth){
    std::cout << "\n     Performance test\n\n";

    moves move_list[1];
    generate_moves(move_list);

    // init start time
    long long start = get_time_ms();

    // loop over generated moves
    for (int move_count = 0; move_count < move_list->count; move_count++){
        copy_board();
        if (!make_move(move_list->moves[move_count], all_moves))
            // skip to the next move
            continue;

        long cummulative_nodes = nodes;

        perft_driver(depth - 1);

        long old_nodes = nodes - cummulative_nodes;

        take_back();

        // print move
        std::cout << "move: " << square_to_coordinates[get_move_source(move_list->moves[move_count])] << square_to_coordinates[get_move_target(move_list->moves[move_count])]
            << (get_move_promoted(move_list->moves[move_count]) ? promoted_pieces[get_move_promoted(move_list->moves[move_count])] : ' ')
            << " node: " << old_nodes << std::endl;
    }

    // print results
    std::cout << "\n    Depth:" << depth;
    std::cout << "\n    Nodes: " << nodes;
    std::cout << "\n    Time: " << get_time_ms() - start << "ms";
}

/**********************************\
 ==================================

             Evaluation

 ==================================
\**********************************/
// material scrore
/*
    ♙ =   100   = ♙
    ♘ =   300   = ♙ * 3
    ♗ =   350   = ♙ * 3 + ♙ * 0.5
    ♖ =   500   = ♙ * 5
    ♕ =   1000  = ♙ * 10
    ♔ =   10000 = ♙ * 100

*/
int material_score[12] = {
    100,      // white pawn score
    300,      // white knight scrore
    350,      // white bishop score
    500,      // white rook score
   1000,      // white queen score
  10000,      // white king score
   -100,      // black pawn score
   -300,      // black knight scrore
   -350,      // black bishop score
   -500,      // black rook score
  -1000,      // black queen score
 -10000,      // black king score
};
// pawn positional score
const int pawn_score[64] ={
    90,  90,  90,  90,  90,  90,  90,  90,
    30,  30,  30,  40,  40,  30,  30,  30,
    20,  20,  20,  30,  30,  30,  20,  20,
    10,  10,  10,  20,  20,  10,  10,  10,
     5,   5,  10,  20,  20,   5,   5,   5,
     0,   0,   0,   5,   5,   0,   0,   0,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};

// knight positional score
const int knight_score[64] ={
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,  10,  10,   0,   0,  -5,
    -5,   5,  20,  20,  20,  20,   5,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,   5,  20,  10,  10,  20,   5,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5, -10,   0,   0,   0,   0, -10,  -5
};

// bishop positional score
const int bishop_score[64] ={
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,  10,  10,   0,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,  10,   0,   0,   0,   0,  10,   0,
     0,  30,   0,   0,   0,   0,  30,   0,
     0,   0, -10,   0,   0, -10,   0,   0

};

// rook positional score
const int rook_score[64] ={
    50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,   0,  20,  20,   0,   0,   0

};

// king positional score
const int king_score[64] ={
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   5,   5,   5,   5,   0,   0,
     0,   5,   5,  10,  10,   5,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
     0,   5,   5,  -5,  -5,   0,   5,   0,
     0,   0,   5,   0, -15,   0,  10,   0
};

// mirror positional score tables for opposite side
const int mirror_score[128] ={
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

//position evaluation returns score based on white perspective
static inline int evaluate(){
    // static evaluation score
    int score = 0;
    U64 bitboard;
    int piece, square;

    // loop over piece bitboards
    for (int bb_piece = P; bb_piece <= k; bb_piece++){
        bitboard = bitboards[bb_piece];

        while (bitboard){
 
            piece = bb_piece;
            square = get_ls1b_index(bitboard);

            // score material weights
            score += material_score[piece];

            // score positional piece scores
            switch (piece){
                // evaluate white pieces
                case P: score += pawn_score[square]; break;
                case N: score += knight_score[square]; break;
                case B: score += bishop_score[square]; break;
                case R: score += rook_score[square]; break;
                case K: score += king_score[square]; break;

                // evaluate black pieces
                case p: score -= pawn_score[mirror_score[square]]; break;
                case n: score -= knight_score[mirror_score[square]]; break;
                case b: score -= bishop_score[mirror_score[square]]; break;
                case r: score -= rook_score[mirror_score[square]]; break;
                case k: score -= king_score[mirror_score[square]]; break;
            }

            popSquare(bitboard, square);
        }
    }

    // return final evaluation based on side
    return (side == white) ? score : -score;
}

/**********************************\
 ==================================

               Search

 ==================================
\**********************************/

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

// MVV LVA [attacker][victim]
static int mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

#define max_ply 64

// killer moves [id][ply]
int killer_moves[2][max_ply];
// history moves [piece][square]
int history_moves[12][64];

/*
      ================================
            Triangular PV table
      --------------------------------
        PV line: e2e4 e7e5 g1f3 b8c6
      ================================

           0    1    2    3    4    5

      0    m1   m2   m3   m4   m5   m6

      1    0    m2   m3   m4   m5   m6

      2    0    0    m3   m4   m5   m6

      3    0    0    0    m4   m5   m6

      4    0    0    0    0    m5   m6

      5    0    0    0    0    0    m6
*/
// PV length[ply]
int pv_length[max_ply];
// PV table[ply][ply]
int pv_table[max_ply][max_ply];

// follow PV & score PV move
int follow_pv, score_pv;

// half move counter
int ply;

// enable PV move scoring
static inline void enable_pv_scoring(moves* move_list){
    // disable following PV
    follow_pv = 0;

    for (int count = 0; count < move_list->count; count++){
        // make sure we hit PV move
        if (pv_table[0][ply] == move_list->moves[count]){
            // enable move scoring
            score_pv = 1;
            // enable following PV
            follow_pv = 1;
        }
    }
}

/*  =======================
         Move ordering
    =======================

    1. PV move
    2. Captures in MVV/LVA
    3. 1st killer move
    4. 2nd killer move
    5. History moves
    6. Unsorted moves
*/
// score moves
static inline int score_move(int move){

    // if PV move scoring is allowed
    if (score_pv){
        // make sure we are dealing with PV move
        if (pv_table[0][ply] == move){
            // disable score PV flag
            score_pv = 0;
            // give PV move the highest score to search it first
            return 20000;
        }
    }

    if (get_move_capture(move)){
        int target_piece = P;
        int start_piece, end_piece;

        if (side == white) { start_piece = p; end_piece = k; }
        else { start_piece = P; end_piece = K; }

        // loop over bitboards opposite to the current side to move
        for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++){
            // if there's a piece on the target square
            if (getSquare(bitboards[bb_piece], get_move_target(move))){
                // remove it from corresponding bitboard
                target_piece = bb_piece;
                break;
            }
        }

        // score move by MVV LVA lookup [source piece][target piece]
        return mvv_lva[get_move_piece(move)][target_piece] + 10000;
    }

    // score quiet move
    else{
        // score 1st killer move
        if (killer_moves[0][ply] == move)
            return 9000;

        // score 2nd killer move
        else if (killer_moves[1][ply] == move)
            return 8000;

        // score history move
        else
            return history_moves[get_move_piece(move)][get_move_target(move)];
    }

    return 0;
}

// sort moves TBD improve sorting algo
static inline void sort_moves(moves* move_list){
    // move scores
    std::vector<int> move_scores(move_list->count);

    for (int count = 0; count < move_list->count; count++)
        move_scores[count] = score_move(move_list->moves[count]);
   
    //sort
    for (int current_move = 0; current_move < move_list->count; current_move++){
        for (int next_move = current_move + 1; next_move < move_list->count; next_move++){
            // compare current and next move scores
            if (move_scores[current_move] < move_scores[next_move]){
                // swap scores
                int temp_score = move_scores[current_move];
                move_scores[current_move] = move_scores[next_move];
                move_scores[next_move] = temp_score;

                // swap moves
                int temp_move = move_list->moves[current_move];
                move_list->moves[current_move] = move_list->moves[next_move];
                move_list->moves[next_move] = temp_move;
            }
        }
    }
}

//quiesence search
static inline int quiescence(int alpha, int beta) {
    nodes++;
    int evaluation = evaluate();
    if (evaluation >= beta){
        // node (move) fails high
        return beta;
    }

    // found a better move
    if (evaluation > alpha){
        // PV node (move)
        alpha = evaluation;
    }

    moves move_list[1];
    generate_moves(move_list);
    sort_moves(move_list);

    // loop over moves within a movelist
    for (int count = 0; count < move_list->count; count++){
        copy_board();
        ply++;
        if (make_move(move_list->moves[count], only_captures) == 0){
            // decrement ply
            ply--;
            continue;
        }

        // score current move
        int score = -quiescence(-beta, -alpha);
        ply--;

        take_back();

        if (score >= beta){
            // node (move) fails high
            return beta;
        }

        if (score > alpha){
            // PV node (move)
            alpha = score;
        }
    }

    // node (move) fails low
    return alpha;
}

//negamax alpha beta search with fail-hard approach -> maybe also implement soft? TBD
const int full_depth_moves = 4;
const int reduction_limit = 3;
static inline int negamax(int alpha, int beta, int depth){
    // define find PV node variable
    int found_pv = 0;

    // init PV length
    pv_length[ply] = ply;

    if (depth == 0)
        // return evaluation
        return quiescence(alpha, beta);

    // we are too deep, so there's an overflow of arrays
    if (ply > max_ply - 1)
        // evaluate position
        return evaluate();

    nodes++;

    //is king in check
    int in_check = is_square_attacked((side == white) ? get_ls1b_index(bitboards[K]) :
                                                        get_ls1b_index(bitboards[k]),
                                                        side ^ 1);

    // increase depth if in check because you can get mated
    if (in_check) depth++;

    int legal_moves = 0;

    moves move_list[1];
    generate_moves(move_list);

    // if we are now following PV line
    if (follow_pv)
        // enable PV move scoring
        enable_pv_scoring(move_list);


    sort_moves(move_list);

    // number of moves searched in a move list
    int moves_searched = 0;

    // loop over moves within a movelist
    for (int count = 0; count < move_list->count; count++){
        // preserve board state
        copy_board();

        ply++;

        // make sure to make only legal moves
        if (make_move(move_list->moves[count], all_moves) == 0){
            // decrement ply
            ply--;
            continue;
        }

        legal_moves++;

        int score;

        // on PV node hit --> code from CMK,  TBD understand it better 
        if (found_pv){
            /* Once you've found a move with a score that is between alpha and beta,
               the rest of the moves are searched with the goal of proving that they are all bad.
               It's possible to do this a bit faster than a search that worries that one
               of the remaining moves might be good. */
            score = -negamax(-alpha - 1, -alpha, depth - 1);

            /* If the algorithm finds out that it was wrong, and that one of the
               subsequent moves was better than the first PV move, it has to search again,
               in the normal alpha-beta manner.  This happens sometimes, and it's a waste of time,
               but generally not often enough to counteract the savings gained from doing the
               "bad move proof" search referred to earlier. */
            if ((score > alpha) && (score < beta)) // Check for failure.
                /* re-search the move that has failed to be proved to be bad
                   with normal alpha beta score bounds*/
                score = -negamax(-beta, -alpha, depth - 1);
        }

        // for all other types of nodes (moves)
        else{
            // full depth search
            if (moves_searched == 0)
                // do normal alpha beta search
                score = -negamax(-beta, -alpha, depth - 1);

            // late move reduction (LMR)
            else{
                // condition to consider LMR
                if (
                    moves_searched >= full_depth_moves &&
                    depth >= reduction_limit &&
                    in_check == 0 &&
                    get_move_capture(move_list->moves[count]) == 0 &&
                    get_move_promoted(move_list->moves[count]) == 0
                    )
                    // search current move with reduced depth:
                    score = -negamax(-alpha - 1, -alpha, depth - 2);

                // hack to ensure that full-depth search is done
                else score = alpha + 1;

                // if found a better move during LMR
                if (score > alpha){
                    // re-search at full depth but with narrowed score bandwith
                    score = -negamax(-alpha - 1, -alpha, depth - 1);

                    // if LMR fails re-search at full depth and full score bandwith
                    if ((score > alpha) && (score < beta))
                        score = -negamax(-beta, -alpha, depth - 1);
                }
            }
        }

        ply--;

        take_back();

        // increment the counter of moves searched so far
        moves_searched++;

        // fail-hard beta cutoff
        if (score >= beta){
            // on quiet moves
            if (get_move_capture(move_list->moves[count]) == 0){
                // store killer moves
                killer_moves[1][ply] = killer_moves[0][ply];
                killer_moves[0][ply] = move_list->moves[count];
            }
            // node (move) fails high
            return beta;
        }

        // found a better move
        if (score > alpha){
            // on quiet moves
            if (get_move_capture(move_list->moves[count]) == 0)
                // store history moves
                history_moves[get_move_piece(move_list->moves[count])][get_move_target(move_list->moves[count])] += depth;

            // PV node (move)
            alpha = score;

            // PV variation flag on
            found_pv = 1;

            // write PV move
            pv_table[ply][ply] = move_list->moves[count];

            // loop over the next ply
            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++)
                // copy move from deeper ply into a current ply's line
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];

            // adjust PV length
            pv_length[ply] = pv_length[ply + 1];
        }
    }

    // no legal move in current position
    if (legal_moves == 0){
        // king is in check
        if (in_check)
            // return mating score (assuming closest distance to mating position)
            return -49000 + ply;

        //king is not in check
        else
            // return stalemate score
            return 0;
    }

    // node (move) fails low
    return alpha;
}

// search position for the best move
void search_position(int depth){
    int score = 0;
    nodes = 0;
    // reset follow PV flags
    follow_pv = 0;
    score_pv = 0;

    // clear helper data structures for search
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    memset(pv_table, 0, sizeof(pv_table));
    memset(pv_length, 0, sizeof(pv_length));

    // iterative deepening
    for (int current_depth = 1; current_depth <= depth; current_depth++)
    {
        follow_pv = 1;

        // find best move within a given position
        score = negamax(-50000, 50000, current_depth);

        std::cout << "\nscore : " << score << "     depth: " << current_depth << "     nodes: " << nodes << "   principal variation:";
        // loop over the moves within a PV line
        for (int count = 0; count < pv_length[0]; count++) {
            // print PV move
            print_move(pv_table[0][count]);
            std::cout << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n\nbestmove ";
    print_move(pv_table[0][0]);
    std::cout << std::endl;
}
//no iterative deepining in the server one need to modify gui
int search_server_position(int depth) {
    // find best move within a given position
    int score = negamax(-50000, 50000, depth);

    return pv_table[0][0];
}

/**********************************\
 ==================================

                UCI

 ==================================
\**********************************/

// parse move string input (e.g. "e7e8q") returns 1 if legal move 0 illegal
int parse_move(const char* move_string){
    moves move_list[1];
    generate_moves(move_list);

    int source_square = (move_string[0] - 'a') + (8 - (move_string[1] - '0')) * 8;
    int target_square = (move_string[2] - 'a') + (8 - (move_string[3] - '0')) * 8;

    // loop over the moves within a move list
    for (int move_count = 0; move_count < move_list->count; move_count++){

        int move = move_list->moves[move_count];

        if (source_square == get_move_source(move) && target_square == get_move_target(move)){
            
            int promoted_piece = get_move_promoted(move);

            // promoted piece is available
            if (promoted_piece){
                // promoted to queen
                if ((promoted_piece == Q || promoted_piece == q) && move_string[4] == 'q') return move;

                // promoted to rook
                else if ((promoted_piece == R || promoted_piece == r) && move_string[4] == 'r') return move;

                // promoted to bishop
                else if ((promoted_piece == B || promoted_piece == b) && move_string[4] == 'b') return move;

                // promoted to knight
                else if ((promoted_piece == N || promoted_piece == n) && move_string[4] == 'n') return move;

                continue;
            }
            //move is legal
            return move;
        }
    }

    //return illegal move
    return 0;
}

//Current UCI implementation is taken from VICE by Richard Allbert aka Bluefever Software.
/*
    Example UCI commands to init position on chess board

    // init start position
    position startpos

    // init start position and make the moves on chess board
    position startpos moves e2e4 e7e5

    // init position from FEN string
    position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1

    // init position from fen string and make moves on chess board
    position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 moves e2a6 e8g8
*/
// parse UCI "position" command
void parse_position(char* command){
    // shift pointer to the right where next token begins
    command += 9;
    char* current_char = command;

    // parse UCI "startpos" command
    if (strncmp(command, "startpos", 8) == 0) parse_fen(start_position);

    // parse UCI "fen" command 
    else{
        // make sure "fen" command is available within command string
        current_char = strstr(command, "fen");

        // if no "fen" command is available within command string
        if (current_char == NULL)
            // init chess board with start position
            parse_fen(start_position);

        // found "fen" substring
        else{
            // shift pointer to the right where next token begins
            current_char += 4;

            // init chess board with position from FEN string
            parse_fen(current_char);
        }
    }

    //parse moves after position
    current_char = strstr(command, "moves");

    // moves available
    if (current_char != NULL){
        // shift pointer to the right where next token begins
        current_char += 6;

        // loop over moves within a move string
        while (*current_char){
            // parse next move
            int move = parse_move(current_char);

            // if no more moves
            if (move == 0)
                // break out of the loop
                break;

            // make move on the chess board
            make_move(move, all_moves);

            // move current character mointer to the end of current move
            while (*current_char && *current_char != ' ') current_char++;

            // go to the next move
            current_char++;
        }

        std::cout << current_char << std::endl;
    }

    print_board();
}

// parse UCI "go" command
void parse_go(char* command){
    // init depth
    int depth = -1;

    // init character pointer to the current depth argument
    char* current_depth = NULL;

    // handle fixed depth search
    if (current_depth = strstr(command, "depth")) depth = atoi(current_depth + 6);

    // different time controls placeholder
    else depth = 6;

    // search position
    search_position(depth);
}
int parse_server_go(char* command) {
    // init depth
    int depth = -1;

    // init character pointer to the current depth argument
    char* current_depth = NULL;

    // handle fixed depth search
    if (current_depth = strstr(command, "depth")) depth = atoi(current_depth + 6);

    // different time controls placeholder
    else depth = 6;

    // search position
    return search_server_position(depth);
}

//main UCI loop
void uci_loop(){
    char input[2000];
    char startpos[] = "position startpos";

    std::cout << R"(
                         _                      _           
                        / \      __ _    __ _  | |_    __ _ 
                       / _ \    / _` |  / _` | | __|  / _` |
                      / ___ \  | (_| | | (_| | | |_  | (_| |
                     /_/   \_\  \__, |  \__,_|  \__|  \__,_|
                                |___/

                           Chess engine made by marrets
)";

    std::cout << "\n\n\n";

    // main loop
    while (1){
        memset(input, 0, sizeof(input));
        fflush(stdout);

        // get user / GUI input
        if (!fgets(input, 2000, stdin))
            continue;

        // make sure input is available
        if (input[0] == '\n')
            continue;

        // parse UCI "isready" command
        if (strncmp(input, "isready", 7) == 0){
            std::cout << "readyok\n";
            continue;
        }

        // parse UCI "position" command
        else if (strncmp(input, "position", 8) == 0)
            parse_position(input);

        // parse UCI "ucinewgame" command
        else if (strncmp(input, "ucinewgame", 10) == 0)
            parse_position(startpos);

        // parse UCI "go" command
        else if (strncmp(input, "go", 2) == 0)
            parse_go(input);

        // parse UCI "quit" command
        else if (strncmp(input, "quit", 4) == 0)
            break;

        // parse UCI "uci" command
        else if (strncmp(input, "uci", 3) == 0){
            std::cout << "id name Agata" << "\n" << "uciok" <<std::endl;
        }
    }
}
void uci_server_loop() {
    char startpos[] = "position startpos";

    std::cout << R"(
                         _                      _           
                        / \      __ _    __ _  | |_    __ _ 
                       / _ \    / _` |  / _` | | __|  / _` |
                      / ___ \  | (_| | | (_| | | |_  | (_| |
                     /_/   \_\  \__, |  \__,_|  \__|  \__,_|
                                |___/

                           Chess engine made by marrets
    )";

    std::cout << "\n\n\n";

    // Socket Logic
    WSADATA wsaData;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addr_len = sizeof(address);
    char buffer[10240] = { 0 };

    // Inizializza Winsock
    initializeWinsock(wsaData);
    server_fd = createSocket();
    if (server_fd == INVALID_SOCKET) {
        cleanupWinsock();
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Ascolta su tutte le interfacce
    address.sin_port = htons(8080);

    // Associa il socket a un indirizzo e porta
    if (!bindSocket(server_fd, address)) {
        closeSockets(server_fd, INVALID_SOCKET);
        cleanupWinsock();
        return;
    }

    // Inizia ad ascoltare
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return;
    }

    std::cout << "now listening on port 8080\n";

    // Accetta una connessione
    new_socket = acceptConnection(server_fd, address, addr_len);
    if (new_socket == INVALID_SOCKET) {
        closeSockets(server_fd, INVALID_SOCKET);
        cleanupWinsock();
        return;
    }

    std::cout << "connection accepted\n";

    // main loop
    while (1) {
        int recv_size = recv(new_socket, buffer, sizeof(buffer), 0);
        if (recv_size > 0) {
            std::cout << "Message from client: " << buffer << std::endl;
        }
        buffer[sizeof(buffer +1)] = '\0';

        // make sure input is available
        if (buffer[0] == '\0')
            continue;

        // parse UCI "isready" command
        if (strncmp(buffer, "isready", 7) == 0) {
            sendResponse(new_socket, "readyok");
            continue;
        }

        // parse UCI "position" command
        else if (strncmp(buffer, "position", 8) == 0) {
            parse_position(buffer);
            sendResponse(new_socket, "Position set correctly\n");
            std::cout << "Position set correctly\n";
        }
            

        // parse UCI "ucinewgame" command
        else if (strncmp(buffer, "ucinewgame", 10) == 0) {
            parse_position(startpos);
            std::cout << "New Game created\n";
        }

        // parse UCI "go" command
        else if (strncmp(buffer, "go", 2) == 0) {
            int move = parse_server_go(buffer);
            std::cout << "Searching Position\n";
            std::string move_string;

            // Aggiungi i vari componenti della mossa alla stringa
            move_string += square_to_coordinates[get_move_source(move)];
            move_string += square_to_coordinates[get_move_target(move)];
            move_string += promoted_pieces[get_move_promoted(move)];

            sendResponse(new_socket, move_string.c_str());
        }

        // parse UCI "quit" command
        else if (strncmp(buffer, "quit", 4) == 0) {
            closesocket(new_socket);
            closesocket(server_fd);
            break;
        }

        // parse UCI "uci" command
        else if (strncmp(buffer, "uci", 3) == 0) {
            sendResponse(new_socket, "id name Agata \nuciok");
            std::cout << "id name Agata \nuciok\n";
        }
    }
}



/**********************************\
 ==================================

               Main

 ==================================
\**********************************/

void init_all(){
    // init magic numbers   | ALREADY GENERATED
    //init_magic_numbers();

    // init leaper pieces attacks
    init_leapers_attacks();

    // init slider pieces attacks
    init_sliders_attacks(bishop);
    init_sliders_attacks(rook);

}

int main(){
    // init all
    init_all();

    // debug mode variable
    int mode =0;

    // if debugging
    if (mode == 0)
    {
        // parse fen
        parse_fen(tricky_position);
        print_board();
        search_position(7);
    }
    else if (mode == 1)
        // connect to the GUI
        uci_server_loop();
    else
        // command line engine
        uci_loop();

    return 0;
}
