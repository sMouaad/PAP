/*****************************************************
    AUTHOR  : Sébastien Valat
    MAIL    : sebastien.valat@univ-grenoble-alpes.fr
    LICENSE : BSD
    YEAR    : 2021
    COURSE  : Parallel Algorithms and Programming
*****************************************************/

//////////////////////////////////////////////////////
//
// Goal: Implement odd/even 1D blocking communication scheme 
//       along X axis.
//
// SUMMARY:
//     - 1D splitting along X
//     - Blocking communications
// NEW:
//     - >>> Odd/even communication ordering <<<<
//
//////////////////////////////////////////////////////

/****************************************************/
#include "src/lbm_struct.h"
#include "src/exercises.h"

/****************************************************/
void lbm_comm_init_ex2(lbm_comm_t * comm, int total_width, int total_height)
{
	//we use the same implementation then ex1
	lbm_comm_init_ex1(comm, total_width, total_height);
}

/****************************************************/
void lbm_comm_ghost_exchange_ex2(lbm_comm_t * comm, lbm_mesh_t * mesh)
{
	//
	// TODO: Implement the 1D communication with blocking MPI functions using
	//       odd/even communications.
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

	// odd/even: even sends first, odd recvs first
	// this avoids the serialization chain of exercise 1
	if (rank % 2 == 0) {
		// send to right, recv from right
		MPI_Send(lbm_mesh_get_cell(mesh, comm->width - 2, 0), count, MPI_DOUBLE, right, 0, MPI_COMM_WORLD);
		MPI_Recv(lbm_mesh_get_cell(mesh, comm->width - 1, 0), count, MPI_DOUBLE, right, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		// send to left, recv from left
		MPI_Send(lbm_mesh_get_cell(mesh, 1, 0), count, MPI_DOUBLE, left, 1, MPI_COMM_WORLD);
		MPI_Recv(lbm_mesh_get_cell(mesh, 0, 0), count, MPI_DOUBLE, left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	} else {
		// recv from left, send to left
		MPI_Recv(lbm_mesh_get_cell(mesh, 0, 0), count, MPI_DOUBLE, left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Send(lbm_mesh_get_cell(mesh, 1, 0), count, MPI_DOUBLE, left, 1, MPI_COMM_WORLD);
		// recv from right, send to right
		MPI_Recv(lbm_mesh_get_cell(mesh, comm->width - 1, 0), count, MPI_DOUBLE, right, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Send(lbm_mesh_get_cell(mesh, comm->width - 2, 0), count, MPI_DOUBLE, right, 0, MPI_COMM_WORLD);
	}
}
