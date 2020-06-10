#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <tuple>
#include <cmath>

#include <CL/cl.h>

#include <compiler.hpp>
#include <vcd.hpp>
 
#define MAX_SOURCE_SIZE (0x100000)

#define CPU_GPU_DEF CL_DEVICE_TYPE_GPU

#define WIRECOUNT 13

short wireValues[WIRECOUNT];
// GlobalEvent = { timestamp, workgroup_id, logic_block, wire_signal }

char to_char(short s) {
    char c = '0'+s;
    return c;
}

const char *getErrorString(cl_int error)
{
switch(error){
    // run-time and JIT compiler errors
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

    // compile-time errors
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

    // extension errors
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
    default: return "Unknown OpenCL error";
    }
}
 
int main(int argc, char** argv) {
    if (argc<2) return 1;
    std::ios::sync_with_stdio(0); std::cout.tie(0); std::cin.tie(0);

    srand(time(0));

    // Create the input vectors
    int i;
    const int LIST_SIZE = atoi(argv[1]);
    short *I0 = (short*)malloc(sizeof(short)*LIST_SIZE);
    short *I1 = (short*)malloc(sizeof(short)*LIST_SIZE);
    short *I2 = (short*)malloc(sizeof(short)*LIST_SIZE);
    short *I3 = (short*)malloc(sizeof(short)*LIST_SIZE);
    short *S0 = (short*)malloc(sizeof(short)*LIST_SIZE);
    short *S1 = (short*)malloc(sizeof(short)*LIST_SIZE);

    for (int i=1; i<LIST_SIZE; ++i) {
        I0[i]=rand()%2;
        I1[i]=rand()%2;
        I2[i]=rand()%2;
        I3[i]=rand()%2;
        S0[i]=rand()%2;
        S1[i]=rand()%2;
    }
 
    // Load the kernel source code into the array source_str
    FILE *fp;
    char *source_str;
    size_t source_size;
 
    fp = fopen("mux_kernel.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose( fp );
 
    // Get platform and device information
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;   
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    ret = clGetDeviceIDs( platform_id, CPU_GPU_DEF, 1, 
            &device_id, &ret_num_devices);
    if(ret != CL_SUCCESS)
        std::cerr << getErrorString(ret) << std::endl;
 
    // Create an OpenCL context
    cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
 
    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
 
    // Create memory buffers on the device for each vector 
    cl_mem I0_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            LIST_SIZE * sizeof(short), NULL, &ret);
    cl_mem I1_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            LIST_SIZE * sizeof(short), NULL, &ret);
    cl_mem I2_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            LIST_SIZE * sizeof(short), NULL, &ret);
    cl_mem I3_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            LIST_SIZE * sizeof(short), NULL, &ret);
    cl_mem S0_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            LIST_SIZE * sizeof(short), NULL, &ret);
    cl_mem S1_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            LIST_SIZE * sizeof(short), NULL, &ret);
    cl_mem O_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
            LIST_SIZE * sizeof(short), NULL, &ret);
 
    // Copy the lists to their respective memory buffers
    ret = clEnqueueWriteBuffer(command_queue, I0_mem_obj, CL_TRUE, 0,
            LIST_SIZE * sizeof(short), I0, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, I1_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(short), I1, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, I2_mem_obj, CL_TRUE, 0,
            LIST_SIZE * sizeof(short), I2, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, I3_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(short), I3, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, S0_mem_obj, CL_TRUE, 0,
            LIST_SIZE * sizeof(short), S0, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, S1_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(short), S1, 0, NULL, NULL);
 
    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, 
            (const char **)&source_str, (const size_t *)&source_size, &ret);
 
    // Build the program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if(ret != CL_SUCCESS) {
        std::cerr << getErrorString(ret) << std::endl;
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

        // Allocate memory for the log
        char *log = (char *) malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        // Print the log
        printf("%s\n", log);
        exit(1);
    }
 
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "mux", &ret);
 
    // Set the arguments of the kernel
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&I0_mem_obj);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&I1_mem_obj);
    ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&I2_mem_obj);
    ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&I3_mem_obj);
    ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&S0_mem_obj);
    ret = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&S1_mem_obj);
    ret = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&O_mem_obj);
    ret = clSetKernelArg(kernel, 7, sizeof(unsigned int), &LIST_SIZE);
 
    // Execute the OpenCL kernel on the list
    size_t local_item_size = 64; // Divide work items into groups of 64
    size_t global_item_size = ceil(LIST_SIZE/(float)local_item_size)*local_item_size;
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
            &global_item_size, &local_item_size, 0, NULL, NULL);
 
    // Read the out memory buffer on the device to the local variable
    short *O = (short*)malloc(sizeof(short)*LIST_SIZE);
    ret = clEnqueueReadBuffer(command_queue, O_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(short), O, 0, NULL, NULL);
 
    // Display the result to the screen
    for(i = 0; i < LIST_SIZE; i++)
        std::cout << I3[i] << I2[i] << I1[i] << I0[i] << " + " << S1[i] << S0[i] << " = " << O[i] << '\n';
    
    vcd::Vcd vcd;
    vcd.date = "test date";
    vcd.scope = {"anything"};
    vcd.version = "1.0";
    vcd.timescale = "1tu";
    vcd.comment = "test";
    std::vector<vcd::Signal> signals = {{"wire",1,"0I","I0"},{"wire",1,"1I","I1"},{"wire",1,"2I","I2"},{"wire",1,"3I","I3"},{"wire",1,"0S","S0"},{"wire",1,"1S","S1"},{"wire",1,"O","O"}};
    vcd.signals = signals;
    std::vector<vcd::Dump> initial_dump = {{{'0'},"0I"},{{'0'},"1I"},{{'0'},"2I"},{{'0'},"3I"},{{'0'},"0S"},{{'0'},"1S"},{{'0'},"O"}};
    vcd.initial_dump = initial_dump;
    std::vector<vcd::Timestamp> timestamps;
    for (int i=0; i<LIST_SIZE; ++i) {
        timestamps.push_back({(uint64_t)i,{
            {{to_char(I0[i])},"0I"},
            {{to_char(I1[i])},"1I"},
            {{to_char(I2[i])},"2I"},
            {{to_char(I3[i])},"3I"},
            {{to_char(S0[i])},"0S"},
            {{to_char(S1[i])},"1S"},
            {{to_char(O[i])},"O"}
        }});
    }
    
    vcd.timestamps = timestamps;

    std::ofstream out("out.vcd");
    if (out.is_open()) {
        std::cout << "Compiling file...\n";
        compiler::compilevcd::compile_vcd_file(vcd,out);
        out.close();
    }
 
    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(I0_mem_obj);
    ret = clReleaseMemObject(I1_mem_obj);
    ret = clReleaseMemObject(I2_mem_obj);
    ret = clReleaseMemObject(I3_mem_obj);
    ret = clReleaseMemObject(S0_mem_obj);
    ret = clReleaseMemObject(S1_mem_obj);
    ret = clReleaseMemObject(O_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    free(I0);
    free(I1);
    free(I2);
    free(I3);
    free(S0);
    free(S1);
    free(O);
    return 0;
}
