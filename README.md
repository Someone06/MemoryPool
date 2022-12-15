# Depth First Search using Pointer Reversal
This is a simple toy project that demonstrates how to do a depth first search
(DFS) on a connected, directed graph while storing the stack needed for the DFS
inside the nodes of the graph.
This usually requires a `size_t pointerCounter` that stores which neighbour to
visit next as well as a `Node *parentPointer` in each node.
By using pointer reversal, the extra storage for the parent pointer is not
needed.
To mark nodes that have already been visited an additional bit is required. 
The highest bit of the `pointerCounter` is used to store that information.
So the overhead of doing a DFS, is a `size_t` per node, plus a constant amount
of stack space.

To free all nodes (which are allocated on the heap) while only using a constant
amount of memory two DFS traversals are required.
The first traversal removes edges such that the modified graph is a (spanning-)
tree.
The second traversal frees the memory.

# License
Licensed under the terms of the Apache 2.0 license. See [`LICENSE`](LICENSE) and [`NOTICE`](NOTICE).
