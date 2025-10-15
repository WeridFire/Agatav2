import argparse
import json
import math
import os
import random
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader

# Feature size must match engine (12*64 + 4 + 8 + 1)
INPUT_SIZE = 12*64 + 4 + 8 + 1

class ValueNet(nn.Module):
    def __init__(self, hidden=256):
        super().__init__()
        self.fc1 = nn.Linear(INPUT_SIZE, hidden)
        self.fc2 = nn.Linear(hidden, 1)

    def forward(self, x):
        x = torch.relu(self.fc1(x))
        v = torch.tanh(self.fc2(x))
        return v

class Samples(Dataset):
    def __init__(self, npz_paths):
        self.X = []
        self.y = []
        for p in npz_paths:
            d = np.load(p)
            self.X.append(d['x'])
            self.y.append(d['z'])
        self.X = np.concatenate(self.X, axis=0).astype(np.float32)
        self.y = np.concatenate(self.y, axis=0).astype(np.float32)

    def __len__(self):
        return self.X.shape[0]

    def __getitem__(self, idx):
        return self.X[idx], self.y[idx]


def save_txt_model(model: ValueNet, path: str):
    with torch.no_grad():
        W1 = model.fc1.weight.cpu().numpy()
        b1 = model.fc1.bias.cpu().numpy()
        W2 = model.fc2.weight.cpu().numpy()[0]
        b2 = float(model.fc2.bias.cpu().numpy()[0])
    with open(path, 'w') as f:
        f.write(f"{INPUT_SIZE} {W1.shape[0]} 1\n")
        # W1 row-major
        f.write(" ".join(map(lambda x: f"{x:.6f}", W1.flatten())) + "\n")
        f.write(" ".join(map(lambda x: f"{x:.6f}", b1)) + "\n")
        f.write(" ".join(map(lambda x: f"{x:.6f}", W2)) + "\n")
        f.write(f"{b2:.6f}\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--data', type=str, required=True, nargs='+', help='NPZ files with keys x (N,INPUT_SIZE) and z (N,) in [-1,1]')
    ap.add_argument('--epochs', type=int, default=10)
    ap.add_argument('--batch', type=int, default=2048)
    ap.add_argument('--hidden', type=int, default=256)
    ap.add_argument('--lr', type=float, default=1e-3)
    ap.add_argument('--out', type=str, default='value_model.txt')
    args = ap.parse_args()

    ds = Samples(args.data)
    dl = DataLoader(ds, batch_size=args.batch, shuffle=True, drop_last=False)

    model = ValueNet(hidden=args.hidden)
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    model.to(device)

    opt = optim.Adam(model.parameters(), lr=args.lr)
    loss_fn = nn.MSELoss()

    for epoch in range(1, args.epochs+1):
        model.train()
        total = 0.0
        n = 0
        for xb, yb in dl:
            xb = xb.to(device)
            yb = yb.to(device).view(-1, 1)
            opt.zero_grad()
            v = model(xb)
            loss = loss_fn(v, yb)
            loss.backward()
            opt.step()
            total += float(loss.item()) * xb.size(0)
            n += xb.size(0)
        print(f"epoch {epoch} loss {total / max(1,n):.6f}")

    save_txt_model(model, args.out)
    print('saved', args.out)

if __name__ == '__main__':
    main()
