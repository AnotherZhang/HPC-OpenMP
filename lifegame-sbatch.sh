#!/bin/bash -e
#SBATCH --job-name="lifegame"
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --time=00:10:00
#SBATCH --partition=normal
#SBATCH --output=lifegame_%j.out


export OMP_NUM_THREADS=16

./lifegame 1 
