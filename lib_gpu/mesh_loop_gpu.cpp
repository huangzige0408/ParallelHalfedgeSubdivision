#include "mesh_loop_gpu.h"

#define DJ_OPENGL_IMPLEMENTATION
#include "dj_opengl.h"

Mesh_Loop_GPU::Mesh_Loop_GPU(int H, int V, int E, int F):
	Mesh_Loop(H,V,E,F)
{
	init_buffers() ;
}


Mesh_Loop_GPU::Mesh_Loop_GPU(const std::string& filename):
	Mesh_Loop(filename)
{
	init_buffers() ;
}

Mesh_Loop_GPU::~Mesh_Loop_GPU()
{
	release_buffers() ;
}

void
Mesh_Loop_GPU::init_buffers()
{
	halfedges_gpu = create_buffer(BUFFER_HALFEDGES_IN, Hd * sizeof(HalfEdge),halfedges.data(),false) ;
}

void
Mesh_Loop_GPU::release_buffers()
{
	release_buffer(halfedges_gpu) ;
}

GLuint
Mesh_Loop_GPU::create_buffer(GLuint buffer_bind_id, uint size, void* data, bool clear_buffer)
{
	GLuint new_buffer ;

	glGenBuffers(1, &new_buffer) ;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, new_buffer) ;
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, data, GL_MAP_READ_BIT) ; // TODO remove READ_BIT ?
	if (clear_buffer)
        assert(false) ;
        //	glClearNamedBufferData(new_buffer,GL_R32F,GL_RED,GL_FLOAT,nullptr) ;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_bind_id, new_buffer) ; // allows to be read in shader

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0) ;

	return new_buffer ;
}

void
Mesh_Loop_GPU::release_buffer(GLuint buffer)
{
	glDeleteBuffers(1, &buffer) ;
}



void
Mesh_Loop_GPU::readback_buffers()
{
	HalfEdge* data = (HalfEdge*) glMapNamedBuffer(halfedges_gpu, GL_READ_ONLY) ;

	halfedges.resize(Hd) ;
    memcpy(&(halfedges[0]), data, Hd * sizeof(HalfEdge)) ;

	glUnmapNamedBuffer(halfedges_gpu) ;
}

GLuint
Mesh_Loop_GPU::create_program_refine_halfedges(GLuint halfedges_gpu_in, GLuint halfedges_gpu_out)
{
	GLuint refine_halfedges_program = glCreateProgram() ;

	djg_program* builder = djgp_create() ;
	djgp_push_string(builder,"#define HALFEDGE_BUFFER_IN %d\n", halfedges_gpu_in) ;
	djgp_push_string(builder,"#define HALFEDGE_BUFFER_OUT %d\n", halfedges_gpu_out) ;
    //djgp_push_string(builder, "#extension GL_NV_shader_atomic_float: require\n");

    djgp_push_file(builder, "../shaders/loop_refine_halfedges.glsl") ;
	djgp_to_gl(builder, 450, false, true, &refine_halfedges_program) ;

	djgp_release(builder) ;

	return refine_halfedges_program ;
}

void
Mesh_Loop_GPU::refine_step_gpu()
{
    const uint new_depth = depth() ; // todo reset + 1 ;

	// ensure input is bound to BUFFER_HALFEDGES_IN
	if (depth() > 0)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, halfedges_gpu) ;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_HALFEDGES_IN, halfedges_gpu) ;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0) ;
	}

	// create new halfedge buffer
	const GLuint H_new_gpu = create_buffer(BUFFER_HALFEDGES_OUT, H(new_depth) * sizeof(HalfEdge), nullptr, false) ;

    // create program for halfedge refinement
	const GLuint refine_halfedges_gpu = create_program_refine_halfedges(BUFFER_HALFEDGES_IN, BUFFER_HALFEDGES_OUT) ;
    glUseProgram(refine_halfedges_gpu) ;

    const GLint u_Hd = glGetUniformLocation(refine_halfedges_gpu, "Hd");
	glUniform1i(u_Hd, Hd) ;

    // execute program
    const uint n_dispatch_groups = std::ceil(Hd / 256.0f) ;
	glDispatchCompute(n_dispatch_groups,1,1) ;
	glMemoryBarrier(GL_ALL_BARRIER_BITS) ;

    // cleanup
    glDeleteProgram(refine_halfedges_gpu) ;
    release_buffer(halfedges_gpu) ;

    // save new state
    set_depth(new_depth) ;
    halfedges_gpu = H_new_gpu ;
    readback_buffers() ;
}