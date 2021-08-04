#include <iostream>

#define MAX_VERTICES pow(2,28)

#include "mesh_loop_gpu.h"
#include "gpu_debug_logger.h"

int main(int argc, char **argv)
{
	// Fetch arguments
	if (argc < 3)
	{
		std::cout << "Usage: " << argv[0] << " <filename>.obj <depth> <export=0> <nb_repetitions=16>" << std::endl ;
		return 0 ;
	}

	const std::string f_name(argv[1]) ;
	const uint D = atoi(argv[2]) ;
	const bool enable_export = argc < 4 ? false : atoi(argv[3]) ;
	const uint runCount = argc == 5 ? atoi(argv[4]) : 16 ;

	// Init GL
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "Hello OpenGL Window", nullptr, nullptr);
	if (window == nullptr)
	{
		printf("window creation failed!\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("window creation failed!\n");
		return -1;
	}

#ifndef NDEBUG
	log_debug_output() ;
#endif

	Timings refine_he_time, refine_cr_time, refine_vx_time ;

	// encapsulates what requires GL context
	{
		// Load data
		std::cout << "Loading " << f_name << std::endl ;
		Mesh_Loop_GPU S0(f_name) ;
		if (!S0.is_tri_only())
		{
			std::cerr << "ERROR: The provided mesh should be triangle-only" << std::endl ;
			exit(0) ;
		}
		if (S0.V(D) > MAX_VERTICES)
		{
			std::cout << std::endl << "ERROR: Mesh may exceed memory limits at depth " << D << std::endl ;
			return 0 ;
		}


		assert(S0.check()) ;
		Mesh_Loop_GPU S = S0 ;

		for (int d = 1 ; d <= D ; d++)
		{
			const bool export_to_obj = enable_export && d==D ;
			std::cout << "Subdividing level " << d << "/" << D << "\r" << std::flush ;

			refine_he_time += S.bench_refine_step_gpu(true, false, false, runCount) ;
            refine_cr_time += S.bench_refine_step_gpu(false, true, false, runCount) ;
			refine_vx_time += S.bench_refine_step_gpu(false, false, true, runCount) ;

			S.bench_refine_step_gpu(true, true, true, 1, true, export_to_obj) ;

			if (export_to_obj)
			{
				assert(S.check()) ;

				std::stringstream ss ;
				ss << "S" << d ;
				ss << ".obj" ;

				std::cout << std::endl << "Exporting " << ss.str() << std::endl ;
				S.export_to_obj(ss.str()) ;
			}
		}
		std::cout << std::endl ;
	}

	glfwTerminate();

    std::cout << std::fixed << refine_he_time.median << "\t" << refine_cr_time.median << "\t" << refine_vx_time.median << std::endl ;

	// write into files
	const std::string f_name_tmp = f_name.substr(f_name.find_last_of("\\/") + 1, 999) ;
	const std::string f_name_clean = f_name_tmp.substr(0,f_name_tmp.find_last_of(".")) ;
	const char* num_threads_str = std::getenv("OMP_NUM_THREADS") ;
	int num_threads = 0 ;
	if (num_threads_str != NULL)
		 num_threads = atoi(num_threads_str) ;
//	std::cout << "Using " << num_threads << " threads" << std::endl ;

	std::stringstream fname_he, fname_cr, fname_vx ;
	fname_he << f_name_clean << "_halfedge_" << D << "_" << num_threads << ".txt" ;
	fname_cr << f_name_clean << "_crease_" << D << "_" << num_threads << ".txt" ;
	fname_vx << f_name_clean << "_points_" << D << "_" << num_threads << ".txt" ;

	std::ofstream f_he ;
	f_he.open(fname_he.str()) ;
	f_he << std::fixed << refine_he_time.median << "\t/\t" << refine_he_time.mean << "\t/\t" << refine_he_time.min << "\t/\t" << refine_he_time.max << std::endl ;
	f_he.close() ;

	std::ofstream f_cr ;
	f_cr.open(fname_cr.str()) ;
	f_cr << std::fixed << refine_cr_time.median << "\t/\t" << refine_cr_time.mean << "\t/\t" << refine_cr_time.min << "\t/\t" << refine_cr_time.max << std::endl ;
	f_cr.close() ;

	std::ofstream f_vx ;
	f_vx.open(fname_vx.str()) ;
	f_vx << std::fixed << refine_vx_time.median << "\t/\t" << refine_vx_time.mean << "\t/\t" << refine_vx_time.min << "\t/\t" << refine_vx_time.max << std::endl ;
	f_vx.close() ;

	return 0;
}







