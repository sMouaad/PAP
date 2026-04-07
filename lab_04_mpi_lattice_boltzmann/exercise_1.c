/*****************************************************
    AUTHOR  : Sébastien Valat
    MAIL    : sebastien.valat@univ-grenoble-alpes.fr
    LICENSE : BSD
    YEAR    : 2021
    COURSE  : Parallel Algorithms and Programming
*****************************************************/

//////////////////////////////////////////////////////
//
//
// GOAL: Implement a 1D communication scheme along
//       X axis with blocking communications.
//
// SUMMARY:
//     - 1D splitting along X
//     - Blocking communications
//
//////////////////////////////////////////////////////

/****************************************************/
#include "src/lbm_struct.h"
#include "src/exercises.h"

/****************************************************/
void lbm_comm_init_ex1(lbm_comm_t * comm, int total_width, int total_height)
{
	//
	// TODO: calculate the splitting parameters for the current task.
	//
	// HINT: You can look in exercise_0.c to get an example for the sequential case.
	//
	int rank;
	int size;
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	// TODO: calculate the number of tasks along X axis and Y axis.
	comm->nb_x = size;
	comm->nb_y = 1;

	// TODO: calculate the current task position in the splitting
	comm->rank_x = rank;
	comm->rank_y = 0;

	if (total_width % size != 0)
        fatal("Width not divisible by number of processes");

	int local_width = total_width / size; 
	// TODO : calculate the local sub-domain size (do not forget the 
	//        ghost cells). Use total_width & total_height as starting 
	//        point.
	comm->width = local_width + 2;  // +2 for ghost cells
	comm->height = total_height+ 2; // +2 for ghost cells

	// TODO : calculate the absolute position in the global mesh.
	//        without accounting the ghost cells
	//        (used to setup the obstable & initial conditions).
	comm->x = rank * local_width;
	comm->y = 0;

	//if debug print comm
	//lbm_comm_print(comm);
}

/****************************************************/
void lbm_comm_ghost_exchange_ex1(lbm_comm_t * comm, lbm_mesh_t * mesh)
{
	//
	// TODO: Implement the 1D communication with blocking MPI functions (MPI_Send & MPI_Recv)
	//
	// To be used:
	//    - DIRECTIONS: the number of doubles composing a cell
	//    - double[DIRECTIONS] lbm_mesh_get_cell(mesh, x, y): function to get the address of a particular cell.
	//    - comm->width : The with of the local sub-domain (containing the ghost cells)
	//    - comm->height : The height of the local sub-domain (containing the ghost cells)
	
	//example to access cell
	//double * cell = lbm_mesh_get_cell(mesh, local_x, local_y);
	//double * cell = lbm_mesh_get_cell(mesh, comm->width - 1, 0);


	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int left  = rank - 1;
	int right = rank + 1;

	if (left < 0) left = MPI_PROC_NULL;
	if (right >= size) right = MPI_PROC_NULL;

	int height = comm->height;

    //LEFT EXCHANGE
    for (int y = 0; y < height; y++)
    {
        double * send_cell = lbm_mesh_get_cell(mesh, 1, y);
        double * recv_cell = lbm_mesh_get_cell(mesh, 0, y);

        MPI_Sendrecv(
            send_cell, DIRECTIONS, MPI_DOUBLE, left, 0,
            recv_cell, DIRECTIONS, MPI_DOUBLE, left, 0,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );
    }

    //RIGHT EXCHANGE
    for (int y = 0; y < height; y++)
    {
        double * send_cell = lbm_mesh_get_cell(mesh, comm->width - 2, y);
        double * recv_cell = lbm_mesh_get_cell(mesh, comm->width - 1, y);

        MPI_Sendrecv(
            send_cell, DIRECTIONS, MPI_DOUBLE, right, 0,
            recv_cell, DIRECTIONS, MPI_DOUBLE, right, 0,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );
    }
}
