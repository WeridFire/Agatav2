# Agata Chess Engine  

<p align="left">
  <img src="Agatalogo.jpg" alt="Agata Logo" width="200">
</p>

Agata is a high-performance chess engine with **neural network evaluation** designed for strategic gameplay and efficient computation. Originally tested on [Lichess](https://lichess.org) with an Elo of 2800+, Agata now incorporates machine learning to improve position evaluation through self-learning.

---

## Features  

### Core Engine

- **Bitboard Representation**  
  Efficient board representation using 64-bit integers for fast move generation and evaluation.  

- **Pre-Calculated Attack Tables**  
  Speeds up move generation using pre-computed attack patterns for all piece types.  

- **Magic Bitboards for Sliding Pieces**  
  Highly optimized algorithms for rook, bishop, and queen movement using magic multiplication.  

- **Moves Encoding as Integers**  
  Compact and efficient move representation for faster processing and lower memory overhead.  

- **Negamax Search with Alpha-Beta Pruning**  
  Reduces the search tree size while maintaining optimal move selection.  

- **Advanced Move Ordering**  
  - PV (Principal Variation) move prioritization  
  - Killer move heuristic  
  - History heuristic  
  - MVV-LVA (Most Valuable Victim - Least Valuable Attacker) for captures  

- **Search Extensions**  
  - Check extension: searches deeper when king is in check  
  - Quiescence search: continues searching captures to avoid horizon effect  
  - Null move pruning: optimizes search by giving opponent extra moves  
  - Late Move Reduction (LMR): reduces depth for unlikely moves  

- **UCI Protocol Support**  
  Fully compatible with Universal Chess Interface (UCI) GUIs like Arena, ChessBase, and CuteChess.  

### Neural Network Integration ⚡ NEW

- **Value Network Evaluation**  
  Replaces traditional handcrafted evaluation with a trained neural network that predicts position outcomes.  

- **Supervised Learning from Master Games**  
  Trained on 300k+ positions from 2000+ rated games (FICS Games Database).  

- **CPU-Optimized MLP Architecture**  
  Lightweight Multi-Layer Perceptron (781 input features → 256 hidden → 1 value output) with no external dependencies.  

- **Feature-Rich Input Representation**  
  - 12 piece bitboard planes (P, N, B, R, Q, K for both colors)  
  - Castling rights (4 bits)  
  - En-passant file (8-bit one-hot)  
  - Side to move (1 bit)  

- **UCI Options for NN Control**  
  ```
  setoption name UseNN value true|false
  setoption name NNModelPath value <path>
  ```

- **Training Pipeline**  
  Complete Python-based training system:  
  - PGN parser to extract positions and outcomes  
  - PyTorch training script with customizable hyperparameters  
  - Text-based model format for easy C++ integration  

---

## Quick Start

### Building the Engine

**Windows (Visual Studio):**
```bash
# Open Agatav2.sln in Visual Studio
# Build in Release x64 configuration
```

**Command Line:**
```bash
msbuild Agatav2.sln /p:Configuration=Release /p:Platform=x64
```

### Running with Neural Network

1. **Train a model** (or use pre-trained):
```bash
# Install Python dependencies
pip install python-chess torch numpy

# Convert PGN games to training data
python training/pgn_to_dataset.py games/your_games.pgn --out data/train.npz --max-games 50000

# Train the value network
python training/train_value.py --data data/train.npz --epochs 30 --hidden 256 --out models/value_model.txt
```

2. **Run the engine** with UCI GUI or command line:
```
uci
setoption name NNModelPath value C:\path\to\models\value_model.txt
setoption name UseNN value true
isready
position startpos
go depth 10
```

---

## Project Structure

```
Agatav2/
├── Agatav2/
│   ├── Agatav2.cpp      # Main engine code
│   ├── neural.h/.cpp    # Neural network inference
│   └── sock.h           # Socket communication
├── training/
│   ├── pgn_to_dataset.py   # Convert PGN to training data
│   ├── train_value.py      # Train value network
│   └── README.md           # Training guide
├── games/               # PGN game databases (not in repo)
├── data/                # Training datasets (not in repo)
├── models/              # Trained models (not in repo)
└── README.md            # This file
```

---

## Training Your Own Model

See [training/README.md](training/README.md) for detailed instructions on:
- Converting PGN databases to training data
- Training the value network
- Hyperparameter tuning
- Iterative improvement with self-play

**Recommended PGN sources:**
- [Lichess Database](https://database.lichess.org/) - Millions of games
- [FICS Games Database](https://www.ficsgames.org/download.html) - Computer games
- [TWIC](https://theweekinchess.com/twic) - Weekly GM games

---

## Performance

- **Classical Eval**: 2200+ Elo on Lichess
- **Neural Eval**: Currently in development and testing
- **Search Speed**: ~2M nodes/sec (Release build, depth 10)
- **NN Inference**: <1ms per evaluation (CPU, 256 hidden units)

---

## Roadmap

- [ ] Policy network for improved move ordering
- [ ] Self-play training loop
- [ ] Transposition table for NN evaluations
- [ ] NNUE-style efficiently updatable features
- [ ] Opening book integration
- [ ] Endgame tablebases support

---

## Credits

- Engine architecture: marrets (WeridFire)
- Neural network integration: 2024-2025
- Tested on Lichess with 2800+ rating

---

## License

This project is open source. Feel free to use, modify, and distribute.

