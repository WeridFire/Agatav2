# Agata Chess Engine  

<p align="left">
  <img src="Agatalogo.jpg" alt="Agata Logo" width="200">
</p>

Agata is a high-performance chess engine designed for strategic gameplay and efficient computation. 
It has been tested on [Lichess](lichess.org) for quite a while and she has an elo between 1900-2200. 

---

## Features  

- **Bitboard Representation**  
  Efficient board representation for fast move generation and evaluation.  

- **Pre-Calculated Attack Tables**  
  Speeds up move generation using pre-computed attack patterns.  

- **Magic Bitboards for Sliding Pieces**  
  Highly optimized algorithms for rook, bishop, and queen movement.  

- **Moves Encoding as Integers**  
  Compact and efficient move representation for faster processing.  

- **Negamax Search with Alpha-Beta Pruning**  
  Reduces the search tree size while maintaining optimal move selection.  

- **PV/Killer/History Move Ordering**  
  Advanced move ordering techniques for faster search performance.  

- **Transposition Tables**  (TBD)
  Avoids redundant calculations by reusing previously evaluated positions.  

- **Comprehensive Evaluation Function**  
  Evaluates material, position, pawn structure(TBD), mobility(TBD), and king safety(TBD).  

- **UCI Protocol Support**  
  Fully compatible with Universal Chess Interface (UCI) GUIs.  

