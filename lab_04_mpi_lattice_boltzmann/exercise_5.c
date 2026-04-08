/*****************************************************
    AUTHOR  : Sébastien Valat
    MAIL    : sebastien.valat@univ-grenoble-alpes.fr
    LICENSE : BSD
    YEAR    : 2021
    COURSE  : Parallel Algorithms and Programming
*****************************************************/

//////////////////////////////////////////////////////
//
// Goal: Implement 2D grid communication scheme with
//      8 neighbors using MPI types for non contiguous
//      side.
//
// SUMMARY:
//     - 2D splitting along X and Y
//     - 8 neighbors communications
//     - Blocking communications
// NEW:
//     - >>> MPI type for non contiguous cells <<<
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
void lbm_comm_init_ex5(lbm_comm_t * comm, int total_width, int total_height)
{
	lbm_comm_init_ex4(comm, total_width, total_height);

	// type to describe a row: width blocks of DIRECTIONS doubles,
	// spaced by height*DIRECTIONS doubles (column-major stride)
	MPI_Type_vector(comm->width, DIRECTIONS, comm->height * DIRECTIONS, MPI_DOUBLE, &comm->type);
	MPI_Type_commit(&comm->type);
}

/****************************************************/
void lbm_comm_release_ex5(lbm_comm_t * comm)
{
	MPI_Type_free(&comm->type);
	lbm_comm_release_ex4(comm);
}

/****************************************************/
void lbm_comm_ghost_exchange_ex5(lbm_comm_t * comm, lbm_mesh_t * mesh)
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

	// LEFT/RIGHT: contiguous columns
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, 1, 0), col_count, MPI_DOUBLE, left, 0,
	             lbm_mesh_get_cell(mesh, w - 1, 0), col_count, MPI_DOUBLE, right, 0,
	             comm->communicator, MPI_STATUS_IGNORE);
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, w - 2, 0), col_count, MPI_DOUBLE, right, 1,
	             lbm_mesh_get_cell(mesh, 0, 0), col_count, MPI_DOUBLE, left, 1,
	             comm->communicator, MPI_STATUS_IGNORE);

	// TOP/BOTTOM: use MPI type for non-contiguous row
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, 0, 1), 1, comm->type, up, 2,
	             lbm_mesh_get_cell(mesh, 0, h - 1), 1, comm->type, down, 2,
	             comm->communicator, MPI_STATUS_IGNORE);
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, 0, h - 2), 1, comm->type, down, 3,
	             lbm_mesh_get_cell(mesh, 0, 0), 1, comm->type, up, 3,
	             comm->communicator, MPI_STATUS_IGNORE);

	// DIAGONALS: single corner cells
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, 1, 1), DIRECTIONS, MPI_DOUBLE, top_left, 4,
	             lbm_mesh_get_cell(mesh, w - 1, h - 1), DIRECTIONS, MPI_DOUBLE, bottom_right, 4,
	             comm->communicator, MPI_STATUS_IGNORE);
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, w - 2, 1), DIRECTIONS, MPI_DOUBLE, top_right, 5,
	             lbm_mesh_get_cell(mesh, 0, h - 1), DIRECTIONS, MPI_DOUBLE, bottom_left, 5,
	             comm->communicator, MPI_STATUS_IGNORE);
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, 1, h - 2), DIRECTIONS, MPI_DOUBLE, bottom_left, 6,
	             lbm_mesh_get_cell(mesh, w - 1, 0), DIRECTIONS, MPI_DOUBLE, top_right, 6,
	             comm->communicator, MPI_STATUS_IGNORE);
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, w - 2, h - 2), DIRECTIONS, MPI_DOUBLE, bottom_right, 7,
	             lbm_mesh_get_cell(mesh, 0, 0), DIRECTIONS, MPI_DOUBLE, top_left, 7,
	             comm->communicator, MPI_STATUS_IGNORE);
}
