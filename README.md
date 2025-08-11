# Experiment Code for the Article "Efficient Reuse of Metric Indexes for Multi-resolution Queries"

[cite_start]This repository contains the code used to perform the experiments described in the article **"Efficient Reuse of Metric Indexes for Multi-resolution Queries"**, authored by Rodrigo César Arboleda.

The code implements the approach proposed in the article to efficiently reuse Metric Access Methods (MAMs) in multi-resolution queries, using the **Slim-tree** code provided by the Database and Image Group (GBDI) of the Institute of Mathematical and Computer Sciences (ICMC) at the University of São Paulo (USP) as a foundation.

## About the Article

[cite_start]The article addresses the challenge of performing similarity queries on data that has undergone resolution transformation, a common scenario in distributed systems and resource-constrained applications[cite: 14, 15, 490]. [cite_start]Traditionally, a change in data resolution invalidates indexes built on the original data, forcing an index rebuild or a sequential scan, both of which are inefficient[cite: 16, 29, 491].

[cite_start]The solution proposed and evaluated in this code consists of adapting the pruning heuristic of a MAM (in this case, the Slim-tree) to incorporate safe upper bounds for distances in the transformed domain[cite: 11, 32, 493]. [cite_start]For the experiments, the **Haar Transform** was used for dimensionality reduction and the **Manhattan distance** as the similarity measure[cite: 30, 126, 493].

## The Code

The code in this repository allows for the execution of experiments that validate the effectiveness of the approach, comparing the performance of the adapted Slim-tree against a sequential scan in terms of:
* [cite_start]Disk accesses[cite: 190, 286].
* [cite_start]Distance calculations[cite: 190, 325].
* [cite_start]Total query execution time[cite: 190, 334].

[cite_start]The experiments demonstrate that reusing the index with the adapted heuristic achieves significant performance gains, even at high compression (resolution reduction) rates, thus validating the feasibility of the technique[cite: 339, 450, 495].