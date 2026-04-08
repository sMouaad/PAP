/*****************************************************
    AUTHOR  : Sébastien Valat
    MAIL    : sebastien.valat@univ-grenoble-alpes.fr
    LICENSE : BSD
    YEAR    : 2021
    COURSE  : Parallel Algorithms and Programming
*****************************************************/

//////////////////////////////////////////////////////
//
// Goal: Implement 2D grid communication with non-blocking
//       messages.
//
// SUMMARY:
//     - 2D splitting along X and Y
//     - 8 neighbors communications
//     - MPI type for non contiguous cells
// NEW:
//     - Non-blocking communications
//
//////////////////////////////////////////////////////

/****************************************************/
#include "src/lbm_struct.h"
#include "src/exercises.h"

/****************************************************/
static int get_rank_at(MPI_Comm cart_comm, int rx, int ry, int nb_x, int nb_y)
{
	if (rx < 0 || rx >= nb_x || ry < 0 || ry >= nb_y)
		return MPI_PROC_NULL;
	int coords[2] = {rx, ry};
	int rank;
	MPI_Cart_rank(cart_comm, coords, &rank);
	return rank;
}

/****************************************************/
void lbm_comm_init_ex6(lbm_comm_t * comm, int total_width, int total_height)
{
	//we use the same implementation than ex5
	lbm_comm_init_ex5(comm, total_width, total_height);
}

/****************************************************/
void lbm_comm_release_ex6(lbm_comm_t * comm)
{
	//we use the same implementation than ext 5
	lbm_comm_release_ex5(comm);
}

/****************************************************/
void lbm_comm_ghost_exchange_ex6(lbm_comm_t * comm, lbm_mesh_t * mesh)
{
	int w = comm->width;
	int h = comm->height;

	int left, right, up, down;
	MPI_Cart_shift(comm->communicator, 0, 1, &left, &right);
	MPI_Cart_shift(comm->communicator, 1, 1, &up, &down);

	int top_left     = get_rank_at(comm->communicator, comm->rank_x - 1, comm->rank_y - 1, comm->nb_x, comm->nb_y);
	int top_right    = get_rank_at(comm->communicator, comm->rank_x + 1, comm->rank_y - 1, comm->nb_x, comm->nb_y);
	int bottom_left  = get_rank_at(comm->communicator, comm->rank_x - 1, comm->rank_y + 1, comm->nb_x, comm->nb_y);
	int bottom_right = get_rank_at(comm->communicator, comm->rank_x + 1, comm->rank_y + 1, comm->nb_x, comm->nb_y);

	int col_count = h * DIRECTIONS;
	MPI_Request reqs[8];

	// --- Batch 1: cardinal directions (left/right/top/bottom) ---
	// left/right (contiguous columns)
	MPI_Irecv(lbm_mesh_get_cell(mesh, w - 1, 0), col_count, MPI_DOUBLE, right, 0, comm->communicator, &reqs[0]);
	MPI_Irecv(lbm_mesh_get_cell(mesh, 0, 0), col_count, MPI_DOUBLE, left, 1, comm->communicator, &reqs[1]);
	MPI_Isend(lbm_mesh_get_cell(mesh, 1, 0), col_count, MPI_DOUBLE, left, 0, comm->communicator, &reqs[2]);
	MPI_Isend(lbm_mesh_get_cell(mesh, w - 2, 0), col_count, MPI_DOUBLE, right, 1, comm->communicator, &reqs[3]);

	// top/bottom (non-contiguous, use MPI type)
	MPI_Irecv(lbm_mesh_get_cell(mesh, 0, h - 1), 1, comm->type, down, 2, comm->communicator, &reqs[4]);
	MPI_Irecv(lbm_mesh_get_cell(mesh, 0, 0), 1, comm->type, up, 3, comm->communicator, &reqs[5]);
	MPI_Isend(lbm_mesh_get_cell(mesh, 0, 1), 1, comm->type, up, 2, comm->communicator, &reqs[6]);
	MPI_Isend(lbm_mesh_get_cell(mesh, 0, h - 2), 1, comm->type, down, 3, comm->communicator, &reqs[7]);

	MPI_Waitall(8, reqs, MPI_STATUSES_IGNORE);

	// --- Batch 2: diagonal corners ---
	MPI_Irecv(lbm_mesh_get_cell(mesh, w - 1, h - 1), DIRECTIONS, MPI_DOUBLE, bottom_right, 4, comm->communicator, &reqs[0]);
	MPI_Irecv(lbm_mesh_get_cell(mesh, 0, h - 1), DIRECTIONS, MPI_DOUBLE, bottom_left, 5, comm->communicator, &reqs[1]);
	MPI_Irecv(lbm_mesh_get_cell(mesh, w - 1, 0), DIRECTIONS, MPI_DOUBLE, top_right, 6, comm->communicator, &reqs[2]);
	MPI_Irecv(lbm_mesh_get_cell(mesh, 0, 0), DIRECTIONS, MPI_DOUBLE, top_left, 7, comm->communicator, &reqs[3]);

	MPI_Isend(lbm_mesh_get_cell(mesh, 1, 1), DIRECTIONS, MPI_DOUBLE, top_left, 4, comm->communicator, &reqs[4]);
	MPI_Isend(lbm_mesh_get_cell(mesh, w - 2, 1), DIRECTIONS, MPI_DOUBLE, top_right, 5, comm->communicator, &reqs[5]);
	MPI_Isend(lbm_mesh_get_cell(mesh, 1, h - 2), DIRECTIONS, MPI_DOUBLE, bottom_left, 6, comm->communicator, &reqs[6]);
	MPI_Isend(lbm_mesh_get_cell(mesh, w - 2, h - 2), DIRECTIONS, MPI_DOUBLE, bottom_right, 7, comm->communicator, &reqs[7]);

	MPI_Waitall(8, reqs, MPI_STATUSES_IGNORE);
}
