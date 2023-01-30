# HPC-OpenMP
## game_life_without_parallel.c
In this file, you can verify different patterns by changing the value of "pattern". Also you can modify the "generations" and "matrix_size" to change the task size.

## lifegame.c
In this file, the parallelism is implemented by OpenMP.
#### How to run it on SURF platform?
```
make lifegame
sbatch lifegame-sbatch.sh
```
