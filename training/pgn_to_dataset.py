#!/usr/bin/env python3
"""
Parse PGN files and extract training data for Agata's value network.
Samples positions from games and labels them with the final game outcome.

Requires: python-chess (pip install python-chess)
"""
import argparse
import random
from pathlib import Path

import numpy as np
import chess
import chess.pgn


def board_to_features(board: chess.Board) -> np.ndarray:
    """
    Build feature vector matching engine format (781 floats):
    - 12*64 = 768: piece planes P,N,B,R,Q,K,p,n,b,r,q,k (one-hot per square)
    - 4: castling rights (wk, wq, bk, bq)
    - 8: en-passant file (one-hot 0..7 if ep square exists)
    - 1: side to move (1 for white, 0 for black)
    """
    x = np.zeros(12*64 + 4 + 8 + 1, dtype=np.float32)
    
    # Piece planes (order: P,N,B,R,Q,K,p,n,b,r,q,k)
    piece_map = {
        chess.PAWN: 0, chess.KNIGHT: 1, chess.BISHOP: 2,
        chess.ROOK: 3, chess.QUEEN: 4, chess.KING: 5
    }
    
    for sq in chess.SQUARES:
        piece = board.piece_at(sq)
        if piece:
            idx = piece_map[piece.piece_type] + (6 if piece.color == chess.BLACK else 0)
            x[idx * 64 + sq] = 1.0
    
    off = 12 * 64
    # Castling rights (wk, wq, bk, bq)
    x[off + 0] = 1.0 if board.has_kingside_castling_rights(chess.WHITE) else 0.0
    x[off + 1] = 1.0 if board.has_queenside_castling_rights(chess.WHITE) else 0.0
    x[off + 2] = 1.0 if board.has_kingside_castling_rights(chess.BLACK) else 0.0
    x[off + 3] = 1.0 if board.has_queenside_castling_rights(chess.BLACK) else 0.0
    
    # En-passant file
    ep_off = off + 4
    if board.ep_square is not None:
        ep_file = chess.square_file(board.ep_square)
        x[ep_off + ep_file] = 1.0
    
    # Side to move
    x[ep_off + 8] = 1.0 if board.turn == chess.WHITE else 0.0
    
    return x


def parse_result(result_str: str) -> float:
    """Convert PGN result to outcome: 1.0 (white win), 0.0 (draw), -1.0 (black win)"""
    if result_str == '1-0':
        return 1.0
    elif result_str == '0-1':
        return -1.0
    else:
        return 0.0


def extract_positions_from_game(game, sample_rate=0.3, min_ply=10):
    """
    Extract positions from a single game.
    - sample_rate: probability of sampling each position (to avoid huge datasets)
    - min_ply: skip early opening moves to reduce book bias
    Returns list of (feature_vector, outcome_from_side_to_move_perspective)
    """
    result = parse_result(game.headers.get('Result', '*'))
    if result == 0.0 and game.headers.get('Result', '*') == '*':
        return []  # Skip unfinished games
    
    positions = []
    board = game.board()
    ply = 0
    
    for move in game.mainline_moves():
        board.push(move)
        ply += 1
        
        if ply < min_ply:
            continue
        if random.random() > sample_rate:
            continue
        
        # Get features
        x = board_to_features(board)
        
        # Outcome from side-to-move perspective
        # result is from white's perspective, so flip if black to move
        z = result if board.turn == chess.WHITE else -result
        
        positions.append((x, z))
    
    return positions


def main():
    ap = argparse.ArgumentParser(description='Convert PGN to NPZ dataset for Agata value net')
    ap.add_argument('pgn', type=str, help='Input PGN file')
    ap.add_argument('--out', type=str, default='training_data.npz', help='Output NPZ file')
    ap.add_argument('--max-games', type=int, default=None, help='Limit number of games to parse')
    ap.add_argument('--sample-rate', type=float, default=0.3, help='Probability of sampling each position')
    ap.add_argument('--min-ply', type=int, default=10, help='Skip first N plies (opening book)')
    ap.add_argument('--seed', type=int, default=42, help='Random seed')
    args = ap.parse_args()
    
    random.seed(args.seed)
    np.random.seed(args.seed)
    
    X_all = []
    z_all = []
    
    print(f"Parsing {args.pgn}...")
    with open(args.pgn, 'r', encoding='utf-8', errors='ignore') as f:
        game_count = 0
        while True:
            game = chess.pgn.read_game(f)
            if game is None:
                break
            
            positions = extract_positions_from_game(game, args.sample_rate, args.min_ply)
            for x, z in positions:
                X_all.append(x)
                z_all.append(z)
            
            game_count += 1
            if game_count % 1000 == 0:
                print(f"  Processed {game_count} games, {len(X_all)} positions...")
            
            if args.max_games and game_count >= args.max_games:
                break
    
    if not X_all:
        print("No positions extracted!")
        return
    
    X = np.stack(X_all, axis=0)
    z = np.array(z_all, dtype=np.float32)
    
    print(f"\nExtracted {len(X)} positions from {game_count} games")
    print(f"Outcome distribution: win={np.sum(z>0.5)}, draw={np.sum(np.abs(z)<0.5)}, loss={np.sum(z<-0.5)}")
    
    np.savez_compressed(args.out, x=X, z=z)
    print(f"Saved to {args.out}")


if __name__ == '__main__':
    main()
