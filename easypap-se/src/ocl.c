
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "constants.h"
#include "cpustat.h"
#include "debug.h"
#include "error.h"
#include "global.h"
#include "graphics.h"
#include "ocl.h"

#define MAX_PLATFORMS 3
#define MAX_DEVICES 5

unsigned TILEX = 16;
unsigned TILEY = 16;
unsigned SIZE  = 0;

static size_t max_workgroup_size = 0;

cl_int err;
static cl_device_id devices[MAX_DEVICES];
static cl_program program; // compute program
static unsigned dev = 0;

cl_context context;
cl_kernel update_kernel;
cl_kernel compute_kernel;
cl_command_queue queue;
cl_mem tex_buffer, cur_buffer, next_buffer;

static size_t file_size (const char *filename)
{
  struct stat sb;

  if (stat (filename, &sb) < 0)
    exit_with_error ("Cannot access \"%s\" kernel file (%s)", filename, strerror (errno));

  return sb.st_size;
}

static char *file_load (const char *filename)
{
  FILE *f;
  char *b;
  size_t s;
  size_t r;

  s = file_size (filename);
  b = malloc (s + 1);
  if (!b)
    exit_with_error ("Malloc failed (%s)", strerror (errno));

  f = fopen (filename, "r");
  if (f == NULL)
    exit_with_error ("Cannot open \"%s\" file (%s)", filename, strerror (errno));

  r = fread (b, s, 1, f);
  if (r != 1)
    exit_with_error ("fread failed (%s)", strerror (errno));

  b[s] = '\0';

  return b;
}

static void ocl_acquire (void)
{
  cl_int err;

  err = clEnqueueAcquireGLObjects (queue, 1, &tex_buffer, 0, NULL, NULL);
  check (err, "Failed to acquire lock");
}

static void ocl_release (void)
{
  cl_int err;

  err = clEnqueueReleaseGLObjects (queue, 1, &tex_buffer, 0, NULL, NULL);
  check (err, "Failed to release lock");
}

void ocl_init (void)
{
  char name[1024], vendor[1024];
  cl_platform_id pf[MAX_PLATFORMS];
  cl_uint nb_platforms = 0;

  cl_device_type dtype;
  cl_uint nb_devices   = 0;
  char *str            = NULL;
  unsigned platform_no = 0;

  str = getenv ("PLATFORM");
  if (str != NULL)
    platform_no = atoi (str);

  str = getenv ("DEVICE");
  if (str != NULL)
    dev = atoi (str);

  str = getenv ("SIZE");
  if (str != NULL)
    SIZE = atoi (str);
  else
    SIZE = DIM;

  if (SIZE > DIM)
    exit_with_error ("SIZE (%d) cannot exceed DIM (%d)", SIZE, DIM);

  // Get list of OpenCL platforms detected
  //
  err = clGetPlatformIDs (MAX_PLATFORMS, pf, &nb_platforms);
  check (err, "Failed to get platform IDs");

  PRINT_DEBUG ('o', "%d OpenCL platforms detected:\n", nb_platforms);

  if (platform_no >= nb_platforms)
    exit_with_error ("Platform number #%d too high", platform_no);

  err = clGetPlatformInfo (pf[platform_no], CL_PLATFORM_NAME, 1024, name, NULL);
  check (err, "Failed to get Platform Info");

  err = clGetPlatformInfo (pf[platform_no], CL_PLATFORM_VENDOR, 1024, vendor,
                           NULL);
  check (err, "Failed to get Platform Info");

  printf ("Using platform %d: %s - %s\n", platform_no, name, vendor);

  // Get list of devices
  //
  err = clGetDeviceIDs (pf[platform_no], CL_DEVICE_TYPE_GPU, MAX_DEVICES,
                        devices, &nb_devices);
  PRINT_DEBUG ('o', "nb devices = %d\n", nb_devices);

  if (nb_devices == 0) {
    exit_with_error ("No appropriate device found on platform %d (%s - %s). "
                     "Try PLATFORM=<p> ./run args...",
                     platform_no, name, vendor);
  }
  if (dev >= nb_devices)
    exit_with_error ("Device number #%d too high", dev);

  err = clGetDeviceInfo (devices[dev], CL_DEVICE_NAME, 1024, name, NULL);
  check (err, "Cannot get type of device");

  err = clGetDeviceInfo (devices[dev], CL_DEVICE_TYPE, sizeof (cl_device_type),
                         &dtype, NULL);
  check (err, "Cannot get type of device");

  printf ("Using Device %d: %s [%s]\n", dev,
          (dtype == CL_DEVICE_TYPE_GPU) ? "GPU" : "CPU", name);

  err = clGetDeviceInfo (devices[dev], CL_DEVICE_MAX_WORK_GROUP_SIZE,
                         sizeof (size_t), &max_workgroup_size, NULL);
  check (err, "Cannot get max workgroup size");

#ifdef ENABLE_SDL
  if (do_display) {
#ifdef __APPLE__
    CGLContextObj cgl_context          = CGLGetCurrentContext ();
    CGLShareGroupObj sharegroup        = CGLGetShareGroup (cgl_context);
    cl_context_properties properties[] = {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
        (cl_context_properties)sharegroup, 0};
#else
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR,
        (cl_context_properties)glXGetCurrentContext (),
        CL_GLX_DISPLAY_KHR,
        (cl_context_properties)glXGetCurrentDisplay (),
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)pf[platform_no],
        0};
#endif

    context = clCreateContext (properties, 1, &devices[dev], NULL, NULL, &err);
  } else
#endif // ENABLE_SDL
    context = clCreateContext (NULL, 1, &devices[dev], NULL, NULL, &err);


  check (err, "Failed to create compute context");

  // Create a command queue
  //
  queue = clCreateCommandQueue (context, devices[dev],
                                CL_QUEUE_PROFILING_ENABLE, &err);
  check (err, "Failed to create command queue");

  PRINT_DEBUG ('i', "Init phase 2: OpenCL initialized\n");
}

void ocl_alloc_buffers (void)
{
  // Allocate buffers inside device memory
  //
  cur_buffer = clCreateBuffer (context, CL_MEM_READ_WRITE,
                               sizeof (unsigned) * DIM * DIM, NULL, NULL);
  if (!cur_buffer)
    exit_with_error ("Failed to allocate input buffer");

  next_buffer = clCreateBuffer (context, CL_MEM_READ_WRITE,
                                sizeof (unsigned) * DIM * DIM, NULL, NULL);
  if (!next_buffer)
    exit_with_error ("Failed to allocate output buffer");
}

void ocl_map_textures (GLuint texid)
{
// Shared texture buffer with OpenGL
//
#ifdef ENABLE_SDL
  tex_buffer = clCreateFromGLTexture (context, CL_MEM_READ_WRITE, GL_TEXTURE_2D,
                                      0, texid, &err);
#else
  err = 1;
#endif
  check (err, "Failed to map texture buffer\n");
}

void ocl_send_image (unsigned *image)
{
  // Load program source into memory
  //
  {
    char kernel_file[1024];

    sprintf (kernel_file, "kernel/ocl/%s.cl", kernel_name);
    const char *opencl_prog = file_load (kernel_file);

    // Attach program source to context
    //
    program = clCreateProgramWithSource (context, 1, &opencl_prog, NULL, &err);
    check (err, "Failed to create program");
  }

  {
    char *str = NULL;

    str = getenv ("TILEX");
    if (str != NULL)
      TILEX = atoi (str);
    else
      TILEX = 16;

    str = getenv ("TILEY");
    if (str != NULL)
      TILEY = atoi (str);
    else
      TILEY = TILEX;
  }

  {
    // Compile program
    //
    char flags[1024];

    if (draw_param)
      sprintf (flags,
               "-cl-mad-enable -cl-fast-relaxed-math"
               " -DDIM=%d -DSIZE=%d -DTILEX=%d -DTILEY=%d -DKERNEL_%s"
               " -DPARAM=%s",
               DIM, SIZE, TILEX, TILEY, kernel_name, draw_param);
    else
      sprintf (flags,
               "-cl-mad-enable -cl-fast-relaxed-math"
               " -DDIM=%d -DSIZE=%d -DTILEX=%d -DTILEY=%d -DKERNEL_%s",
               DIM, SIZE, TILEX, TILEY, kernel_name);

    err = clBuildProgram (program, 0, NULL, flags, NULL, NULL);
    // Display compiler log
    //
    {
      size_t len;

      clGetProgramBuildInfo (program, devices[dev], CL_PROGRAM_BUILD_LOG, 0,
                             NULL, &len);

      if (len >= 1 && len <= 2048) {
        char buffer[len];

        fprintf (stderr, "--- OpenCL Compiler log ---\n");
        clGetProgramBuildInfo (program, devices[dev], CL_PROGRAM_BUILD_LOG,
                               sizeof (buffer), buffer, NULL);
        fprintf (stderr, "%s\n", buffer);
        fprintf (stderr, "---------------------------\n");
      }
    }

    if (err != CL_SUCCESS)
      exit_with_error ("Failed to build program");
  }

  // Create the compute kernel in the program we wish to run
  //
  {
    char name[1024];

    sprintf (name, "%s_%s", kernel_name, variant_name);
    compute_kernel = clCreateKernel (program, name, &err);
    check (err, "Failed to create compute kernel <%s>", name);

    PRINT_DEBUG ('o', "Using OpenCL kernel: %s\n", variant_name);

    sprintf (name, "%s_update_texture", kernel_name);

    // First look for kernel-specific version of update_texture
    update_kernel = clCreateKernel (program, name, &err);
    if (err != CL_SUCCESS) {
      // Fall back to generic version
      update_kernel = clCreateKernel (program, "update_texture", &err);
      check (err, "Failed to create update kernel <update_texture>");
    }
  }

  printf ("Using %dx%d workitems grouped in %dx%d tiles \n", SIZE, SIZE, TILEX,
          TILEY);

  err = clEnqueueWriteBuffer (queue, cur_buffer, CL_TRUE, 0,
                              sizeof (unsigned) * DIM * DIM, image, 0, NULL,
                              NULL);
  check (err, "Failed to write to cur_buffer");

  err = clEnqueueWriteBuffer (queue, next_buffer, CL_TRUE, 0,
                              sizeof (unsigned) * DIM * DIM, alt_image, 0, NULL,
                              NULL);
  check (err, "Failed to write to next_buffer");

  PRINT_DEBUG (
      'i', "Init phase 7 : Initial image data transferred to OpenCL device\n");
}

void ocl_retrieve_image (unsigned *image)
{
  err =
      clEnqueueReadBuffer (queue, cur_buffer, CL_TRUE, 0,
                           sizeof (unsigned) * DIM * DIM, image, 0, NULL, NULL);
  check (err, "Failed to read from cur_buffer");

  PRINT_DEBUG ('o', "Final image retrieved from device.\n");
}

static cl_event prof_event;

unsigned ocl_invoke_kernel_generic (unsigned nb_iter)
{
  size_t global[2] = {SIZE, SIZE};   // global domain size for our calculation
  size_t local[2]  = {TILEX, TILEY}; // local domain size for our calculation

  for (unsigned it = 1; it <= nb_iter; it++) {

    // Set kernel arguments
    //
    err = 0;
    err |= clSetKernelArg (compute_kernel, 0, sizeof (cl_mem), &cur_buffer);
    err |= clSetKernelArg (compute_kernel, 1, sizeof (cl_mem), &next_buffer);
    check (err, "Failed to set kernel arguments");

    err = clEnqueueNDRangeKernel (queue, compute_kernel, 2, NULL, global, local,
                                  0, NULL, &prof_event);
    check (err, "Failed to execute kernel");

    // Swap buffers
    {
      cl_mem tmp  = cur_buffer;
      cur_buffer  = next_buffer;
      next_buffer = tmp;
    }
  }

  return 0;
}

void ocl_wait (void)
{
  cl_ulong start, end;
  // Wait for the command commands to get serviced before reading back results
  //
  clFinish (queue);

  clGetEventProfilingInfo (prof_event, CL_PROFILING_COMMAND_START,
                           sizeof (cl_ulong), &start, NULL);
  clGetEventProfilingInfo (prof_event, CL_PROFILING_COMMAND_END,
                           sizeof (cl_ulong), &end, NULL);
  clReleaseEvent (prof_event);
#if 0
  cl_ulong elapsed = end - start;
  printf ("Last kernel started at %lld and finished at %lld (duration = %lld)\n", start, end, elapsed);
  printf ("time now = %ld\n", what_time_is_it ());
#endif
}

void ocl_update_texture (void)
{
  size_t global[2] = {DIM, DIM}; // global domain size for our calculation
  size_t local[2]  = {16, 16};   // local domain size for our calculation

  ocl_acquire ();

  // Set kernel arguments
  //
  err = 0;
  err |= clSetKernelArg (update_kernel, 0, sizeof (cl_mem), &cur_buffer);
  err |= clSetKernelArg (update_kernel, 1, sizeof (cl_mem), &tex_buffer);
  check (err, "Failed to set kernel arguments");

  err = clEnqueueNDRangeKernel (queue, update_kernel, 2, NULL, global, local, 0,
                                NULL, NULL);
  check (err, "Failed to execute kernel");

  ocl_release ();

  clFinish (queue);
}

size_t ocl_get_max_workgroup_size (void)
{
  return max_workgroup_size;
}
