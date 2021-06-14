#include "mesh.h"

void
MeshSubdivision::refine_step()
{
	halfedge_buffer H_new ;
	crease_buffer C_new ;
	vertex_buffer V_new ;

	H_new.resize(H(depth + 1));
	C_new.resize(C(depth + 1));
	V_new.resize(V(depth + 1),{0.,0.,0.});

	refine_halfedges(H_new) ;
	refine_creases(C_new) ;
	refine_vertices_with_creases(V_new) ;

	halfedges = H_new ;
	creases = C_new ;
	vertices = V_new ;

	depth++ ;
}

void
MeshSubdivision::refine_step_inplace()
{
	halfedge_buffer H_new ;
	crease_buffer C_new ;
	vertex_buffer& V_new = this->vertices ;

	H_new.resize(H(depth + 1));
	C_new.resize(C(depth + 1));
	V_new.resize(V(depth + 1),{0.,0.,0.});

	refine_halfedges(H_new) ;
	refine_creases(C_new) ;
	refine_vertices_inplace() ;

	halfedges = H_new ;
	creases = C_new ;

	depth++ ;
}

void
MeshSubdivision::refine_creases(crease_buffer& C_new) const
{
	const int Cd = C(depth) ;
CC_PARALLEL_FOR
	for (int c = 0; c < Cd; ++c)
	{
		if (is_crease_edge(c))
		{
			const int sharpness = Sigma(c) ;
			const int c_next = NextC(c) ;
			const int c_prev = PrevC(c) ;

			const float c0_Sigma = std::max(0., (Sigma(c_prev) + 3. * sharpness ) / 4. - 1.) ;
			const float c1_Sigma = std::max(0., (Sigma(NextC(c)) + 3. * sharpness ) / 4. - 1.) ;

			const int c0_Next = 2*c + 1 ;
			const int c1_Next = 2*c_next + (c == PrevC(c_next) ? 0 : 1.) ;

			const int c0_Prev = 2*c_prev + (c == NextC(c_prev) ? 1. : 0) ;
			const int c1_Prev = 2*c ;

			C_new[2*c + 0] = Crease(c0_Sigma, c0_Next, c0_Prev) ;
			C_new[2*c + 1] = Crease(c1_Sigma, c1_Next, c1_Prev) ;
		}
	}
CC_BARRIER
}


double
MeshSubdivision::bench_refine_step(bool refine_he, bool refine_cr, bool refine_vx, uint repetitions)
{
	duration total_time(0) ;

	if (refine_he)
	{
		halfedge_buffer H_new ;
		H_new.resize(H(depth + 1));

		for (uint i = 0 ; i < repetitions; ++i)
		{
			auto start = timer::now() ;
			refine_halfedges(H_new) ;
			auto stop = timer::now() ;

			total_time += stop - start;
		}
	}

	if (refine_cr)
	{
		crease_buffer C_new ;
		C_new.resize(C(depth + 1));

		for (uint i = 0 ; i < repetitions; ++i)
		{
			auto start = timer::now() ;
			refine_creases(C_new) ;
			auto stop = timer::now() ;

			total_time += stop - start;
		}
	}

	if (refine_vx)
	{
		vertex_buffer V_new ;
		V_new.resize(V(depth + 1));

		for (uint i = 0 ; i < repetitions; ++i)
		{
			std::fill(V_new.begin(), V_new.end(), vec3({0.,0.,0.})); // important: include memset for realtime scenario
			auto start = timer::now() ;
			refine_vertices_with_creases(V_new) ;
			auto stop = timer::now() ;

			total_time += stop - start;
		}
	}

	return total_time.count() / double(repetitions) ;
}
