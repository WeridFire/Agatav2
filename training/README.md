# Agata ValueNet training

This is a minimal pipeline to train a value network that plugs into the engine (Option A).

## Features
- Input size: `12*64 + 4 + 8 + 1 = 781`
  - 12 planes (P..k) flattened to 768.
  - 4 castling bits (wk,wq,bk,bq).
  - 8 en-passant file one-hot (0..7), else zeros.
  - 1 side-to-move bit (1 if white, else 0).
- Output: value v in [-1,1] from side-to-move perspective.

## Data format
- NPZ files with keys:
  - `x`: float32 array (N, 781)
  - `z`: float32 array (N,) with targets in [-1,1]

## Getting training data

### Option 1: Use existing PGN games (recommended for bootstrapping)
You already have a great dataset: **FICS Games Database** with 331k+ games at 2000+ rating.

1. Install Python dependencies:
```powershell
pip install python-chess torch numpy
```

2. Convert PGN to training dataset:
```powershell
python training/pgn_to_dataset.py games/ficsgamesdb_202501_standard2000_nomovetimes_331935.pgn --out data/training_data.npz --max-games 50000 --sample-rate 0.3
```

This will:
- Parse up to 50,000 games
- Sample ~30% of positions from each game (after ply 10)
- Label each position with the game outcome from side-to-move perspective
- Save to `data/training_data.npz`

**Other PGN sources:**
- **Lichess Database**: https://database.lichess.org/ (monthly databases with millions of games)
- **FICS Games Database**: https://www.ficsgames.org/download.html (you already have this!)
- **CCRL/CEGT**: Computer chess rating lists with engine games
- **TWIC (The Week In Chess)**: https://theweekinchess.com/twic (weekly updates)

Recommended: Start with 2000+ rated games to avoid learning bad moves.

### Option 2: Self-play (for iterative improvement)
After initial training, generate data from your own engine:
- Use the engine to play games against itself at fixed depth (e.g., depth 6-8)
- Record positions and outcomes
- Mix with human games to maintain diversity

## Train
Once you have the NPZ dataset:

```powershell
python training/train_value.py --data data/training_data.npz --epochs 20 --batch 4096 --hidden 256 --out models/value_model.txt
```

Options:
- `--hidden 256`: Network size (try 128-512)
- `--epochs 20`: Training epochs (watch for overfitting)
- `--lr 1e-3`: Learning rate
- `--batch 4096`: Batch size (adjust for your RAM/GPU)

## Use in engine (UCI)
Start engine and set options:
```
uci
setoption name NNModelPath value C:\\path\\to\\models\\value_model.txt
setoption name UseNN value true
isready
position startpos
go depth 10
```

The engine will use the neural value at leaves and in quiescence.

## Example workflow
```powershell
# 1. Convert PGN to dataset
python training/pgn_to_dataset.py games/ficsgamesdb_202501_standard2000_nomovetimes_331935.pgn --out data/train.npz --max-games 50000

# 2. Train model
python training/train_value.py --data data/train.npz --epochs 30 --hidden 256 --out models/value_v1.txt

# 3. Test in engine
# (in UCI interface)
setoption name NNModelPath value C:\\Users\\filip\\Documents\\c++\\Agatav2\\models\\value_v1.txt
setoption name UseNN value true
go depth 8
```

## Tips
- Start with 10k-50k games to train quickly and test the pipeline
- Monitor training loss; if it plateaus early, try a larger network
- Test against your classical eval to verify improvement
- Later: mix in self-play data for iterative improvement
