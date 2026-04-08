/*****************************************************
    AUTHOR  : Sébastien Valat
    MAIL    : sebastien.valat@univ-grenoble-alpes.fr
    LICENSE : BSD
    YEAR    : 2021
    COURSE  : Parallel Algorithms and Programming
*****************************************************/

//////////////////////////////////////////////////////
//
// Goal: Implement non-blocking 1D communication scheme
//       along X axis.
//
// SUMMARY:
//     - 1D splitting along X
// NEW:
//     - >>> Non-blocking communications <<<
//
//////////////////////////////////////////////////////

/****************************************************/
#include "src/lbm_struct.h"
#include "src/exercises.h"

/****************************************************/
void lbm_comm_init_ex3(lbm_comm_t * comm, int total_width, int total_height)
{
	//we use the same implementation then ex1
	lbm_comm_init_ex1(comm, total_width, total_height);
}

/****************************************************/
void lbm_comm_ghost_exchange_ex3(lbm_comm_t * comm, lbm_mesh_t * mesh)
{
	//
	// TODO: Implement the 1D communication with non-blocking MPI functions.
	//
	// To be used:
	//    - DIRECTIONS: the number of doubles composing a cell
	//    - double[9] lbm_mesh_get_cell(mesh, x, y): function to get the address of a particular cell.
	//    - comm->width : The with of the local sub-domain (containing the ghost cells)
	//    - comm->height : The height of the local sub-domain (containing the ghost cells)
	
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int left  = rank - 1;
	int right = rank + 1;
	if (left < 0) left = MPI_PROC_NULL;
	if (right >= size) right = MPI_PROC_NULL;

	int count = comm->height * DIRECTIONS;
	MPI_Request requests[4];

	// post all receives first
	MPI_Irecv(lbm_mesh_get_cell(mesh, 0, 0), count, MPI_DOUBLE, left, 0, MPI_COMM_WORLD, &requests[0]);
	MPI_Irecv(lbm_mesh_get_cell(mesh, comm->width - 1, 0), count, MPI_DOUBLE, right, 1, MPI_COMM_WORLD, &requests[1]);

	// then all sends
	MPI_Isend(lbm_mesh_get_cell(mesh, 1, 0), count, MPI_DOUBLE, left, 1, MPI_COMM_WORLD, &requests[2]);
	MPI_Isend(lbm_mesh_get_cell(mesh, comm->width - 2, 0), count, MPI_DOUBLE, right, 0, MPI_COMM_WORLD, &requests[3]);

	// wait for everything
	MPI_Waitall(4, requests, MPI_STATUSES_IGNORE);
}
