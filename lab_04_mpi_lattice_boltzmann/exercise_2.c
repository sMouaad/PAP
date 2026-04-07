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

    //left
    for (int y = 0; y < height; y++)
    {
        double * send_cell = lbm_mesh_get_cell(mesh, 1, y);
        double * recv_cell = lbm_mesh_get_cell(mesh, 0, y);

        if (rank % 2 == 0)
        {
            MPI_Recv(recv_cell, DIRECTIONS, MPI_DOUBLE, left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(send_cell, DIRECTIONS, MPI_DOUBLE, left, 0, MPI_COMM_WORLD);
        }
        else
        {
            MPI_Send(send_cell, DIRECTIONS, MPI_DOUBLE, left, 0, MPI_COMM_WORLD);
            MPI_Recv(recv_cell, DIRECTIONS, MPI_DOUBLE, left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    //right
    for (int y = 0; y < height; y++)
    {
        double * send_cell = lbm_mesh_get_cell(mesh, comm->width - 2, y);
        double * recv_cell = lbm_mesh_get_cell(mesh, comm->width - 1, y);

        if (rank % 2 == 0)
        {
            MPI_Recv(recv_cell, DIRECTIONS, MPI_DOUBLE, right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(send_cell, DIRECTIONS, MPI_DOUBLE, right, 0, MPI_COMM_WORLD);
        }
        else
        {
            MPI_Send(send_cell, DIRECTIONS, MPI_DOUBLE, right, 0, MPI_COMM_WORLD);
            MPI_Recv(recv_cell, DIRECTIONS, MPI_DOUBLE, right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}
