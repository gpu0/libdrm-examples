#include<stdio.h>
#include<assert.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include"drm.h"
#include"xf86drm.h"
#include"amdgpu.h"
#include"amdgpu_drm.h"

#define NUM_GPUS 10
#define BUFFER_SIZE (4*1024)
#define BUFFER_ALIGN (4*1024)

char gpu_loc[] = "/dev/dri/card0";
int gpu;

/*
xxxxxxx INCOMPLETE xxxxxxxx
*/

int gpu_alloc_buffer_map(amdgpu_device_handle handle,
                                  uint64_t size, uint64_t alignment, 
                                  uint32_t type, uint64_t flags,
                                  amdgpu_bo_handle *bo,  void **cpu,
                                  uint64_t *vmc_addr,
                                  amdgpu_va_handle *va_handle)
{
    struct amdgpu_bo_alloc_request req = {0};
    amdgpu_bo_handle buf_handle;
    int r;

    req.alloc_size = size;
    req.phys_alignment = alignment;
    req.preferred_heap = type;
    req.flags = flags;

    r = amdgpu_bo_alloc(handle, &req, &buf_handle);
    assert(r == 0);

    r = amdgpu_va_range_alloc(handle,
                              amdgpu_gpu_va_range_general,
                              size, alignment, 0, vmc_addr,
                              va_handle, 0);
    assert(r == 0);

    r = amdgpu_bo_va_op(buf_handle, 0, size, *vmc_addr, 0, AMDGPU_VA_OP_MAP);
    assert(r == 0);

    r = amdgpu_bo_cpu_map(buf_handle, cpu);
    assert(r == 0);
    *bo = buf_handle;

    return r;
}

int gpu_free_buffer_unmap(amdgpu_bo_handle bo,
                              amdgpu_va_handle va_handle,
                              uint64_t vmc_addr,
                              uint64_t size)
{
    int r;
    r = amdgpu_bo_cpu_unmap(bo);
    assert(r == 0);

    r = amdgpu_bo_va_op(bo, 0, size, vmc_addr, 0, AMDGPU_VA_OP_UNMAP);
    assert(r == 0);

    r = amdgpu_va_range_free(va_handle);
    assert(r == 0);

    r = amdgpu_bo_free(bo);
    assert(r == 0);

    return 0;
}

int main()
{
    int avail = drmAvailable();
    if(avail == 0){
        return -1;
    }

    int i;
    uint32_t major, minor;
    amdgpu_device_handle handle;
    amdgpu_bo_handle bo_handle;
    amdgpu_va_handle va_handle;
    uint64_t vmc_addr;
    void *cpu;

    for(i=0;i<NUM_GPUS;i++){
      gpu = open(gpu_loc, O_RDWR | O_CLOEXEC);
      if(gpu >= 0){
        drmVersionPtr ver = drmGetVersion(gpu);
        if(ver->name[0] == 'a'){
          int r = amdgpu_device_initialize(gpu, &major, &minor, &handle);
          if(r){
            printf("Unable to initialize AMDGPU\n");
            return -1;
          }
          printf("GPU on %s is [%s]\n", gpu_loc, ver->name);

          const int sdwa_write_length = 1024;
          const int pm4_dw = 256;
          amdgpu_context_handle context_handle;
          amdgpu_bo_handle bo1, bo2;
          amdgpu_bo_handle *resources;
          uint32_t *pm4;
          struct amdgpu_cs_ib_info *ib_info;
          struct amdgpu_cs_request *ibs_request;
          uint64_t bo1_mc, bo2_mc;
          int i, j, loop1, loop2;
          uint64_t gtt_flags[2] = {0, AMDGPU_GEM_CREATE_CPU_GTT_USWC};
          amdgpu_va_handle va_handle;

          pm4 = calloc(pm4_dw, sizeof(*pm4));          
          assert(pm4 != NULL);

          r = amdgpu_device_deinitialize(handle);
          if(r){
            printf("Unable to deinitialize device\n");
          }
        }

        close(gpu);
      }
      gpu_loc[13] += 0x1;
    }

    return 0;
}
