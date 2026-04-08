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
//       8 neighbors using manual copy for non
//       contiguous side and blocking communications
//
// SUMMARY:
//     - 2D splitting along X and Y
//     - 8 neighbors communications
//     - Blocking communications
//     - Manual copy for non continguous cells
//
//////////////////////////////////////////////////////

/****************************************************/
#include "src/lbm_struct.h"
#include "src/exercises.h"
#include <string.h>

/****************************************************/
// helper to get rank of a neighbor at given grid coords, MPI_PROC_NULL if out of bounds
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
void lbm_comm_init_ex4(lbm_comm_t * comm, int total_width, int total_height)
{
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// let MPI figure out a 2D decomposition
	int dims[2] = {0, 0};
	MPI_Dims_create(size, 2, dims);
	comm->nb_x = dims[0];
	comm->nb_y = dims[1];

	if (total_width % comm->nb_x != 0)
		fatal("Width not divisible by nb_x");
	if (total_height % comm->nb_y != 0)
		fatal("Height not divisible by nb_y");

	// create cartesian communicator, non-periodic
	int periods[2] = {0, 0};
	MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &comm->communicator);

	int coords[2];
	MPI_Cart_coords(comm->communicator, rank, 2, coords);
	comm->rank_x = coords[0];
	comm->rank_y = coords[1];

	int local_width  = total_width / comm->nb_x;
	int local_height = total_height / comm->nb_y;
	comm->width  = local_width + 2;
	comm->height = local_height + 2;

	comm->x = comm->rank_x * local_width;
	comm->y = comm->rank_y * local_height;

	// allocate temp buffers for non-contiguous top/bottom rows
	comm->buffer_send_up   = malloc(comm->width * DIRECTIONS * sizeof(double));
	comm->buffer_send_down = malloc(comm->width * DIRECTIONS * sizeof(double));
	comm->buffer_recv_up   = malloc(comm->width * DIRECTIONS * sizeof(double));
	comm->buffer_recv_down = malloc(comm->width * DIRECTIONS * sizeof(double));
}

/****************************************************/
void lbm_comm_release_ex4(lbm_comm_t * comm)
{
	free(comm->buffer_send_up);
	free(comm->buffer_send_down);
	free(comm->buffer_recv_up);
	free(comm->buffer_recv_down);
}

/****************************************************/
void lbm_comm_ghost_exchange_ex4(lbm_comm_t * comm, lbm_mesh_t * mesh)
{
	int w = comm->width;
	int h = comm->height;

	// cardinal neighbors
	int left, right, up, down;
	MPI_Cart_shift(comm->communicator, 0, 1, &left, &right);
	MPI_Cart_shift(comm->communicator, 1, 1, &up, &down);

	// diagonal neighbors
	int top_left     = get_rank_at(comm->communicator, comm->rank_x - 1, comm->rank_y - 1, comm->nb_x, comm->nb_y);
	int top_right    = get_rank_at(comm->communicator, comm->rank_x + 1, comm->rank_y - 1, comm->nb_x, comm->nb_y);
	int bottom_left  = get_rank_at(comm->communicator, comm->rank_x - 1, comm->rank_y + 1, comm->nb_x, comm->nb_y);
	int bottom_right = get_rank_at(comm->communicator, comm->rank_x + 1, comm->rank_y + 1, comm->nb_x, comm->nb_y);

	int col_count = h * DIRECTIONS;
	int row_count = w * DIRECTIONS;

	// --- LEFT/RIGHT: columns are contiguous in memory ---
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, 1, 0), col_count, MPI_DOUBLE, left, 0,
	             lbm_mesh_get_cell(mesh, w - 1, 0), col_count, MPI_DOUBLE, right, 0,
	             comm->communicator, MPI_STATUS_IGNORE);
	MPI_Sendrecv(lbm_mesh_get_cell(mesh, w - 2, 0), col_count, MPI_DOUBLE, right, 1,
	             lbm_mesh_get_cell(mesh, 0, 0), col_count, MPI_DOUBLE, left, 1,
	             comm->communicator, MPI_STATUS_IGNORE);

	// --- TOP/BOTTOM: rows are NOT contiguous, manual copy needed ---
	// pack top real row (y=1)
	for (int x = 0; x < w; x++)
		memcpy(&comm->buffer_send_up[x * DIRECTIONS], lbm_mesh_get_cell(mesh, x, 1), DIRECTIONS * sizeof(double));
	// pack bottom real row (y=h-2)
	for (int x = 0; x < w; x++)
		memcpy(&comm->buffer_send_down[x * DIRECTIONS], lbm_mesh_get_cell(mesh, x, h - 2), DIRECTIONS * sizeof(double));

	// exchange top row
	MPI_Sendrecv(comm->buffer_send_up, row_count, MPI_DOUBLE, up, 2,
	             comm->buffer_recv_down, row_count, MPI_DOUBLE, down, 2,
	             comm->communicator, MPI_STATUS_IGNORE);
	// exchange bottom row
	MPI_Sendrecv(comm->buffer_send_down, row_count, MPI_DOUBLE, down, 3,
	             comm->buffer_recv_up, row_count, MPI_DOUBLE, up, 3,
	             comm->communicator, MPI_STATUS_IGNORE);

	// unpack into ghost rows
	if (down != MPI_PROC_NULL)
		for (int x = 0; x < w; x++)
			memcpy(lbm_mesh_get_cell(mesh, x, h - 1), &comm->buffer_recv_down[x * DIRECTIONS], DIRECTIONS * sizeof(double));
	if (up != MPI_PROC_NULL)
		for (int x = 0; x < w; x++)
			memcpy(lbm_mesh_get_cell(mesh, x, 0), &comm->buffer_recv_up[x * DIRECTIONS], DIRECTIONS * sizeof(double));

	// --- DIAGONALS: single corner cells ---
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
