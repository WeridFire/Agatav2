# Data directory
This folder holds NPZ training datasets.

## Quick start
Run from project root:
```powershell
python training/pgn_to_dataset.py games/ficsgamesdb_202501_standard2000_nomovetimes_331935.pgn --out data/training_data.npz --max-games 50000
```

This creates `training_data.npz` with sampled positions and outcomes ready for training.
